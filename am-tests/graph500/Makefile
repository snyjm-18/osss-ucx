TARGET=bfs_oshmem bfs_oshmem-mailbox bfs_oshmem-single-mailbox bfs_oshmem-single-mailbox-omp bfs_oshmem-single-mailbox-replicated bfs_oshmem-single-mailbox-strided bfs_oshmem-single-mailbox-concurrent bfs_oshmem-single-mailbox-hiper bfs_oshmem-single-mailbox-concurrent-crc bfs_oshmem-bitvector bfs_oshmem-single-mailbox-concurrent-crc-hiper

FLAGS=-ffast-math -O3 -g

# For Cray SHMEM
# OSHMEM_LD_FLAGS=-L$(OPENSHMEM_INSTALL)/lib64 -lsma

OPENSHMEM_INSTALL=

# For OpenMPI SHMEM
#OSHMEM_LD_FLAGS=-L$(OPENSHMEM_INSTALL)/lib64 -L$(OPENSHMEM_INSTALL)/lib -lsma
#SOS_LD_FLAGS=-L$(OPENSHMEM_INSTALL)/lib64 -L$(OPENSHMEM_INSTALL)/lib -lsma


graph500: graph500.c utilities.c mrg.c splittable_mrg.c generator.c
	oshcc  $(FLAGS) -I$(OPENSHMEM_INSTALL)/include $(OSHMEM_LD_FLAGS) -lrt -o $@ $^ -DGENERATOR_USE_PACKED_EDGE_TYPE

clean:
	rm -f $(TARGET)
