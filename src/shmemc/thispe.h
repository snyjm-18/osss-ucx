/* For license: see LICENSE file at top-level */

#ifndef _THISPE_H
#define _THISPE_H 1

#include "boolean.h"
#include "threading.h"
#include "shmemc-ucx.h"

#include <sys/types.h>
#include <ucp/api/ucp.h>
#include <poll.h>
#include <pthread.h>

#define MAX_CBS 10

/*
 * -- for UCX --------------------------------------------------------
 */

/*
 * exchanged at start-up
 */
typedef struct worker_info {
    ucp_address_t *addr;        /* worker address */
    char *buf;                  /* allocated to copy remote worker */
    size_t len;                 /* size of worker */
} worker_info_t;

/*
 * Encapsulate what UCX needs for remote access to a memory region
 */
typedef struct mem_region_access {
    ucp_mem_h mh;               /* memory handle */
    ucp_rkey_h rkey;            /* remote key for this heap */
} mem_region_access_t;

/*
 * each PE has a number of memory regions, which need the following
 * info:
 */
typedef struct mem_info {
    size_t id;                  /* number of this heap */
    uint64_t base;              /* start of this heap */
    uint64_t end;               /* end of this heap */
    size_t len;                 /* its size (b) */
    mem_region_access_t racc;   /* for remote access */
} mem_info_t;

/*
 * for PE exchange
 */
typedef struct mem_region {
    mem_info_t *minfo;          /* nranks mem info */
} mem_region_t;

/*
 * *Internal* OpenSMHEM context management handle
 *
 * NB difference between UCX context, and OpenSHMEM context.  Each
 * OpenSHMEM context requires...
 */

/*
 * context attributes, see OpenSMHEM 1.4 spec, sec. 9.4.1, pp. 30-31
 */
typedef struct shmemc_context_attr {
    bool serialized;
    bool private;
    bool nostore;
} shmemc_context_attr_t;

typedef struct shmemc_context {
    ucp_worker_h w;             /* for separate context progress */

    unsigned long id;           /* internal tracking */

    shmemc_thread_t creator_thread; /* thread ID that created me */

    /*
     * parsed options during creation (defaults: no)
     */
    shmemc_context_attr_t attr;

    /*
     * possibly other things
     */
} shmemc_context_t;

typedef shmemc_context_t *shmemc_context_h;

/*
 * this comms-layer needs to know...
 */
typedef struct comms_info {
    ucp_context_h ucx_ctxt;     /* local communication context */
    ucp_config_t *ucx_cfg;      /* local config */
    ucp_ep_h *eps;              /* nranks endpoints (1 of which is mine) */
    uct_ep_h *am_eps;           /* nranks for uct endpoints for active message */

    worker_info_t *xchg_wrkr_info; /* nranks worker info exchanged */

    shmemc_context_h *ctxts;    /* PE's contexts */
    size_t nctxts;              /* how many contexts */

    mem_region_t *regions;      /* exchanged symmetric regions */
    size_t nregions;            /* how many regions */
} comms_info_t;

typedef struct thread_desc {
    ucs_thread_mode_t ucx_tl;   /* UCX thread level */
    int osh_tl;                 /* corresponding OpenSHMEM thread level */
    shmemc_thread_t invoking_thread; /* thread that called shmem_init*() */
} thread_desc_t;

/*
 * -- General --------------------------------------------------------
 */

typedef enum shmemc_status {
    SHMEMC_PE_SHUTDOWN = 0,
    SHMEMC_PE_RUNNING,
    SHMEMC_PE_FAILED,
    SHMEMC_PE_UNKNOWN
} shmemc_status_t;

/*
 * which collectives we're using.  might want to move this per-team at
 * some point so we can optimize through locality-awareness
 */
typedef enum shmemc_coll {
    SHMEMC_COLL_LINEAR = 0,
    SHMEMC_COLL_TREE,
    SHMEMC_COLL_DISSEM,
    SHMEMC_COLL_BINOMIAL,
    SHMEMC_COLL_BINOMIAL2,
    SHMEMC_COLL_UNKNOWN
} shmemc_coll_t;

#define SHMEMC_COLL_DEFAULT SHMEMC_COLL_TREE

typedef struct heapinfo {
    size_t nheaps;              /* how many heaps requested */
    size_t *heapsize;           /* array of their sizes */
} heapinfo_t;

/*
 * implementations support some environment variables
 */
typedef struct env_info {
    /*
     * required
     */
    bool print_version;         /* produce info output? */
    bool print_info;
    heapinfo_t heaps;
    bool debug;                 /* are we doing sanity debugging? */

    /*
     * this implementation
     */
    bool logging;            /* turn on message logging? */
    char *logging_file;      /* where does logging output go? */
    char *logging_events;    /* show only these types of messages */
    bool xpmem_kludge;       /* protect against UCX bug temporarily */
    shmemc_coll_t barrier_algo;
    shmemc_coll_t broadcast_algo;
} env_info_t;

/*
 * PEs can belong to teams
 */
typedef struct shmemc_team {
    unsigned long id;           /* team ID */
    int *members;               /* list of PEs in the team */
    size_t nmembers;
} shmemc_team_t;

typedef struct shmemc_am_fence_mem{
    int *pWrk;
    long *pSync;
    int *total_ams;
    int *local_ams;
} shmemc_am_fence_mem_t;

/* we need a global AM data structure and a active_message file TODO */
typedef unsigned shmem_am_handle_t;
typedef void (*shmem_am_func)(void *elem, void *args, int elem_index, void *cb_context);

typedef struct am_info{
    int sent_ams;               /* how many ams I have sent */
    int received_ams;           /* how many ams I have received */
    shmem_am_func put_cbs[MAX_CBS]; /* user callbacks for put active messages */
    shmem_am_func get_cbs[MAX_CBS];
    void *        put_contexts[MAX_CBS];
    void *        get_contexts[MAX_CBS];
    shmem_am_handle_t next_put_am_index;
    shmem_am_handle_t next_get_am_index;
    shmemc_am_fence_mem_t am_fence;  /* symetric memory regions for am fence ops */
    struct pollfd am_fd;        /* fd for ifaces and ams */
    pthread_t am_tid;
    size_t req_size;
    int sent_am_replys;
    int recv_am_replys;
    uint64_t next_get_tag; /*next get tag for active messages */
} am_info_t;

/*
 * each PE has this state info
 */
typedef struct thispe_info {
    comms_info_t comms;         /* per-comms layer info */
    env_info_t env;             /* environment vars */
    thread_desc_t td;           /* threading model invoked */
    int rank;                   /* rank info */
    int nranks;                 /* how many ranks */
    shmemc_status_t status;     /* up, down, out to lunch etc */
    int refcount;               /* library initialization count */
    int *peers;                 /* peer PEs in a node group */
    int npeers;                 /* how many peers */
    shmemc_team_t *teams;       /* PE teams we belong to */
    size_t nteams;              /* how many teams */
    am_info_t am_info;          /* all data needed for ams */
} thispe_info_t;

#endif /* ! _THISPE_H */
