#ifndef DATATYPE_H
#define DATATYPE_H

#include <stddef.h> // size_t
#include <stdint.h> // uint32_t

#if defined(__cplusplus)
extern "C" {
#endif

typedef int       (*cmp_fun_t)   (const void *, const void *);
typedef uint32_t  (*hash_fun_t)  (const void *); // U(nique)ID returns map_key_t
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

#if defined(__cplusplus)
}
#endif

#endif//DATATYPE_H
