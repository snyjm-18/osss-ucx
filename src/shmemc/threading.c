/* For license: see LICENSE file at top-level */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include "threading.h"

int
shmemc_mutex_init(shmemc_mutex_t *tp)
{
    return pthread_mutex_init(tp, NULL);
}

int
shmemc_mutex_destroy(shmemc_mutex_t *mp)
{
    return pthread_mutex_destroy(mp);
}

int
shmemc_mutex_lock(shmemc_mutex_t *mp)
{
    return pthread_mutex_lock(mp);
}

int
shmemc_mutex_unlock(shmemc_mutex_t *mp)
{
    return pthread_mutex_unlock(mp);
}

int
shmemc_mutex_trylock(shmemc_mutex_t *mp)
{
    return pthread_mutex_trylock(mp);
}

unsigned long
shmemc_thread_id(void)
{
    return (unsigned long) pthread_self();
}

int
shmemc_thread_equal(shmemc_thread_t t1, shmemc_thread_t t2)
{
    return pthread_equal(t1, t2);
}
