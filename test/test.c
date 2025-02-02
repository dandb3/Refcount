#include "test.h"
typedef int refcount_t;

atomic_t wow1;

struct wow {
#ifdef WOW
    refcount_t elearn;
#endif
};

void atomic_set(atomic_t *v, int i) {
    *v = i;
}

void atomic_add(int i, atomic_t *v) {
    *v += i;
}

void atomic_sub(int i, atomic_t *v) {
    *v -= i;
}

void atomic_inc(atomic_t *v) {
    (*v)++;
}

void atomic_dec(atomic_t *v) {
    (*v)--;
}

struct krefkref test;

void func() {
    atomic_t wow;
    int refcount;
    struct kref good;
    atomic_t ref;

    atomic_set(&test.two, 1);
    atomic_add(-2, &test.two);
    atomic_sub(-2, &test.two);
    atomic_inc(&test.two);
    atomic_dec(&test.two);
}