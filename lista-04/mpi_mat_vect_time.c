/* File:     mpi_mat_vect_time.c
 *
 * Purpose:  Implement parallel matrix-vector multiplication using
 *           one-dimensional arrays to store the vectors and the
 *           matrix.  Vectors use block distributions and the
 *           matrix is distributed by block rows.  This version
 *           generates a random matrix A and a random vector x.
 *           It prints out the run-time.
 *
 * Compile:  mpicc -g -Wall -o mpi_mat_vect_time mpi_mat_vect_time.c
 * Run:      mpiexec -n <number of processes> ./mpi_mat_vect_time
 *
 * Input:    Dimensions of the matrix (m = number of rows, n
 *              = number of columns)
 * Output:   Elapsed time for execution of the multiplication
 *
 * Notes:
 *    1. Number of processes should evenly divide both m and n
 *    2. Define DEBUG for verbose output, including the product
 *       vector y
 *
 * IPP:  Section 3.6.2 (pp. 122 and ff.)
 */
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

void Check_for_error(int local_ok, char fname[], char message[], MPI_Comm comm);
void Allocate_arrays(double** local_A_pp, double** local_x_pp,
      double** local_y_pp, int local_m, int n, int local_n,
      MPI_Comm comm);
void Generate_matrix(double local_A[], int local_m, int n);
void Generate_vector(double local_x[], int local_n);
void Mat_vect_mult(double local_A[], double local_x[],
      double local_y[], int local_m, int n, int local_n,
      MPI_Comm comm);

/*-------------------------------------------------------------------*/
int main(void) {
      double* local_A;
      double* local_x;
      double* local_y;
      int m, local_m, n, local_n;
      int my_rank, comm_sz;
      MPI_Comm comm;
      double start, finish, loc_elapsed, elapsed, aggregatedElapsed = 0.0;

      MPI_Init(NULL, NULL);

      for (int i = 0; i < 5; i++)
      {
            comm = MPI_COMM_WORLD;
            MPI_Comm_size(comm, &comm_sz);
            MPI_Comm_rank(comm, &my_rank);

            /* Tamanho da matriz */
            m = n = 20000;
            local_m = m / comm_sz;
            local_n = n / comm_sz;
            /* Tamanho da matriz */

            Allocate_arrays(&local_A, &local_x, &local_y, local_m, n, local_n, comm);

            srandom(my_rank);
            Generate_matrix(local_A, local_m, n);
            Generate_vector(local_x, local_n);

            MPI_Barrier(comm);
            start = MPI_Wtime();
            Mat_vect_mult(local_A, local_x, local_y, local_m, n, local_n, comm);
            finish = MPI_Wtime();
            loc_elapsed = finish - start;
            MPI_Reduce(&loc_elapsed, &elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, comm);

            if (my_rank == 0) {
                  printf("[Execucao %d] Elapsed time = %.3f milliseconds\n", i + 1, elapsed * 1000);
                  aggregatedElapsed += elapsed;
            }

            free(local_A);
            free(local_x);
            free(local_y);
      }

      if (my_rank == 0) {
            printf("[Media] Elapsed time = %.3f milliseconds\n", aggregatedElapsed / 5 * 1000);
      }

      MPI_Finalize();

      return 0;
}  /* main */


/*-------------------------------------------------------------------*/
void Check_for_error(
      int       local_ok   /* in */,
      char      fname[]    /* in */,
      char      message[]  /* in */,
      MPI_Comm  comm       /* in */) {
      int ok;

      MPI_Allreduce(&local_ok, &ok, 1, MPI_INT, MPI_MIN, comm);
      if (ok == 0) {
            int my_rank;
            MPI_Comm_rank(comm, &my_rank);
            if (my_rank == 0) {
                  fprintf(stderr, "Proc %d > In %s, %s\n", my_rank, fname,
                        message);
                  fflush(stderr);
            }
            MPI_Finalize();
            exit(-1);
      }
}  /* Check_for_error */

/*-------------------------------------------------------------------*/
void Allocate_arrays(
      double** local_A_pp  /* out */,
      double** local_x_pp  /* out */,
      double** local_y_pp  /* out */,
      int       local_m     /* in  */,
      int       n           /* in  */,
      int       local_n     /* in  */,
      MPI_Comm  comm        /* in  */) {

      int local_ok = 1;

      *local_A_pp = malloc(local_m * n * sizeof(double));
      *local_x_pp = malloc(local_n * sizeof(double));
      *local_y_pp = malloc(local_m * sizeof(double));

      if (*local_A_pp == NULL || local_x_pp == NULL ||
            local_y_pp == NULL) local_ok = 0;
      Check_for_error(local_ok, "Allocate_arrays",
            "Can't allocate local arrays", comm);
}  /* Allocate_arrays */

/*-------------------------------------------------------------------*/
void Generate_matrix(
      double local_A[]  /* out */,
      int    local_m    /* in  */,
      int    n          /* in  */) {
      int i, j;

      for (i = 0; i < local_m; i++)
            for (j = 0; j < n; j++)
                  local_A[i * n + j] = ((double)random()) / ((double)RAND_MAX);
}  /* Generate_matrix */

/*-------------------------------------------------------------------*/
void Generate_vector(
      double local_x[] /* out */,
      int    local_n   /* in  */) {
      int i;

      for (i = 0; i < local_n; i++)
            local_x[i] = ((double)random()) / ((double)RAND_MAX);
}  /* Generate_vector */

/*-------------------------------------------------------------------*/
void Mat_vect_mult(
      double    local_A[]  /* in  */,
      double    local_x[]  /* in  */,
      double    local_y[]  /* out */,
      int       local_m    /* in  */,
      int       n          /* in  */,
      int       local_n    /* in  */,
      MPI_Comm  comm       /* in  */) {
      double* x;
      int local_i, j;
      int local_ok = 1;

      x = malloc(n * sizeof(double));
      if (x == NULL) local_ok = 0;
      Check_for_error(local_ok, "Mat_vect_mult",
            "Can't allocate temporary vector", comm);
      MPI_Allgather(local_x, local_n, MPI_DOUBLE,
            x, local_n, MPI_DOUBLE, comm);

      for (local_i = 0; local_i < local_m; local_i++) {
            local_y[local_i] = 0.0;
            for (j = 0; j < n; j++)
                  local_y[local_i] += local_A[local_i * n + j] * x[j];
      }
      free(x);
}  /* Mat_vect_mult */