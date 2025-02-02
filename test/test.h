typedef int atomic_t;
typedef long atomic_long_t;
typedef long atomic64_t;

atomic_t elearn;
typedef struct refcount_struct {
    atomic_t refs;
} refcount_t;
struct kref {
    refcount_t refcount;
};
struct krefkref {
    atomic_t two;
};

void wow12() {
    atomic_t ll;
}