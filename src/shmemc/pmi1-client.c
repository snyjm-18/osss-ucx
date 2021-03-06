/* For license: see LICENSE file at top-level */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include "thispe.h"
#include "shmemu.h"
#include "state.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <pmi.h>

#include <ucp/api/ucp.h>

static char *kvs_name = NULL;   /* PMI namespace */
static int kvs_max_len;

inline static char *
alloc_string(void)
{
    return (char *) malloc(sizeof(*buf) * kvs_max_len);
}

#ifndef ENABLE_ALIGNED_ADDRESSES

/*
 * formats are <pe>:<region-index>:region:<key>
 */
static const char *region_base_fmt = "%d:%d:mr:base";
static const char *region_size_fmt = "%d:%d:mr:size";

void
shmemc_pmi_publish_heap_info(void)
{
}

void
shmemc_pmi_exchange_heap_info(void)
{
}

#endif /* ! ENABLE_ALIGNED_ADDRESSES */

static const char *wrkr_exch_fmt = "%d:wrkr:addr";

/*
 * PMI key/val lookups
 */
static char *key;
static char *val;

void
shmemc_pmi_publish_worker(void)
{
    int next;

    /* everyone publishes their info */
    snprintf(key, kvs_max_len,
             wrkr_exch_fmt,
             proc.rank);
    next = snprint(val, "%8lu", proc.comms.xchg_wrkr_info[proc.rank].len);
    memcpy(val + next,
           proc.comms.xchg_wrkr_info[proc.rank].addr,
           proc.comms.xchg_wrkr_info[proc.rank].len);

    ps = PMI_KVS_Put(kvs_name, key, val);
    shmemu_assert(ps == PMI_SUCCESS,
                  "put of worker blob failed (status %d)",
                  ps);

    ps = PMI_KVS_Commit(kvs_name);
    shmemu_assert(ps == PMI_SUCCESS,
                  "worker blob commit failed (status %d)",
                  ps);
}

void
shmemc_pmi_exchange_workers(void)
{
    int pe;
    unsigned long len;

    for (pe = 0; pe < proc.nranks; pe += 1) {
        const int i = (pe + proc.rank) % proc.nranks;

        snprintf(key, kvx_max_len,
                 "%d:wrkr:addr",
                 i);

        ps = PMI_KVS_Get(kvs_name, key, val, kvs_max_len);
        shmemu_assert(ps == PMI_SUCCESS,
                      "get of remote worker blob failed (status %d)",
                      ps);

        sscanf(val, "%8lu", &len);

        proc.comms.xchg_wrkr_info[i].buf = (char *) malloc(len);
        shmemu_assert(proc.comms.xchg_wrkr_info[i].buf != NULL,
                      "can't allocate memory for remote worker blob");
        memcpy(proc.comms.xchg_wrkr_info[i].buf, val + 8, len);
    }
}

/*
 * PE:rkey:HEAPNO
 */
static const char *rkey_exch_fmt = "%d:rkey:%d";

void
shmemc_pmi_publish_my_rkeys(void)
{
}

void
shmemc_pmi_exchange_all_rkeys(void)
{
}

/* -------------------------------------------------------------- */

/*
 * this barrier is purely for internal use with PMI, nothing to do
 * with SHMEM/UCX
 */
void
shmemc_pmi_barrier_all(void)
{
    const int ps = PMI_Barrier();

    shmemu_assert(ps == PMI_SUCCESS,
                  "PMI barrier failed (status %d)",
                  ps);
}

/*
 * find my rank, program size, peer PEs
 */

void
shmemc_pmi_client_init(void)
{
    int ps;
    int spawned;

    ps = PMI_Init(&spawned);
    shmemu_assert(ps == PMI_SUCCESS,
                  "can't initialize PMI (status %d)",
                  ps);

    ps = PMI_Get_rank(&proc.rank);
    shmemu_assert(ps == PMI_SUCCESS,
                  "PMI can't get PE rank (status %d)",
                  ps);

    shmemu_assert(proc.rank >= 0,
                  "PMI PE rank is not valid (status %d)",
                  ps);

    ps = PMI_Get_size(&proc.nranks);
    shmemu_assert(ps == PMI_SUCCESS,
                  "PMI can't get program size (status %d)",
                  ps);

    /* is the world a sane size? */
    shmemu_assert(proc.nranks > 0,
                  "PMI count of PE ranks %d is not valid",
                  proc.nranks);
    shmemu_assert(IS_VALID_PE_NUMBER(proc.rank),
                  "PMI PE rank %d is not valid",
                  proc.rank);

    /* I have no peer info */
    proc.npeers = 0;
    proc.peers = NULL;

    ps = PMI_KVS_Get_key_length_max(&kvs_max_len);
    shmemu_assert(ps == PMI_SUCCESS,
                  "PMI can't get max key length (status %d)",
                  ps);

    kvs_name = alloc_string();
    shmemu_assert(kvs_name != NULL, "PMI can't allocate name string");

    ps = PMI_KVS_Get_my_name(kvs_name, kvs_max_len);
    shmemu_assert(ps == PMI_SUCCESS,
                  "PMI can't find its name (status %d)",
                  ps);

    key = alloc_string();
    shmemu_assert(key != NULL,
                  "PMI can't allocate memory for key holder");
    val = alloc_string();
    shmemu_assert(val != NULL,
                  "PMI can't allocate memory for value holder");
}

void
shmemc_pmi_client_finalize(void)
{
    pmix_status_t ps;
    int pe;

    ps = PMI_Finalize();
    shmemu_assert(ps == PMI_SUCCESS,
                  "PMI can't finalize (status %d)",
                  ps);

    /* TODO: PMI client common code below */

    for (pe = 0; pe < proc.nranks; pe += 1) {
        size_t r;

        /* clean up allocations for exchanged buffers */
        free(proc.comms.xchg_wrkr_info[pe].buf);
        for (r = 0; r < proc.comms.nregions; r += 1) {
            ucp_rkey_destroy(proc.comms.regions[r].minfo[pe].racc.rkey);
        }
    }

    /* clean up allocated memory */
    free(proc.peers);
    free(kvs_name);
    free(key);
    free(val);
}

void
shmemc_pmi_client_abort(const char *msg, int status)
{
    pmix_status_t ps;

    ps = PMI_Abort(status, msg);
    shmemu_assert(ps == PMI_SUCCESS,
                  "PMI can't abort (status %d)",
                  ps);
}
