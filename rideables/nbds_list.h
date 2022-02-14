#ifndef LIST_H
#define LIST_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

// map/common
typedef intptr_t map_key_t;
typedef intptr_t map_val_t;
#define CAS_EXPECT_DOES_NOT_EXIST ( 0)
#define CAS_EXPECT_EXISTS         (-1)
#define CAS_EXPECT_WHATEVER       (-2)
#define EXPECT_TRUE(x)      __builtin_expect(!!(x), 1)
#define EXPECT_FALSE(x)     __builtin_expect(!!(x), 0)
typedef size_t markable_t;
#define TAG_VALUE(v, tag) ((v) |  tag)
#define IS_TAGGED(v, tag) ((v) &  tag)
#define STRIP_TAG(v, tag) ((v) & ~tag)
#define DOES_NOT_EXIST  ((intptr_t)NULL)
#define VOLATILE_DEREF(x) (*((volatile __typeof__(x))(x)))
#define TRACE(flag, format, v1, v2) do { } while (0)
#define TRUE  true
#define FALSE false
// datatype
typedef int       (*cmp_fun_t)   (const void *, const void *);
typedef map_key_t  (*hash_fun_t)  (const void *); // U(nique)ID
typedef struct {
    // if we use hash_fun_t, the compare function is just integer compare
    // otherwise, we need it size for storage and compare it using cmp_fun_t
    // union {
        struct {
            size_t size;
            cmp_fun_t cmp;
        };
        hash_fun_t  hash;
    // };
} datatype_t;

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
