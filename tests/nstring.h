#ifndef NSTRING_H
#define NSTRING_H

#include <stddef.h> // size_t
#include <stdio.h>
#include <stdlib.h> // malloc
#include <string.h> // strxxx
#include <assert.h> // assert
#include "murmur.h"
#include "nbds/datatype.h"

typedef struct {
    size_t len;
    char data[];
} nstring_t;

// currently not support variable length, only give max size
#define NS_MAX_LEN 1024
#define ns_len (sizeof(nstring_t) + NS_MAX_LEN)
// static size_t ns_len (const nstring_t *ns) {
//     return (sizeof(nstring_t) + ns->len);
// }

static int ns_cmp (const nstring_t *ns1, const nstring_t *ns2) {
    assert(ns1 != NULL);
    assert(ns2 != NULL);
    // printf("(%s): ns1(%zu, %s), ns2(%zu, %s)\n", __func__, ns1->len, ns1->data, ns2->len, ns2->data);
    // int d = memcmp(ns1->data, ns2->data, (ns1->len < ns2->len) ? ns1->len : ns2->len);
    int d = ns1->len - ns2->len;
    return (d == 0) ? strcmp(ns1->data, ns2->data) : d;
}

static uint32_t ns_hash (const nstring_t *ns) {
    return murmur32(ns->data, ns->len);
}

// static nstring_t *ns_dup (const nstring_t *ns1) {
//     nstring_t *ns2 = ns_alloc(ns1->len);
//     memcpy(ns2->data, ns1->data, ns1->len);
//     return ns2;
// }

// static void ns_clone (nstring_t *dst, const nstring_t *src) {
//     assert(dst != NULL);
//     assert(src != NULL);
//     dst->len = src->len;
//     snprintf(dst->data, dst->len, "%s", src->data);
//     // memcpy(dst->data, src->data, dst->len);
//     // dst->data[dst->len - 1] = '\0';
// }

static inline
nstring_t *ns_alloc (size_t len) {
    assert(len <= NS_MAX_LEN);
    // map will cpy all data, so we need to alloc enough length
    nstring_t *ns = malloc(/* sizeof(nstring_t) + len */ns_len);
    ns->len = len;
    ns->data[ns->len - 1] = '\0';
    return ns;
}

static inline
void ns_free (nstring_t *ns) {
    assert(ns != NULL);
    free(ns);
}

const datatype_t DATATYPE_NSTRING = { { (size_t)ns_len, (cmp_fun_t)ns_cmp, }, (hash_fun_t)ns_hash };

#endif//NSTRING_H 
