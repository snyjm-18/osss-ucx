/* For license: see LICENSE file at top-level */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmemu.h"
#include "shmemc.h"
#include "state.h"
#include "info.h"
#include "threading.h"

#ifdef ENABLE_EXPERIMENTAL
#include "allocator/xmemalloc.h"
#endif  /* ENABLE_EXPERIMENTAL */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef ENABLE_PSHMEM
#pragma weak shmem_init_thread = pshmem_init_thread
#define shmem_init_thread pshmem_init_thread
#pragma weak shmem_query_thread = pshmem_query_thread
#define shmem_query_thread pshmem_query_thread
#pragma weak shmem_init = pshmem_init
#define shmem_init pshmem_init
#pragma weak shmem_finalize = pshmem_finalize
#define shmem_finalize pshmem_finalize
#endif /* ENABLE_PSHMEM */

/*
 * finish SHMEM portion of program, release resources
 */

static void
finalize_helper(void)
{
    /* do nothing if multiple finalizes */
    if (proc.refcount > 0) {
        const shmemc_thread_t this = shmemc_thread_id();

        if (this != proc.td.invoking_thread) {
            logger(LOG_FINALIZE,
                   "mis-match: thread %lu initialized, but %lu finalized",
                   proc.td.invoking_thread, this);
        }

#ifdef ENABLE_EXPERIMENTAL
        shmemxa_finalize();
#endif  /* ENABLE_EXPERIMENTAL */
        shmemu_finalize();
        shmemc_finalize();

        proc.refcount = 0;      /* finalized is finalized */
        proc.status = SHMEMC_PE_SHUTDOWN;
    }
}

inline static int
init_thread_helper(int requested, int *provided)
{
    /* do nothing if multiple inits */
    if (proc.refcount == 0) {
        int s;

        shmemc_init();
        shmemu_init();
#ifdef ENABLE_EXPERIMENTAL
        shmemxa_init(proc.env.heaps.nheaps);
#endif  /* ENABLE_EXPERIMENTAL */

        s = atexit(finalize_helper);
        if (s != 0) {
            logger(LOG_FATAL,
                   "unable to register atexit() handler: %s",
                   strerror(errno)
                   );
            /* NOT REACHED */
        }

        proc.status = SHMEMC_PE_RUNNING;

        /* for now */
        switch(requested) {
        case SHMEM_THREAD_SINGLE:
            break;
        case SHMEM_THREAD_FUNNELED:
            break;
        case SHMEM_THREAD_SERIALIZED:
            break;
        case SHMEM_THREAD_MULTIPLE:
            break;
        default:
            logger(LOG_FATAL,
                   "unknown thread level %d requested",
                   requested
                   );
            /* NOT REACHED */
            break;
        }

        /* save and return */
        proc.td.osh_tl = requested;
        if (provided != NULL) {
            *provided = proc.td.osh_tl;
        }

        proc.td.invoking_thread = shmemc_thread_id();

        if (proc.rank == 0) {
            if (proc.env.print_version) {
                info_output_package_version(stdout, 0);
            }
            if (proc.env.print_info) {
                shmemc_print_env_vars(stdout, "# ");
            }
        }
    }
    proc.refcount += 1;

    logger(LOG_INIT,
           "%s(requested=%d, provided->%d)",
           __func__,
           requested, proc.td.osh_tl
           );

    /* just declare success */
    return 0;
}

/*
 * -- API --------------------------------------------------------------------
 */

/*
 * initialize SHMEM portion of program with threading model
 */

int
shmem_init_thread(int requested, int *provided)
{
    return init_thread_helper(requested, provided);
}

void
shmem_init(void)
{
    (void) init_thread_helper(SHMEM_THREAD_SINGLE, NULL);
}

/* AM Stuff
 *
 *
 */
void
shmem_init_am()
{
    proc.am_info.am_fence.pWrk = NULL; /* so we know to make memory regions */
    shmemc_init_am();
}

void
shmem_get_am_wait(shmem_get_am_nb_handle_t *handle, int num_handles)
{
    for(int i = 0; i < num_handles; i++){
        while(!(handle[i]->completed)){
            shmemc_ctx_progress(SHMEM_CTX_DEFAULT);
        }
        handle[i]->completed = 0;
        ucp_request_free(handle[i]);
    }
}

int
shmem_get_am_test(shmem_get_am_nb_handle_t handle)
{
    int ret_val = handle->completed;
    //TODO If we call this on a handle that has been freed, it hangs
    if(handle->completed){
        ucp_request_free(handle);
    }
    return ret_val;
}

void 
shmem_put_am(void *dest, int nelems, size_t elem_size, int pe, shmem_am_handle_t id, void *args, size_t arg_length)
{
    shmemc_put_am(dest, nelems, elem_size, pe, id, args, arg_length, SHMEM_CTX_DEFAULT);
}

void
shmem_get_am(void *dest, void *src, int nelems, size_t elem_size, int pe, shmem_am_handle_t id, void *args, size_t arg_length)
{
    shmem_get_am_nb_handle_t wait_handle = shmemc_get_am_nb(dest, src, nelems, elem_size, pe, id, args, arg_length, SHMEM_CTX_DEFAULT);
    shmem_get_am_wait(&wait_handle, 1);
}

shmem_get_am_nb_handle_t
shmem_get_am_nb(void *dest, void *src, int nelems, size_t elem_size, int pe, shmem_am_handle_t id, void *args, size_t arg_length)
{
    return shmemc_get_am_nb(dest, src, nelems, elem_size, pe, id, args, arg_length, SHMEM_CTX_DEFAULT);
}

shmem_am_handle_t
shmem_insert_cb(shmem_am_type_t type, shmem_am_cb cb){
    return shmemc_insert_cb(type, cb);
}

/*
 * finish SHMEM portion of program, release resources
 */

void
shmem_finalize(void)
{
    SHMEMU_CHECK_INIT();

    logger(LOG_FINALIZE,
           "%s()",
           __func__
           );

    finalize_helper();
}

/*
 * query thread level support
 *
 * TODO: should this be here or in pe-query.c ?
 */

void
shmem_query_thread(int *provided)
{
    SHMEMU_CHECK_INIT();

    logger(LOG_FINALIZE,
           "%s() -> %d",
           __func__,
           proc.td.osh_tl
           );

    *provided = proc.td.osh_tl;
}

/*
 * deprecated
 */

#ifdef ENABLE_PSHMEM
#pragma weak start_pes = pstart_pes
#define start_pes pstart_pes
#endif /* ENABLE_PSHMEM */

void
start_pes(int n)
{
    NO_WARN_UNUSED(n);

    deprecate(__func__, 1, 2);

    shmem_init();
}
