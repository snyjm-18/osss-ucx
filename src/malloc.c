#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmem/defs.h"
#include "shmemc.h"
#include "shmemu.h"

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>

int malloc_error = SHMEM_MALLOC_OK;

#ifdef ENABLE_PSHMEM
#pragma weak shmem_malloc = pshmem_malloc
#define shmem_malloc pshmem_malloc
#pragma weak shmem_free = pshmem_free
#define shmem_free pshmem_free
#pragma weak shmem_realloc = pshmem_realloc
#define shmem_realloc pshmem_realloc
#pragma weak shmem_align = pshmem_align
#define shmem_align pshmem_align
#endif /* ENABLE_PSHMEM */

void *
shmem_malloc(size_t s)
{
    return shmemc_mem_alloc(s);
}

void
shmem_free(void *p)
{
    shmemc_mem_free(p);
}

void *
shmem_realloc(void *p, size_t s)
{
    return shmemc_mem_realloc(p, s);
}

void *
shmem_align(size_t a, size_t s)
{
    return shmemc_mem_align(a, s);
}

/*
 * layer these through top-level API in case we want to insert error
 * checking above
 */

void *
shmalloc(size_t s)
{
    deprecate(__func__);
    return shmem_malloc(s);
}

void
shfree(void *p)
{
    deprecate(__func__);
    shmem_free(p);
}

void *
shrealloc(void *p, size_t s)
{
    deprecate(__func__);
    return shmem_realloc(p, s);
}

void *
shmemalign(size_t a, size_t s)
{
    deprecate(__func__);
    return shmem_align(a, s);
}
