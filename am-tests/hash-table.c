#include <stdio.h>
#include <shmem.h>
#include <stdlib.h>
#include <unistd.h>
#include "uthash.h"

typedef struct ht_s{
  unsigned long key;
  void *value;
  UT_hash_handle hh;
} ht_t;

ht_t *ht;
size_t elem_size = sizeof(unsigned long);

typedef struct am_arg{
    unsigned long key;
    unsigned long val;
} am_arg_t;

void func_put(void *item, void *args, int index){
  am_arg_t *entry = args;
  ht_t *instance;
  HASH_FIND(hh, ht, &entry->key, sizeof(unsigned long), instance);
  if(!instance){
      instance = calloc(1, sizeof(ht_t));
      instance->key = entry->key;
      instance->value = malloc(2 * sizeof(unsigned long));
      HASH_ADD(hh, ht, key, sizeof(unsigned long), instance);
  }
  memcpy(instance->value, args, 2 * sizeof(unsigned long));
}

void func_get(void *item, void *args, int index){
  unsigned long key = *(unsigned long *)args;
  unsigned long val;
  ht_t *instance;
  HASH_FIND(hh, ht, &key, sizeof(unsigned long), instance);
  
  val = *(unsigned long *)(instance->value + sizeof(unsigned long));
  
  unsigned long *ret = item;
  *ret = val;
}

int main(int argc, char *argv[]){
  
  shmem_init();
	
  shmem_init_am();
  
  int me = shmem_my_pe();
  int size = shmem_n_pes();
  unsigned long *new = shmem_malloc(sizeof(unsigned long) * 2);
  int target = (me + 1) % size;

  unsigned long num[2] = {0, 0};
  unsigned long dest;
  am_arg_t entry;

  entry.key = 12345;
  entry.val = me;

  shmem_ulong_put(new, num, 2, me);
  
  shmem_am_handle_t h1;
  shmem_am_handle_t h2;

  h1 = shmem_insert_cb(SHMEM_AM_PUT, func_put);
  h2 = shmem_insert_cb(SHMEM_AM_GET, func_get);
  
  shmem_fence();
  shmem_barrier_all();

  shmem_put_am(new, 1, sizeof(int), me, h1, &entry, sizeof(am_arg_t));
  
  shmem_fence_am();
  
  shmem_get_am(&dest, new, 1, sizeof(unsigned long), target, h2, &(entry.key), sizeof(unsigned long));
  
  printf("got : %lu rank : %d \n", dest, me);
  
  shmem_finalize();	
  return 0;
}
