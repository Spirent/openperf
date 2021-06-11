#ifndef _OP_LIST_H_
#define _OP_LIST_H_

/**
 * @file
 *
 * This file includes the function declarations for a lock-free,
 * sorted, singly linked list.  In order to provide lock-free guarantees
 * without excessive overhead, this list can only grow.  Calling
 * the delete function on a key merely marks the node as deleted
 * and prevents it from showing up in subsequent searches.
 *
 * Note that destroying the list _will_ delete all nodes.  Concurrent
 * access should be blocked by some mechanism before calling destroy.
 *
 * This code is based on the implementation described in
 * _High Performance Dynamic Lock-Free Hash Tables and List-Based Sets_
 * by Maged M. Michael.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

struct op_list;
struct op_list_item;

/**
 * typedef for function to compare list entries
 * This function should match the semantics of memcmp/strcmp.
 */
typedef int(op_comparator)(const void* thing1, const void* thing2);

/**
 * typedef for function to destroy list items
 */
typedef void(op_destructor)(void* thing);

/**
 * Allocate a new list.
 *
 * @return
 *   pointer to new allocated list or NULL on error
 */
struct op_list* op_list_allocate();

/**
 * Set the list compare function.
 *
 * @param[in] list
 *   pointer to list
 * @param[in] comparator
 *   item comparing function
 */
void op_list_set_comparator(struct op_list* list, op_comparator comparator);

/**
 * Set the list destroy function.
 *
 * @param[in] list
 *   pointer to list
 * @param[in] destructor
 *   value destructor; may be free for simple heap allocated items
 */
void op_list_set_destructor(struct op_list* list, op_destructor destructor);

/**
 * Destroy a list and free all memory
 * If a destructor was specified, it will be called on all items.
 *
 * @param[in,out] listp
 *  address of pointer to list
 */
void op_list_free(struct op_list** listp);

/**
 * Delete all current items in the list.
 *
 * @param[in] list
 *   pointer to list
 *
 * @return
 *   -  0: Success
 *   - !0: Error
 */
int op_list_purge(struct op_list* list);

/**
 * Destroy all items previously marked for delete.  If a destructor was
 * specified, it will be called on all items.
 * Note: concurrent access should be blocked by an external mechanism
 * before calling this function.
 *
 * @param[in] list
 *   pointer to list
 */
void op_list_garbage_collect(struct op_list* list);

/**
 * Retrieve the current length of the list
 *
 * @param[in] list
 *   pointer to list
 *
 * @return
 *   number of non-deleted items in the list
 */
size_t op_list_length(struct op_list* list);

/**
 * Retrieve the head of the list
 * Note: The head list item never contains user data.
 *
 * @param[in] list
 *   pointer to list
 *
 * @return
 *   pointer to first item in the list
 */
struct op_list_item* op_list_head(struct op_list* list);

/**
 * Retrieve the next item in the list
 *
 * @param[in] list
 *   pointer to list
 * @param[in,out] prevp
 *   address of pointer to previous list item
 *
 * @return
 *   pointer to value of next item or NULL on error
 */
void* op_list_next(struct op_list* list, struct op_list_item** prevp);

/**
 * Allocate a new list item
 *
 * @param[in] item
 *   pointer of item to add to the list
 *
 * @return
 *   pointer to new allocated list item or NULL on error
 */
struct op_list_item* op_list_item_allocate(void* item);

/**
 * Free a list item structure.
 * Note: key/value are not touched by this function!
 *
 * @param[in,out] itemp
 *   address of pointer to list item
 */
void op_list_item_free(struct op_list_item** itemp);

/**
 * Retrieve the data from an op_list_item
 *
 * @param[in] item
 *   pointer to list item
 */
void* op_list_item_data(const struct op_list_item* item);

/**
 * Insert items into the list
 *
 * @param[in] list
 *   pointer to list
 * @param[in] item
 *   pointer to item to insert into the list
 *
 * @return
 *   -  true: item was successfully inserted
 *   - false: item was not inserted
 */
bool op_list_insert_head_item(struct op_list* list, struct op_list_item* item);

/**
 * Insert items into the list after the specified node.
 *
 * @param[in] list
 *   pointer to list
 * @param[in] start
 *   pointer to existing list item from which to begin the insertion process;
 * @param[in] item
 *   pointer to item to insert into the list
 *
 * @return
 *   -  true: item was successfully inserted
 *   - false: item was not inserted
 */
bool op_list_insert_node_item(struct op_list* list,
                              struct op_list_item* start,
                              struct op_list_item* item);

/**
 * Insert items into the list
 *
 * @param[in] list
 *   pointer to list
 * @param[in] item
 *   data to add to the list
 *
 * @return
 *   -  true: item was successfully inserted
 *   - false: item was not inserted
 */
bool op_list_insert_head_value(struct op_list* list, void* item);

/**
 * Insert items into the list
 *
 * @param[in] list
 *   pointer to list
 * @param[in] start
 *   pointer to existing list item from which to begin the insertion process;
 * @param[in] item
 *   data to add to the list
 *
 * @return
 *   -  true: item was successfully inserted
 *   - false: item was not inserted
 */
bool op_list_insert_node_value(struct op_list* list,
                               struct op_list_item* start,
                               void* item);

#define op_list_insert_head(list, item)                                        \
    _Generic((item), struct op_list_item *                                     \
             : op_list_insert_head_item, default                               \
             : op_list_insert_head_value)(list, item)

#define op_list_insert_node(list, start, item)                                 \
    _Generic((item), struct op_list_item *                                     \
             : op_list_insert_node_item, default                               \
             : op_list_insert_node_value)(list, start, item)

#define GET_LIST_INSERT_MACRO(_1, _2, _3, NAME, ...) NAME
#define op_list_insert(...)                                                    \
    GET_LIST_INSERT_MACRO(                                                     \
        __VA_ARGS__, op_list_insert_node, op_list_insert_head)                 \
    (__VA_ARGS__)

/**
 * Look for key in the list.  Return the value if found.
 *
 * @param[in] list
 *   pointer to list
 * @param[in] key
 *   key to find
 *
 * @return
 *   item value if found, otherwise NULL
 */
void* op_list_find_head(struct op_list* list, const void* key);

/**
 * Look for key in the list starting at the specified node.
 * Return the value if found.
 *
 * @param[in] list
 *   pointer to list
 * @param[in] start
 *   pointer to existing list item from which to begin the search;
 * @param[in] key
 *   key to find
 *
 * @return
 *   item value if found, otherwise NULL
 */
void* op_list_find_node(struct op_list* list,
                        struct op_list_item* start,
                        const void* key);

#define GET_LIST_FIND_MACRO(_1, _2, _3, NAME, ...) NAME
#define op_list_find(...)                                                      \
    GET_LIST_FIND_MACRO(__VA_ARGS__, op_list_find_node, op_list_find_head)     \
    (__VA_ARGS__)

/**
 * Look for key in the list.  Mark it deleted if found.
 *
 * @param[in] list
 *   pointer to list
 * @param[in] key
 *   item to delete
 *
 * @return
 *   -  true: item marked for delete
 *   - false: item not found
 */
bool op_list_delete_head(struct op_list* list, const void* key);

/**
 * Look for key in the list starting at the specified node.
 * Mark it deleted if found.
 *
 * @param[in] list
 *   pointer to list
 * @param[in] start
 *   pointer to existing list item from which to begin the search;
 * @param[in] key
 *   item to delete
 *
 * @return
 *   -  true: item marked for delete
 *   - false: item not found
 */
bool op_list_delete_node(struct op_list* list,
                         struct op_list_item* start,
                         const void* key);

#define GET_LIST_DELETE_MACRO(_1, _2, _3, NAME, ...) NAME
#define op_list_delete(...)                                                    \
    GET_LIST_DELETE_MACRO(                                                     \
        __VA_ARGS__, op_list_delete_node, op_list_delete_head)                 \
    (__VA_ARGS__)

/**
 * Look for key in the list. Delete it and call the destructor
 * immediately if found.
 * XXX: Clients are responsible for ensuring that this item cannot
 * be used by other threads.
 *
 * @param[in] list
 *  pointer to list
 * @param[in] key
 *  item to delete
 *
 * @return
 *   - true: item deleted
 *   - false: item not found
 */
bool op_list_clear_head(struct op_list* list, const void* key);

/**
 * Look for key in the list starting at the specified node.
 * Delete it and call the destructor immediately if found.
 * XXX: Clients are responsible for ensuring that this item
 * cannot be used by other threads.
 *
 * @param[in] list
 *   pointer to list
 * @param[in] start
 *   pointer to existing list item from which to begin the search;
 * @param[in] key
 *   item to delete
 *
 * @return
 *   -  true: item deleted
 *   - false: item not found
 */
bool op_list_clear_node(struct op_list* list,
                        struct op_list_item* start,
                        const void* key);

#define GET_LIST_CLEAR_MACRO(_1, _2, _3, NAME, ...) NAME
#define op_list_clear(...)                                                     \
    GET_LIST_CLEAR_MACRO(__VA_ARGS__, op_list_clear_node, op_list_clear_head)  \
    (__VA_ARGS__)

/**
 * Retrieve an array of item pointers.  Caller is responsible
 * for freeing itemsp when done.
 *
 * @param[in] list
 *   pointer to a list of items
 * @param[out] itemsp
 *   address of pointer to items
 * @param[out] nb_items
 *   pointer to nb_items
 *
 * @return
 *   -  0: Success
 *   - !0: Error
 */
int op_list_snapshot(struct op_list* list, void** itemsp[], size_t* nb_items);

#ifdef __cplusplus
}
#endif

#endif /* _OP_LIST_H_ */
