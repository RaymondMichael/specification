#include <shmem.h>
#include <stdlib.h>

#define N 100

int main(void)
{
  int total_sum = 0;

  shmem_init();
  int mype = shmem_my_pe();
  int npes = shmem_n_pes();

  int *my_data = malloc(N * sizeof(int));
  int *all_data = shmem_malloc(N * npes * sizeof(int));

  int *flags = shmem_calloc(npes, sizeof(int));
  size_t *indices = malloc(npes * sizeof(size_t));
  int *status = calloc(npes, sizeof(int));

  for (int i = 0; i < N; i++)
      my_data[i] = mype*N + i;

  for (int i = 0; i < npes; i++)
      shmem_put_nbi(&all_data[mype*N], my_data, N, i);

  shmem_fence();

  for (int i = 0; i < npes; i++)
      shmem_atomic_set(&flags[mype], 1, i);

  size_t ncompleted;
  while ((ncompleted = shmem_wait_until_some(flags, npes, indices,
                                             status, SHMEM_CMP_NE, 0))) {
      for (size_t i = 0; i < ncompleted; i++) {
          for (size_t j = 0; j < N; j++) {
              total_sum += all_data[indices[i]*N + j];
          }
          status[indices[i]] = 1;
      }
  }

  /* check the result */
  int M = N * npes - 1;
  if (total_sum != M * (M + 1) / 2) {
      shmem_global_exit(1);
  }

  shmem_finalize();
  return 0;
}
