#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include "shmemu.h"
#include "shmemc.h"
#include "state.h"
#include "info.h"
#include "threading.h"
#include "shmem/api.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

void
shmem_get_am_wait(shmem_am_nb_handle_t *handle, int num_handles)
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
shmem_get_am_test(shmem_am_nb_handle_t handle)
{          
    int ret_val = handle->completed;
    if(handle->completed){
        handle->completed = 0;
        ucp_request_free(handle);
    }
    return ret_val;
}

void
shmem_put_am(void *dest, int nelems, size_t elem_size, 
             int pe, shmem_am_handle_t id, void *args, 
             size_t arg_length)
{
    shmemc_put_am(dest, nelems, elem_size, pe, 
                  id, args, arg_length, SHMEM_CTX_DEFAULT);
}   

void
shmem_get_am(void *dest, void *src, 
             int nelems, size_t elem_size, 
             int pe, shmem_am_handle_t id, 
             void *args, size_t arg_length)
{          
    shmem_am_nb_handle_t wait_handle = shmemc_get_am_nb(dest, src, nelems, 
                                                            elem_size, pe, id, 
                                                            args, arg_length, 
                                                            SHMEM_CTX_DEFAULT);
    shmem_get_am_wait(&wait_handle, 1);
}

shmem_am_nb_handle_t
shmem_get_am_nb(void *dest, void *src, 
                int nelems, size_t elem_size, 
                int pe, shmem_am_handle_t id, 
                void *args, size_t arg_length)
{
    return shmemc_get_am_nb(dest, src, nelems, elem_size, 
                            pe, id, args, arg_length, 
                            SHMEM_CTX_DEFAULT);
}

shmem_am_handle_t
shmem_insert_cb(shmem_am_type_t type, shmem_am_cb cb, void *cb_context){
    return shmemc_insert_cb(type, cb, cb_context);
}

void
shmem_fence_am(void)
{
    long put_val = SHMEM_SYNC_VALUE;
    int local_vals[2];// = {proc.sent_ams, proc.received_ams};


    local_vals[0] = proc.am_info.sent_ams;
    local_vals[1] = proc.am_info.received_ams;

    /* if we haven't make our shared space for this reduction */
    /* maybe do this at start up */
    /* Need to free this memory */
    if(!proc.am_info.am_fence.pWrk){
        proc.am_info.am_fence.pWrk = shmem_malloc(sizeof(int) * 
                                          SHMEM_REDUCE_MIN_WRKDATA_SIZE);
        proc.am_info.am_fence.pSync = shmem_malloc(sizeof(long) * 
                                          SHMEM_REDUCE_SYNC_SIZE);

        shmem_long_put(proc.am_info.am_fence.pSync, &put_val, 
                       SHMEM_REDUCE_SYNC_SIZE, proc.rank);
        proc.am_info.am_fence.total_ams = shmem_malloc(2 * sizeof(int));
        proc.am_info.am_fence.local_ams = shmem_malloc(2 * sizeof(int));
    }
    shmem_int_put(proc.am_info.am_fence.local_ams, local_vals, 2, proc.rank);

    shmem_int_sum_to_all(proc.am_info.am_fence.total_ams, 
                         proc.am_info.am_fence.local_ams, 
                         2, 0, 0, proc.nranks, 
                         proc.am_info.am_fence.pWrk, 
                         proc.am_info.am_fence.pSync);
    while(proc.am_info.am_fence.total_ams[0] != proc.am_info.am_fence.total_ams[1]){
        shmemc_progress();
        shmem_int_sum_to_all(proc.am_info.am_fence.total_ams, 
                             proc.am_info.am_fence.local_ams, 
                             2, 0, 0, proc.nranks, 
                             proc.am_info.am_fence.pWrk, 
                             proc.am_info.am_fence.pSync);
        local_vals[0] = proc.am_info.sent_ams;
        local_vals[1] = proc.am_info.received_ams;
        shmem_int_put(proc.am_info.am_fence.local_ams, local_vals,
                      2, proc.rank);
    }
}

