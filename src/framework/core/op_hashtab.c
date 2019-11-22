#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core/op_common.h"
#include "core/op_list.h"
#include "core/op_hashtab.h"

static const size_t _load_factor = 4;  /**< Specifies max ratio of list entries
                                        * to table size before increasing table
                                        * entries */
static void *dummy_value = (void *)0x0ddba11;  /**< Used to tag dummy key/value entries */

struct op_hashtab_key_value {
    void *key;                         /**< item key */
    void *value;                       /**< item value */
    op_destructor *key_destructor;    /**< key destructor; may be NULL */
    op_destructor *value_destructor;  /**< value destructor; may be NULL */
};

struct op_hashtab {
    struct op_list_item **buckets;    /**< bucket table */
    struct op_list *list;             /**< backend list */
    op_hasher *hasher;                /**< hashing function */
    op_destructor *key_destructor;    /**< key destructor */
    op_destructor *value_destructor;  /**< value destructor */
    size_t max_tab_size;               /**< largest possible tab size */
    atomic_size_t tab_size;            /**< # of valid table entries */
    atomic_size_t list_size;           /**< # of list entries (excludes dummy nodes) */
};

/**
 * Hash taken from...
 * https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
 */
static __attribute__((pure))
uintptr_t _hashtab_hash(const void *thing)
{
    uintptr_t x = (uintptr_t)thing;
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccd;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53;
    x ^= x >> 33;

    return (x);
}

static __attribute__((pure))
int _hashtab_comparator(const void *thing1, const void *thing2)
{
    const void *key1 = thing1 ? ((const struct op_hashtab_key_value *)thing1)->key : NULL;
    const void *key2 = thing2 ? ((const struct op_hashtab_key_value *)thing2)->key : NULL;

    if (key1 == key2) {
        return (0);
    } else if (key1 < key2) {
        return (-1);
    } else {
        return (1);
    }
}

/**
 * Reverse bits with a lookup table
 * http://graphics.stanford.edu/~seander/bithacks.html#BitReverseTable
 */
#define R2(n)   (n),   (n + 2 * 64),   (n + 1 * 64),   (n + 3 * 64)
#define R4(n) R2(n), R2(n + 2 * 16), R2(n + 1 * 16), R2(n + 3 * 16)
#define R6(n) R4(n), R4(n + 2 *  4), R4(n + 1 *  4), R4(n + 3 *  4)

static const uint8_t _bit_reverse_table[256] = {
    R6(0), R6(2), R6(1), R6(3)
};

static __attribute__((const))
uintptr_t _reverse_bits(uintptr_t x)
{
    uintptr_t r = ((uint64_t)_bit_reverse_table[x & 0xff]  << 56
                   | (uint64_t)_bit_reverse_table[(x >> 8)  & 0xff] << 48
                   | (uint64_t)_bit_reverse_table[(x >> 16) & 0xff] << 40
                   | (uint64_t)_bit_reverse_table[(x >> 24) & 0xff] << 32
                   | (uint64_t)_bit_reverse_table[(x >> 32) & 0xff] << 24
                   | (uint64_t)_bit_reverse_table[(x >> 40) & 0xff] << 16
                   | (uint64_t)_bit_reverse_table[(x >> 48) & 0xff] << 8
                   | (uint64_t)_bit_reverse_table[(x >> 58) & 0xff]);

    return (r);
}

static __attribute__((const))
uintptr_t _get_dummy_key(uintptr_t key)
{
    return _reverse_bits(key & ~0x8000000000000000);
}

static __attribute__((const))
uintptr_t _get_regular_key(uintptr_t key)
{
    return _reverse_bits(key | 0x8000000000000000);
}

static __attribute__((const))
uintptr_t _get_parent(uintptr_t idx)
{
    /* Find the position of the MSB */
    uintptr_t x = idx;
    size_t p = 0;

    while (x >>= 1) {
        p++;
    }

    /* And set it to 0 */
    return (idx & ~(1 << p));
}

static struct op_list_item *
_hashtab_item_allocate(struct op_hashtab *tab, void *key, void *value)
{
    struct op_hashtab_key_value *kv = calloc(1, sizeof(*kv));
    if (!kv) {
        return (NULL);
    }

    kv->key = key;
    kv->value = value;

    if (value != dummy_value) {
        kv->key_destructor = tab->key_destructor;
        kv->value_destructor = tab->value_destructor;
    }

    return (op_list_item_allocate(kv));
}

static void _hashtab_key_value_free(void *thing)
{
    struct op_hashtab_key_value *kv = thing;
    if (kv->key_destructor) {
        kv->key_destructor(kv->key);
    }

    if (kv->value_destructor) {
        kv->value_destructor(kv->value);
    }

    free(kv);
}

/**
 * Note: this function is intended to be called on the error path of functions
 * that allocate hashtab specific op_list_items.  Because of this, it
 * intentionally *does not* call the key/value destructor functions.
 * The rationale is that on a failed insert, the hash table does not
 * need to assume ownership of the key/value pairs that failed to get
 * added to the table.
 */
static void
_hashtab_item_free(struct op_list_item **itemp)
{
    free(op_list_item_data(*itemp));
    op_list_item_free(itemp);
}

static
void _initialize_bucket(struct op_hashtab *tab, size_t bucket_idx)
{
    struct op_list_item *parent = NULL;
    size_t parent_idx = _get_parent(bucket_idx);
    while ((parent = tab->buckets[parent_idx]) == NULL) {
        _initialize_bucket(tab, parent_idx);
    }

    struct op_list_item *dummy =
        _hashtab_item_allocate(tab, (void *)_get_dummy_key(bucket_idx), dummy_value);
    assert(dummy);

    if (!op_list_insert(tab->list, parent, dummy)) {
        _hashtab_item_free(&dummy);
        return;  /* we lost a race with another thread */
    }

    tab->buckets[bucket_idx] = dummy;
}

static
size_t _get_bucket_size(struct op_hashtab *tab)
{
    size_t size = atomic_load_explicit(&tab->tab_size, memory_order_relaxed);
    return (size);
}

static
struct op_list_item *_get_bucket(struct op_hashtab *tab, uintptr_t key)
{
    struct op_list_item *bucket = NULL;
    size_t bucket_idx = key % _get_bucket_size(tab);
    while ((bucket = tab->buckets[bucket_idx]) == NULL) {
        _initialize_bucket(tab, bucket_idx);
    }

    return (bucket);
}

struct op_hashtab * op_hashtab_allocate()
{
    static long max_tab_size = 0;
    if (!max_tab_size) {
        max_tab_size = op_max(4096, sysconf(_SC_PAGESIZE)) / sizeof(struct op_list_item *);
    }

    struct op_hashtab *tab = calloc(1, sizeof(*tab));
    if (!tab) {
        return (NULL);
    }

    if ((tab->buckets = calloc(max_tab_size, sizeof(*tab->buckets))) == NULL) {
        free(tab);
        return (NULL);
    }

    tab->list = op_list_allocate();
    if (!tab->list) {
        free(tab->buckets);
        free(tab);
        return (NULL);
    }

    op_list_set_comparator(tab->list, _hashtab_comparator);
    op_list_set_destructor(tab->list, _hashtab_key_value_free);
    tab->buckets[0] = op_list_head(tab->list);
    tab->hasher = _hashtab_hash;
    tab->max_tab_size = max_tab_size;
    tab->tab_size = ATOMIC_VAR_INIT(2);
    tab->list_size = ATOMIC_VAR_INIT(0);

    return (tab);
}

void op_hashtab_set_hasher(struct op_hashtab *tab, op_hasher hasher)
{
    tab->hasher = hasher;
}

void op_hashtab_set_key_destructor(struct op_hashtab *tab, op_destructor destructor)
{
    tab->key_destructor = destructor;
}

void op_hashtab_set_value_destructor(struct op_hashtab *tab, op_destructor destructor)
{
    tab->value_destructor = destructor;
}

/**
 * XXX: Assumes that contention has been halted by an
 * external mechanism...
 */
void op_hashtab_free(struct op_hashtab **tabp)
{
    struct op_hashtab *tab = *tabp;
    free(tab->buckets);
    op_list_free(&tab->list);
    free(tab);
    *tabp = NULL;
}

int op_hashtab_purge(struct op_hashtab *tab)
{
    struct op_hashtab_key_value **items = NULL;
    size_t nb_items = 0;
    int error = 0;

    if ((error = op_list_snapshot(tab->list, (void ***)&items, &nb_items)) != 0) {
        return (error);
    }

    for (size_t i = 0; i < nb_items; i++) {
        struct op_hashtab_key_value *item = items[i];
        if (item->value == dummy_value) {
            continue;
        }

        /*
         * We have no way to retrieve the original key, so we can't find the
         * closest bucket to index into the list, hence do things the slow
         * way.
         */
        if (op_list_delete(tab->list, item)) {
            atomic_fetch_sub_explicit(&tab->list_size, 1, memory_order_relaxed);
        }
    }

    free(items);

    return (0);
}

void op_hashtab_garbage_collect(struct op_hashtab *tab)
{
    op_list_garbage_collect(tab->list);
}

bool op_hashtab_insert(struct op_hashtab *tab, void *key, void *value)
{
    uintptr_t hashed_key = tab->hasher(key);
    struct op_list_item *item =
        _hashtab_item_allocate(tab, (void *)_get_regular_key(hashed_key), value);
    if (!item) {
        return (false);
    }

    struct op_list_item *bucket = _get_bucket(tab, hashed_key);

    if (!op_list_insert(tab->list, bucket, item)) {
        _hashtab_item_free(&item);
        return (false);
    }

    size_t tab_size = atomic_load_explicit(&tab->tab_size,
                                           memory_order_relaxed);
    if ((((atomic_fetch_add_explicit(&tab->list_size, 1, memory_order_relaxed)
           + 1) / tab_size) > _load_factor)
        && (tab_size * 2 < tab->max_tab_size)) {
        /*
         * Try to update the tab size.  A failure here just means either
         * some other thread already changed it for us or we encountered
         * a spurious error, in which case the next caller will attempt
         * the update.
         */
        atomic_compare_exchange_weak_explicit(&tab->tab_size,
                                              &tab_size,
                                              tab_size * 2,
                                              memory_order_release,
                                              memory_order_relaxed);
    }

    return (true);
}

void *op_hashtab_find(struct op_hashtab *tab, const void *key)
{
    uintptr_t hashed_key = tab->hasher(key);
    struct op_hashtab_key_value to_find = {
        .key = (void *)_get_regular_key(hashed_key),
    };
    struct op_list_item *bucket = _get_bucket(tab, hashed_key);
    struct op_hashtab_key_value *kv  = op_list_find(tab->list, bucket, &to_find);

    return (kv ? kv->value : NULL);
}

void *op_hashtab_next(struct op_hashtab *tab, void **cursor)
{
    struct op_hashtab_key_value *kv = NULL;

    /*
     * Make sure there is something in the table to return,
     * otherwise abort.
     */
    if (op_hashtab_size(tab) == 0) {
        *cursor = NULL;
        return (NULL);
    }

    /* Start searching for next at either the cursor or the head */
    struct op_list_item *prev = *cursor ? *cursor : op_list_head(tab->list);
    while ((kv = op_list_next(tab->list, &prev)) == NULL
           || kv->value == dummy_value) {
        if (!prev) {
            /*
             * If the cursor comes back NULL, then our search needs to
             * be restarted at the head.
             */
            prev = op_list_head(tab->list);
        }
    }

    *cursor = prev;
    return (kv->value);
}

void *op_hashtab_iterate(struct op_hashtab *tab, void **cursor)
{
    struct op_hashtab_key_value *kv = NULL;

    /*
     * Make sure there is something in the table to return,
     * otherwise abort.
     */
    if (op_hashtab_size(tab) == 0) {
        *cursor = NULL;
        return (NULL);
    }

    /* Start searching for next at either the cursor or the head */
    struct op_list_item *prev = *cursor ? *cursor : op_list_head(tab->list);
    do {
        kv = op_list_next(tab->list, &prev);
    } while (kv != NULL && kv->value == dummy_value);

    *cursor = prev;
    return (kv ? kv->value : NULL);
}

bool op_hashtab_delete(struct op_hashtab *tab, const void *key)
{
    uintptr_t hashed_key = tab->hasher(key);
    struct op_hashtab_key_value to_delete = {
        .key = (void *)_get_regular_key(hashed_key),
    };
    struct op_list_item *bucket = _get_bucket(tab, hashed_key);
    if (op_list_delete(tab->list, bucket, &to_delete)) {
        atomic_fetch_sub_explicit(&tab->list_size, 1, memory_order_relaxed);
        return (true);
    }

    return (false);
}

size_t op_hashtab_size(struct op_hashtab *tab)
{
    size_t size = atomic_load_explicit(&tab->list_size, memory_order_relaxed);
    return (size);
}

int op_hashtab_snapshot(struct op_hashtab *tab, void **itemsp[], size_t *nb_items)
{
    void **items = NULL;

    do {
        size_t length = op_hashtab_size(tab);
        items = realloc(items, length * sizeof(*items));
        if (!items) {
            return (-ENOMEM);
        }
        void *cursor = NULL;
        void *current = op_hashtab_iterate(tab, &cursor);
        for (*nb_items = 0;
             *nb_items < length && current != NULL;
             (*nb_items)++, current = op_hashtab_iterate(tab, &cursor)) {
            items[*nb_items] = current;
        }
    } while (*nb_items != op_hashtab_size(tab));

    *itemsp = items;
    return (0);
}
