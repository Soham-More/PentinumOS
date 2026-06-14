#include "ksch.hpp"

#define MAX_KERNEL_THREADS 8

namespace ksch
{
    // thread information block
    struct TIB
    {
        u16 priority;
        thread_state_t state;
        x86::registers_t regs;
    };

    // a priority queue of kernel threads
    TIB tasks[MAX_KERNEL_THREADS];

    // a test kernel thread:
    void test_kmain() {
        ;
    }
}


