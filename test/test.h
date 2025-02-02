typedef int atomic_t;
typedef long atomic_long_t;
typedef long atomic64_t;

atomic_t elearn;
struct kref {
    atomic_t one;
};
struct refcount_t {
    atomic64_t two;
};
struct krefkref {
    atomic64_t two;
};

void wow12() {
    atomic_t ll;
}