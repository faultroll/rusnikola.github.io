#ifndef LIST_H
#define LIST_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "map.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct ll list_t;
typedef struct ll_iter ll_iter_t;

list_t *   ll_alloc   (const datatype_t *key_type);
map_val_t  ll_cas     (list_t *ll, map_key_t key, map_val_t expected_val, map_val_t new_val);
map_val_t  ll_lookup  (list_t *ll, map_key_t key);
map_val_t  ll_remove  (list_t *ll, map_key_t key);
size_t     ll_count   (list_t *ll);
void       ll_print   (list_t *ll, int verbose);
void       ll_free    (list_t *ll);
map_key_t  ll_min_key (list_t *sl);

ll_iter_t * ll_iter_alloc (list_t *sl);
void        ll_iter_begin (ll_iter_t *iter, map_key_t key);
map_val_t   ll_iter_next  (ll_iter_t *iter, map_key_t *key_ptr);
void        ll_iter_free  (ll_iter_t *iter);

#if defined(__cplusplus)
}
#endif

#endif//LIST_H
