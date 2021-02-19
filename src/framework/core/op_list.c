#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "core/op_list.h"

const void* op_dummy_list_value = (void*)0x0ddba11;

/**
 * Internal flags, structures, and defines
 */
enum op_list_entry_flags {
    LIST_ENTRY_DELETED = (1 << 0),
};

struct op_list_item;

struct op_list_entry
{
    uint32_t version;          /**< version for avoiding aba problem */
    uint32_t flags;            /**< attributes for this node */
    struct op_list_item* next; /**< pointer to list item */
} __attribute__((aligned(16)));

#define OP_LIST_ENTRY_INIT                                                     \
    {                                                                          \
        0, 0, NULL                                                             \
    }

struct op_list_item
{
    _Atomic struct op_list_entry entry;
    void* data;
};

/*
 * XXX: Double word alignment is necessary for all
 * struct op_list_item members
 */
struct op_list
{
    struct op_list_item head;      /**< list head */
    atomic_size_t length;          /**< # of items in the list */
    op_comparator* comparator;     /**< list key compare function */
    op_destructor* destructor;     /**< list item destructor */
    atomic_size_t free_length;     /**< # of items on the freelist */
    struct op_list_item free_head; /**< freelist head */
} __attribute__((aligned(64)));

/**
 * Various static functions
 */
static bool _is_deleted(struct op_list_entry* p)
{
    return (p->flags & LIST_ENTRY_DELETED ? true : false);
}

static void _list_item_destroy(struct op_list_item** itemp,
                               op_destructor destructor)
{
    struct op_list_item* item = *itemp;

    if (destructor) { destructor(item->data); }

    op_list_item_free(itemp);
}

static int _list_item_compare(const void* thing1, const void* thing2)
{
    if (thing1 == thing2) {
        return (0);
    } else if (thing1 < thing2) {
        return (-1);
    } else {
        return (1);
    }
}

void _initialize_head_item(struct op_list_item* head)
{
    head->data = NULL;
    struct op_list_entry head_entry = OP_LIST_ENTRY_INIT;
    atomic_init(&head->entry, head_entry);
}

/**
 * Move deleted items to the free list.
 */
void _op_list_collect_deleted(struct op_list* list)
{
    /* op_list_next moves deleted items to the free list, so just need to iterate through the list */
    struct op_list_item* cursor = op_list_head(list);
    while(op_list_next(list, &cursor)){}
}

struct op_list* op_list_allocate()
{
    struct op_list* list = NULL;
    if (posix_memalign((void**)&list, 64, sizeof(*list)) != 0) {
        return (NULL);
    }
    list->length = ATOMIC_VAR_INIT(0);
    list->free_length = ATOMIC_VAR_INIT(0);
    list->comparator = _list_item_compare;
    list->destructor = NULL;

    /* Initialize list heads */
    _initialize_head_item(&list->head);
    _initialize_head_item(&list->free_head);

    return (list);
}

void op_list_set_comparator(struct op_list* list, op_comparator comparator)
{
    list->comparator = comparator;
}

void op_list_set_destructor(struct op_list* list, op_destructor destructor)
{
    list->destructor = destructor;
}

void _destroy_list_items(struct op_list_item* head, op_destructor destructor)
{
    struct op_list_entry head_entry = atomic_load(&head->entry);
    struct op_list_item* current = head_entry.next;

    while (current != NULL) {
        struct op_list_entry current_entry = atomic_load(&current->entry);
        struct op_list_item* next = current_entry.next;
        _list_item_destroy(&current, destructor);
        current = next;
    }

    /* head is now empty; update entry */
    head_entry.version++;
    head_entry.next = NULL;
    atomic_store(&head->entry, head_entry);
}

/**
 * XXX: Assumes that concurrent use has been halted by an external
 * mechanism...
 */
void op_list_free(struct op_list** listp)
{
    struct op_list* list = *listp;

    /* Free everything in the lists */
    _destroy_list_items(&list->head, list->destructor);
    _destroy_list_items(&list->free_head, list->destructor);

    free(list);
    *listp = NULL;
}

int op_list_purge(struct op_list* list)
{
    void** items = NULL;
    size_t nb_items = 0;
    int error = 0;

    if ((error = op_list_snapshot(list, &items, &nb_items)) != 0) {
        return (error);
    }

    for (size_t i = 0; i < nb_items; i++) {
        void* item = items[i];
        op_list_delete(list, item);
    }

    free(items);

    return (0);
}

/**
 * XXX: Assumes that concurrent use has been halted by an external
 * mechanism...
 */
void op_list_garbage_collect(struct op_list* list)
{
    _op_list_collect_deleted(list);
    _destroy_list_items(&list->free_head, list->destructor);
    atomic_store(&list->free_length, 0);
}

struct op_list_item* op_list_item_allocate(void* value)
{
    assert(value);

    struct op_list_item* item = calloc(1, sizeof(*item));
    if (!item) { return (NULL); }

    item->data = value;

    struct op_list_entry item_entry = OP_LIST_ENTRY_INIT;
    atomic_init(&item->entry, item_entry);

    return (item);
}

void op_list_item_free(struct op_list_item** itemp)

{
    assert(*itemp);
    free(*itemp);
    *itemp = NULL;
}

void* op_list_item_data(const struct op_list_item* item)
{
    return (item->data);
}

/**
 * Insert an item on the free list
 * It should have already been marked for delete
 */
bool _freelist_insert(struct op_list* list, struct op_list_item* node)
{
    assert(list);
    assert(node);

    struct op_list_item* head = &list->free_head;
    struct op_list_entry head_now =
        atomic_load_explicit(&head->entry, memory_order_acquire);
    struct op_list_entry head_update = OP_LIST_ENTRY_INIT;

    do {
        /* Update node to point to head's next item */
        struct op_list_entry node_entry =
            atomic_load_explicit(&node->entry, memory_order_relaxed);
        assert(node_entry.flags & LIST_ENTRY_DELETED);
        node_entry.version += 1;
        node_entry.next = head_now.next;
        atomic_store_explicit(&node->entry, node_entry, memory_order_relaxed);

        /* Update head to point to node */
        head_update.version = head_now.version + 1;
        head_update.flags = head_now.flags;
        head_update.next = node;

    } while (!atomic_compare_exchange_weak_explicit(&head->entry,
                                                    &head_now,
                                                    head_update,
                                                    memory_order_release,
                                                    memory_order_relaxed));

    atomic_fetch_add_explicit(&list->free_length, 1, memory_order_relaxed);
    return (true);
}

int _handle_list_delete(struct op_list* list,
                        struct op_list_item* to_delete,
                        struct op_list_entry* to_delete_entry,
                        struct op_list_item* prev,
                        struct op_list_entry* prev_entry)
{
    /*
     * This node has been marked deleted.
     * Update previous to point to next.
     */
    struct op_list_entry prev_entry_update = {
        .version = prev_entry->version + 1,
        .flags = prev_entry->flags,
        .next = to_delete_entry->next,
    };
    if (atomic_compare_exchange_weak_explicit(&prev->entry,
                                              prev_entry,
                                              prev_entry_update,
                                              memory_order_release,
                                              memory_order_relaxed)) {
        /*
         * Current is no longer in the list.
         * Stick it on the freelist so we don't lose it.
         * Note: since previous now has a pointer to next, we
         * don't change our previous pointer on this code path.
         */
        _freelist_insert(list, to_delete);
        return (0);
    } else {
        /* Another thread beat us updating this entry. */
        return (-1);
    }
}

/**
 * Find the last node less than or equal to the search key
 * This function uses prev and prev_entry as working values so that calling
 * functions can use the exact same list entries as this function for their
 * CAS operations.  This ensures the atomicity of the insert, find, and
 * delete operations on the list.
 */
bool _op_list_pfind(struct op_list* list,
                    struct op_list_item* start,
                    const void* key,
                    struct op_list_item** prev,
                    struct op_list_entry* prev_entry)
{
    assert(list);
    assert(start);
    assert(key);
    assert(prev_entry);
    assert(prev);

start:
    *prev = start;
    *prev_entry = atomic_load_explicit(&(*prev)->entry, memory_order_acquire);

    while (prev_entry->next != NULL) {
        struct op_list_item* curr = prev_entry->next;
        struct op_list_entry curr_entry =
            atomic_load_explicit(&curr->entry, memory_order_acquire);

        if (!_is_deleted(&curr_entry)) {
            int result = list->comparator(curr->data, key);
            if (result == 0) { /* equal */
                return (true);
            } else if (result > 0) { /* greater than */
                return (false);
            }
            /* else less than; keep looking */
            *prev = curr;
            *prev_entry = curr_entry;
        } else {
            if (_handle_list_delete(list, curr, &curr_entry, *prev, prev_entry)
                != 0) {
                /*
                 * Some other thread updated the list for us.  Restart
                 * the loop in case other things changed.
                 */
                goto start;
            }
        }
    }

    return (false);
}

bool op_list_insert_head_item(struct op_list* list, struct op_list_item* item)
{
    return (op_list_insert_node(list, &list->head, item));
}

bool op_list_insert_head_value(struct op_list* list, void* value)
{
    struct op_list_item* item = op_list_item_allocate(value);
    if (!item) { return (false); }

    return (op_list_insert_head_item(list, item));
}

bool op_list_insert_node_item(struct op_list* list,
                              struct op_list_item* start,
                              struct op_list_item* item)
{
    assert(list);
    assert(start);
    assert(item);

    struct op_list_item* prev = NULL;
    struct op_list_entry prev_now = OP_LIST_ENTRY_INIT;
    struct op_list_entry prev_update = OP_LIST_ENTRY_INIT;

    /* Now insert into the list */
    do {
        if (_op_list_pfind(list, start, item->data, &prev, &prev_now)) {
            return (false); /* item already in list */
        }

        /* New node goes after previous */
        /* Update node to point to whatever is after previous */
        struct op_list_entry item_next = {0, 0, prev_now.next};
        atomic_store_explicit(&item->entry, item_next, memory_order_relaxed);

        /* Update previous to point to our new node */
        prev_update.version = prev_now.version + 1;
        prev_update.flags = prev_now.flags;
        prev_update.next = item;

        /* Now try to swap */
    } while (!atomic_compare_exchange_weak_explicit(&prev->entry,
                                                    &prev_now,
                                                    prev_update,
                                                    memory_order_release,
                                                    memory_order_relaxed));

    atomic_fetch_add_explicit(&list->length, 1, memory_order_relaxed);
    return (true);
}

bool op_list_insert_node_value(struct op_list* list,
                               struct op_list_item* start,
                               void* value)
{
    struct op_list_item* item = op_list_item_allocate(value);
    if (!item) { return (false); }

    return (op_list_insert_node_item(list, start, item));
}

void* op_list_find_head(struct op_list* list, const void* key)
{
    struct op_list_entry prev_entry = OP_LIST_ENTRY_INIT;
    struct op_list_item* prev = NULL;
    return (_op_list_pfind(list, &list->head, key, &prev, &prev_entry)
                ? prev_entry.next->data
                : NULL);
}

void* op_list_find_node(struct op_list* list,
                        struct op_list_item* start,
                        const void* key)
{
    struct op_list_entry prev_entry = OP_LIST_ENTRY_INIT;
    struct op_list_item* prev = NULL;
    return (_op_list_pfind(list, start, key, &prev, &prev_entry)
                ? prev_entry.next->data
                : NULL);
}

bool op_list_delete_head(struct op_list* list, const void* key)
{
    return (op_list_delete_node(list, &list->head, key));
}

bool op_list_delete_node(struct op_list* list,
                         struct op_list_item* start,
                         const void* key)
{
    assert(list);
    assert(start);
    assert(key);

    struct op_list_item* prev = NULL;
    struct op_list_entry prev_entry = OP_LIST_ENTRY_INIT;
    struct op_list_item* to_delete = NULL;
    struct op_list_entry to_delete_now = OP_LIST_ENTRY_INIT;
    struct op_list_entry to_delete_update = OP_LIST_ENTRY_INIT;

    do {
        if (!_op_list_pfind(list, start, key, &prev, &prev_entry)) {
            return (false); /* no matching key */
        }

        to_delete = prev_entry.next;
        to_delete_now =
            atomic_load_explicit(&to_delete->entry, memory_order_acquire);
        to_delete_update = to_delete_now;
        to_delete_update.version = to_delete_now.version + 1;
        to_delete_update.flags |= LIST_ENTRY_DELETED;
    } while (!atomic_compare_exchange_weak_explicit(&to_delete->entry,
                                                    &to_delete_now,
                                                    to_delete_update,
                                                    memory_order_release,
                                                    memory_order_relaxed));

    atomic_fetch_sub_explicit(&list->length, 1, memory_order_relaxed);
    return (true);
}

size_t op_list_length(struct op_list* list)
{
    size_t length = atomic_load_explicit(&list->length, memory_order_relaxed);
    return (length);
}

struct op_list_item* op_list_head(struct op_list* list)
{
    return (&list->head);
}

void* op_list_next(struct op_list* list, struct op_list_item** prevp)
{
    (void)list;
    struct op_list_item* prev = *prevp;
    struct op_list_entry prev_entry =
        atomic_load_explicit(&prev->entry, memory_order_acquire);

    /* Make sure previous has not been deleted */
    if (_is_deleted(&prev_entry)) {
        /* There is no next; terminate. */
        *prevp = NULL;
        return (NULL);
    }

    /* Look for the next item */
    while (prev_entry.next != NULL) {
        struct op_list_item* curr = prev_entry.next;
        struct op_list_entry curr_entry =
            atomic_load_explicit(&curr->entry, memory_order_acquire);

        /* Return the first non-deleted item we come across in the list */
        if (!_is_deleted(&curr_entry)) {
            *prevp = curr;
            return (curr->data);
        } else {
            /* Try to remove the deleted node from the list */
            if (_handle_list_delete(list, curr, &curr_entry, prev, &prev_entry)
                == 0) {
                /* On success, we can continue our search */
                continue;
            } else {
                /*
                 * Some other thread beat us updating this entry.
                 * There is no next.
                 */
                break;
            }
        }
    }

    *prevp = NULL;
    return (NULL);
}

int op_list_snapshot(struct op_list* list, void** itemsp[], size_t* nb_items)
{
    void** items = NULL;

    do {
        size_t length = op_list_length(list);
        items = realloc(items, length * sizeof(*items));
        if (!items) { return (-ENOMEM); }
        struct op_list_item* cursor = op_list_head(list);
        void* current = op_list_next(list, &cursor);
        for (*nb_items = 0; *nb_items < length && current != NULL;
             (*nb_items)++, current = op_list_next(list, &cursor)) {
            items[*nb_items] = current;
        }
        /* If the length has changed, then the list has definitely mutated. */
    } while (*nb_items != op_list_length(list));

    *itemsp = items;
    return (0);
}
