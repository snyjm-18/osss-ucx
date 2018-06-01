#include <stdio.h>
#include <shmem.h>
#include <stdlib.h>
#include <unistd.h>

void func_put(void *item, void *args, int index){
  int *old_nums = item;
  int *new_nums = args;
  *old_nums = *old_nums * new_nums[index];
}
void func_get(void *item, void *args, int index){
  int *old_num = item;
  int *new_num = args;
  *old_num = *old_num * new_num[index];
}
int main(int argc, char *argv[]){
  int got;
  
  //shmem_init_thread(SHMEM_THREAD_MULTIPLE, &got);
	shmem_init();
  shmem_init_am();
  
  shmem_get_am_nb_handle_t get_handle;
  int me = shmem_my_pe();
  int size = shmem_n_pes();
  int target = (me + 1) % size;
  int *new = shmem_malloc(sizeof(int) * 2);
  
  int num[2] = {me + 1, me + 1};
  int args[2] = {2, 4};
  int put_args[2] = {-1, 5};

  shmem_int_put(new, num, 2, me);
 

  shmem_am_handle_t h1, h2;

  h1 = shmem_insert_cb(SHMEM_AM_GET, func_get);
  h2 = shmem_insert_cb(SHMEM_AM_PUT, func_put);

  shmem_fence();
  shmem_barrier_all();

  shmem_put_am(new, 2, sizeof(int), target, h2, put_args, 2 * sizeof(int));

  shmem_barrier_all();
  
  shmem_fence_am();

  get_handle = shmem_get_am_nb(args, new, 2, sizeof(int), target, h1, args, 2 * sizeof(int));
  
  if(shmem_get_am_test(get_handle)){
    printf("finished : num[0] : %d num[1] : %d rank : %d \n", args[0], args[1], me);
  }
  else{
    printf("unfinished : num[0] : %d num[1] : %d rank : %d \n", args[0], args[1], me);
    shmem_get_am_wait(get_handle);
    printf("finished now : num[0] : %d num[1] : %d rank : %d \n", args[0], args[1], me);
  }


  shmem_fence_am();
  shmem_barrier_all();
  
  printf("num[0] : %d num[1] : %d rank : %d \n", args[0], args[1], me);

  shmem_fence();
  shmem_barrier_all();
  
  shmem_finalize();	
  return 0;
}
