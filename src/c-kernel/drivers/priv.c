#include "priv.h"

heap_allocator_t* g_driver_alloca;

void initialize_driver_manager(heap_allocator_t* alloca)
{
    g_driver_alloca = alloca;
}

heap_allocator_t *get_driver_allocator()
{
    return g_driver_alloca;
}
