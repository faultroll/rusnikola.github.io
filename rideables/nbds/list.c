/* 
 * Written by Josh Dybnis and released to the public domain, as explained at
 * http://creativecommons.org/licenses/publicdomain
 *
 * Harris-Michael lock-free list-based set
 * http://www.research.ibm.com/people/m/michael/spaa-2002.pdf
 */

#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "atomic_c.h"
#include "thread_c.h"
#include "mtracker.h"

// common
#define TRUE  true
#define FALSE false
#define EXPECT_TRUE(x)      __builtin_expect(!!(x), TRUE)
#define EXPECT_FALSE(x)     __builtin_expect(!!(x), FALSE)
#define TRACE(flag, format, v1, v2) do { /* printf("%s:" format "\n", flag, v1, v2); */ } while (0)
// tag
typedef uintptr_t markable_t;
#define TAG_VALUE(v, tag) ((v) |  tag)
#define IS_TAGGED(v, tag) ((v) &  tag)
#define STRIP_TAG(v, tag) ((v) & ~tag)
#define VOLATILE_DEREF(x) (*((volatile __typeof__(x))(x)))

typedef struct node {
    map_key_t  key;
    ATOMIC_VAR(map_val_t)  val;
    ATOMIC_VAR(markable_t) next; // next node
} node_t;

struct ll_iter {
    list_t *ll;
    node_t *pred;
};

struct ll {
    node_t *head;
    /* const */ datatype_t *key_type;
    mt_Inst* tracker;
};

// Marking the <next> field of a node logically removes it from the list
#define  MARK_NODE(x) TAG_VALUE((markable_t)(x), 0x1)
#define   HAS_MARK(x) (IS_TAGGED((x), 0x1) == 0x1)
#define   GET_NODE(x) ((node_t *)(x))
#define STRIP_MARK(x) ((node_t *)STRIP_TAG((x), 0x1))

static node_t *node_alloc (list_t *ll, map_key_t key, map_val_t val) {
    node_t *item = (node_t *)mt_Alloc(ll->tracker, MT_DEFAULT_TID);
    assert(!HAS_MARK((size_t)item));
    if (EXPECT_TRUE(ll->key_type == NULL)) {
        item->key = key;
    } else {
        item->key = (map_key_t)((uintptr_t)item + sizeof(node_t));
    }
    item->val = val;
    return item;
}

static void node_free(list_t *ll, node_t *item)
{
    mt_Retire(ll->tracker, MT_DEFAULT_TID, item);
}

list_t *ll_alloc (const datatype_t *key_type) {
    list_t *ll = (list_t *)malloc(sizeof(list_t));
    // Three ways tracking key_type
    // 1. store in node
    // 2. each list has two trackers
    //    one common tracker, for node; the other in list, for key
    // (we use this)3. alloc with node (just like tracker info)
    //    (not using this)we can treat no key_type as map_key_t key_type
    mt_Type type = MT_HE;
    mt_Config config = MT_DEFAULT_CONF(sizeof(node_t));
    if (EXPECT_FALSE(key_type != NULL)) {
        ll->key_type = (datatype_t *)malloc(sizeof(datatype_t));
        memcpy(ll->key_type, key_type, sizeof(datatype_t));
        config.mem_size += ll->key_type->size;
    } else {
        ll->key_type = NULL;
    }
    ll->tracker = mt_Create(type, config);
    ll->head = node_alloc(ll, 0, 0);
    ll->head->next = DOES_NOT_EXIST;
    return ll;
}

void ll_free (list_t *ll) {
    node_t *item = STRIP_MARK(ll->head->next);
    while (item) {
        node_t *next = STRIP_MARK(item->next);
        node_free(ll, item);
        item = next;
    }
    node_free(ll, ll->head);
    mt_Destroy(ll->tracker);
    if (EXPECT_FALSE(ll->key_type != NULL)) {
        free(ll->key_type);
    }
    free(ll);
}

size_t ll_count (list_t *ll) {
    size_t count = 0;
    node_t *item = STRIP_MARK(ll->head->next);
    while (item) {
        if (!HAS_MARK(item->next)) {
            count++;
        }
        item = STRIP_MARK(item->next);
    }
    return count;
}

static int find_pred (node_t **pred_ptr, node_t **item_ptr, list_t *ll, map_key_t key, int help_remove) {
    node_t *pred = ll->head;
    node_t *item = GET_NODE(pred->next); // mt_Read(ll->tracker, MT_DEFAULT_TID, 1, GET_NODE(pred->next));
    // if (HAS_MARK((markable_t)item))
    //     TRACE("l4", "find_pred: tracker slot 1 %p is marked", item, 0);
    mt_Read(ll->tracker, MT_DEFAULT_TID, 1, STRIP_MARK((markable_t)item)); // unmarked_ptr is hazard for reclaim, not marked_ptr
    TRACE("l2", "find_pred: searching for key %p in list (head is %p)", key, pred);

    // haz_t *temp, *hp0 = haz_get_static(0), *hp1 = haz_get_static(1);
    while (item != NULL) {
        // haz_set(hp0, item);
        if (mt_Read(ll->tracker, MT_DEFAULT_TID, 2, STRIP_MARK(pred->next)) != item)
            return find_pred(pred_ptr, item_ptr, ll, key, help_remove); // retry

        markable_t next = (markable_t)GET_NODE(item->next); // mt_Read(ll->tracker, MT_DEFAULT_TID, 0, GET_NODE(item->next));
        // if (HAS_MARK(next))
        //     TRACE("l4", "find_pred: tracker slot 0 %p is marked", next, 0);
        mt_Read(ll->tracker, MT_DEFAULT_TID, 0, STRIP_MARK(next)); // same as hazard slot 1

        // A mark means the node is logically removed but not physically unlinked yet.
        while (EXPECT_FALSE(HAS_MARK(next))) {
            // Skip over logically removed items.
            if (!help_remove) {
                item = STRIP_MARK(item->next);
                if (EXPECT_FALSE(item == NULL))
                    break;
                TRACE("l3", "find_pred: skipping marked item %p (next is %p)", item, next);
                next = item->next;
                continue;
            }

            // Unlink logically removed items.
            TRACE("l3", "find_pred: unlinking marked item %p next is %p", item, next);

            markable_t other = ATOMIC_VAR_CAS(&pred->next, (markable_t)item, (markable_t)STRIP_MARK(next));
            if (other == (markable_t)item) {
                TRACE("l2", "find_pred: unlinked item %p from pred %p", item, pred);
                item = STRIP_MARK(next);
                next = (item != NULL) ? item->next : DOES_NOT_EXIST;
                TRACE("l3", "find_pred: now current item is %p next is %p", item, next);

                // The thread that completes the unlink should free the memory.
                node_free(ll, GET_NODE(other));
            } else {
                TRACE("l2", "find_pred: lost a race to unlink item %p from pred %p", item, pred);
                TRACE("l2", "find_pred: pred's link changed to %p", other, 0);
                if (HAS_MARK(other))
                    return find_pred(pred_ptr, item_ptr, ll, key, help_remove); // retry
                item = GET_NODE(other);
                next = (item != NULL) ? item->next : DOES_NOT_EXIST;
            }
        }

        if (EXPECT_FALSE(item == NULL))
            break;

        TRACE("l3", "find_pred: visiting item %p (next is %p)", item, next);
        TRACE("l4", "find_pred: key %p val %p", item->key, item->val);

        // if (EXPECT_FALSE((void *)item->key == NULL))
        //     break;

        int d;
        if (EXPECT_TRUE(ll->key_type == NULL)) {
            d = item->key - key;
        } else {
            d = ll->key_type->cmp((void *)item->key, (void *)key);
        }

        // If we reached the key (or passed where it should be), we found the right predesssor
        if (d >= 0) {
            if (pred_ptr != NULL) {
                *pred_ptr = pred;
            }
            if (item_ptr != NULL) {
                *item_ptr = item;
            }
            if (d == 0) {
                TRACE("l2", "find_pred: found matching item %p in list, pred is %p", item, pred);
                return TRUE;
            } 
            TRACE("l2", "find_pred: found proper place for key %p in list, pred is %p", key, pred);
            return FALSE;
        }

        pred = item;

        // temp = hp0; hp0 = hp1; hp1 = temp;
        // mt_Transfer(ll->tracker, MT_DEFAULT_TID, 0, 1);

        item = GET_NODE(next);
    }

    // <key> is not in <ll>.
    if (pred_ptr != NULL) {
        *pred_ptr = pred;
    }
    *item_ptr = NULL;
    TRACE("l2", "find_pred: reached end of list. last item is %p", pred, 0);
    return FALSE;
}

// Fast find. Do not help unlink partially removed nodes and do not return the found item's predecessor.
map_val_t ll_lookup (list_t *ll, map_key_t key) {
    TRACE("l1", "ll_lookup: searching for key %p in list %p", key, ll);
    node_t *item;

    mt_StartOp(ll->tracker, MT_DEFAULT_TID);
    int found = find_pred(NULL, &item, ll, key, FALSE);
    // If we found an <item> matching the key return its value.
    if (found) {
        map_val_t val = item->val;
        if (val != DOES_NOT_EXIST) {
            TRACE("l1", "ll_lookup: found item %p. val %p. returning item", item, item->val);
            mt_EndOp(ll->tracker, MT_DEFAULT_TID);
            return val;
        }
    }
    mt_EndOp(ll->tracker, MT_DEFAULT_TID);

    TRACE("l1", "ll_lookup: no item in the list matched the key", 0, 0);
    return DOES_NOT_EXIST;
}

map_val_t ll_cas (list_t *ll, map_key_t key, map_val_t expectation, map_val_t new_val) {
    TRACE("l1", "ll_cas: key %p list %p", key, ll);
    TRACE("l1", "ll_cas: expectation %p new value %p", expectation, new_val);
    assert((int64_t)new_val > 0);

    mt_StartOp(ll->tracker, MT_DEFAULT_TID);
    do {
        node_t *pred, *old_item;
        int found = find_pred(&pred, &old_item, ll, key, TRUE);
        if (!found) {
            // There was not an item in the list that matches the key. 
            if (EXPECT_FALSE(expectation != CAS_EXPECT_DOES_NOT_EXIST && expectation != CAS_EXPECT_WHATEVER)) {
                TRACE("l1", "ll_cas: the expectation was not met, the list was not changed", 0, 0);
                mt_EndOp(ll->tracker, MT_DEFAULT_TID);
                return DOES_NOT_EXIST; // failure
            }

            // Create a new item and insert it into the list.
            TRACE("l2", "ll_cas: attempting to insert item between %p and %p", pred, pred->next);
            node_t *new_item = node_alloc(ll, key, new_val);
            markable_t next = new_item->next = (markable_t)old_item;
            markable_t other = ATOMIC_VAR_CAS(&pred->next, (markable_t)next, (markable_t)new_item);
            if (other == next) {
                TRACE("l1", "ll_cas: successfully inserted new item %p", new_item, 0);
                mt_EndOp(ll->tracker, MT_DEFAULT_TID);
                return DOES_NOT_EXIST; // success
            }

            // Lost a race. Failed to insert the new item into the list.
            TRACE("l1", "ll_cas: lost a race. CAS failed. expected pred's link to be %p but found %p", next, other);
            node_free(ll, new_item); // we can use mt_Reclaim here
            continue; // retry
        }

        // Found an item in the list that matches the key.
        map_val_t old_item_val = old_item->val;
        do {
            // If the item's value is DOES_NOT_EXIST it means another thread removed the node out from under us.
            if (EXPECT_FALSE(old_item_val == DOES_NOT_EXIST)) {
                TRACE("l2", "ll_cas: lost a race, found an item but another thread removed it. retry", 0, 0);
                break; // retry
            }

            if (EXPECT_FALSE(expectation == CAS_EXPECT_DOES_NOT_EXIST)) {
                TRACE("l1", "ll_cas: found an item %p in the list that matched the key. the expectation was "
                        "not met, the list was not changed", old_item, old_item_val);
                mt_EndOp(ll->tracker, MT_DEFAULT_TID);
                return old_item_val; // failure
            }

            // Use a CAS and not a SWAP. If the node is in the process of being removed and we used a SWAP, we could
            // replace DOES_NOT_EXIST with our value. Then another thread that is updating the value could think it
            // succeeded and return our value even though we indicated that the node has been removed. If the CAS 
            // fails it means another thread either removed the node or updated its value.
            map_val_t ret_val = ATOMIC_VAR_CAS(&old_item->val, old_item_val, new_val);
            if (ret_val == old_item_val) {
                TRACE("l1", "ll_cas: the CAS succeeded. updated the value of the item", 0, 0);
                mt_EndOp(ll->tracker, MT_DEFAULT_TID);
                return ret_val; // success
            }
            TRACE("l2", "ll_cas: lost a race. the CAS failed. another thread changed the item's value", 0, 0);

            old_item_val = ret_val;
        } while (1);
    } while (1);
}

map_val_t ll_remove (list_t *ll, map_key_t key) {
    TRACE("l1", "ll_remove: removing item with key %p from list %p", key, ll);
    node_t *pred;
    node_t *item;

    mt_StartOp(ll->tracker, MT_DEFAULT_TID);
    int found = find_pred(&pred, &item, ll, key, TRUE);
    if (!found) {
        TRACE("l1", "ll_remove: remove failed, an item with a matching key does not exist in the list", 0, 0);
        mt_EndOp(ll->tracker, MT_DEFAULT_TID);
        return DOES_NOT_EXIST;
    }

    // Mark <item> removed. If multiple threads try to remove the same item only one of them should succeed.
    markable_t next;
    markable_t old_next = item->next;
    do {
        next     = old_next;
        old_next = ATOMIC_VAR_CAS(&item->next, next, MARK_NODE(STRIP_MARK(next)));
        if (HAS_MARK(old_next)) {
            TRACE("l1", "ll_remove: lost a race -- %p is already marked for removal by another thread", item, 0);
            mt_EndOp(ll->tracker, MT_DEFAULT_TID);
            return DOES_NOT_EXIST;
        }
    } while (next != old_next);
    TRACE("l2", "ll_remove: logically removed item %p", item, 0);
    assert(HAS_MARK(VOLATILE_DEREF(item).next));

    // Atomically swap out the item's value in case another thread is updating the item while we are 
    // removing it. This establishes which operation occurs first logically, the update or the remove. 
    map_val_t val = ATOMIC_VAR_XCHG(&item->val, DOES_NOT_EXIST); 
    TRACE("l2", "ll_remove: replaced item's val %p with DOES_NOT_EXIT", val, 0);

    // Unlink <item> from <ll>. If we lose a race to another thread just back off. It is safe to leave the
    // item logically removed for a later call (or some other thread) to physically unlink. By marking the
    // item earlier, we logically removed it. 
    TRACE("l2", "ll_remove: unlink the item by linking its pred %p to its successor %p", pred, next);
    markable_t other;
    if ((other = ATOMIC_VAR_CAS(&pred->next, (markable_t)item, next)) != (markable_t)item) {
        TRACE("l1", "ll_remove: unlink failed; pred's link changed from %p to %p", item, other);
        mt_EndOp(ll->tracker, MT_DEFAULT_TID);
        return val;
    } 

    // The thread that completes the unlink should free the memory.
    node_free(ll, item);
    TRACE("l1", "ll_remove: successfully unlinked item %p from the list", item, 0);
    mt_EndOp(ll->tracker, MT_DEFAULT_TID);
    return val;
}

void ll_print (list_t *ll, int verbose) {
    if (verbose) {
        markable_t next = ll->head->next;
        int i = 0;
        while (next != DOES_NOT_EXIST) {
            node_t *item = STRIP_MARK(next);
            if (item == NULL)
                break;
            printf("%s%p:0x%llx ", HAS_MARK(item->next) ? "*" : "", (void *)item, (long long)item->key);
            fflush(stdout);
            if (i++ > 30) {
                printf("...");
                break;
            }
            next = item->next;
        }
        printf("\n");
    }
    printf("count:%llu\n", (long long)ll_count(ll));
}

ll_iter_t *ll_iter_alloc (list_t *ll) {
    assert(ll);
    ll_iter_t *iter = (ll_iter_t *)malloc(sizeof(ll_iter_t));
    assert(iter);
    iter->ll = ll;

    return iter;
}

void ll_iter_begin (ll_iter_t *iter, map_key_t key) {
    assert(iter);
    list_t *ll = iter->ll;
    assert(ll);
    if (key != DOES_NOT_EXIST) {
        find_pred(&iter->pred, NULL, ll, key, FALSE);
    } else {
        iter->pred = ll->head;
    }
}

map_val_t ll_iter_next (ll_iter_t *iter, map_key_t *key_ptr) {
    assert(iter);
    list_t *ll = iter->ll;
    assert(ll);
    if (iter->pred == NULL)
        return DOES_NOT_EXIST;

    // advance iterator to next item; skip items that have been removed
    markable_t item;

    // haz_t *hp0 = haz_get_static(0);
    void *pred;
    do {

        do {
            item = iter->pred->next;
            // haz_set(hp0, STRIP_MARK(item));
            pred = mt_Read(ll->tracker, MT_DEFAULT_TID, 0, STRIP_MARK(item));
        } while (item != VOLATILE_DEREF(iter->pred).next);

        iter->pred = pred; // STRIP_MARK(item);
        if (iter->pred == NULL)
            return DOES_NOT_EXIST;
    } while (HAS_MARK(item));

    if (key_ptr != NULL) {
        *key_ptr = GET_NODE(item)->key;
    }
    return GET_NODE(item)->val;
}

void ll_iter_free (ll_iter_t *iter) {
    free(iter);
}
