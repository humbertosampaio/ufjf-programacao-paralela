#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

char* INPUT_FILE_NAME = "mpi_odd_even_exercicio_7_input.txt";
char* OUTPUT_FILE_NAME = "mpi_odd_even_exercicio_7_output.txt";

const int RANDOM_NUMBER_UPPER_BOUND = 100000;
const int ELEMENTS_IN_SOURCE_VECTOR = 16000000;

/* Local functions */
void Read_vector_from_input_file(int* global_A, int global_n);
void Write_vector_to_output_file(int A[]);

void Usage(char* program);
void Merge_low(int local_A[], int temp_B[], int temp_C[],
   int local_n);
void Merge_high(int local_A[], int temp_B[], int temp_C[],
   int local_n);
void Generate_list(int local_A[], int local_n, int my_rank);
int  Compare(const void* a_p, const void* b_p);

/* Functions involving communication */
void Sort(int local_A[], int local_n, int my_rank, int p, MPI_Comm comm);
void Odd_even_iter(int local_A[], int temp_B[], int temp_C[],
   int local_n, int phase, int even_partner, int odd_partner,
   int my_rank, int p, MPI_Comm comm);

void Print_global_list(int local_A[], int local_n, int my_rank, int p, MPI_Comm comm);


/*-------------------------------------------------------------------*/
int main(int argc, char* argv[]) {
   int my_rank, p;
   int* local_A, * global_A;
   int global_n;
   int local_n;
   MPI_Comm comm;
   double start, finish, loc_elapsed, elapsed, file_read_elapsed, aggregated_elapsed = 0.0;

   MPI_Init(&argc, &argv);

   comm = MPI_COMM_WORLD;
   MPI_Comm_size(comm, &p);
   MPI_Comm_rank(comm, &my_rank);

   global_n = ELEMENTS_IN_SOURCE_VECTOR;
   local_n = global_n / p;

   global_A = (int*)malloc(global_n * sizeof(int));
   local_A = (int*)malloc(local_n * sizeof(int));

   if (my_rank == 0) {
      start = MPI_Wtime();
      Read_vector_from_input_file(global_A, global_n);
      finish = MPI_Wtime();
      file_read_elapsed = finish - start;
      printf("Tempo para leitura do vetor (nao-paralelizavel): %.3fms\n", file_read_elapsed * 1000);
   }

   MPI_Barrier(comm);
   start = MPI_Wtime();
   MPI_Scatter(global_A, local_n, MPI_INT, local_A, local_n, MPI_INT, 0, comm);
   Sort(local_A, local_n, my_rank, p, comm);
   MPI_Gather(local_A, local_n, MPI_INT, global_A, local_n, MPI_INT, 0, comm);
   finish = MPI_Wtime();

   loc_elapsed = finish - start;
   MPI_Reduce(&loc_elapsed, &elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, comm);

   if (my_rank == 0) {
      // printf("[Execucao %d] Elapsed: %.3f milliseconds\n", i, elapsed * 1000);
      printf("Tempo para scatter/gather/sort (paralelizado):   %.3fms\n", elapsed * 1000);
      printf("Tempo total:                                     %.3fms\n", (elapsed + file_read_elapsed) * 1000);
      // aggregated_elapsed += elapsed;
   }

   free(local_A);

   // for (int i = 0; i < 5; i++)
   // {
   //
   // }

   // if (my_rank == 0) {
   //    printf("[Media] Elapsed: %.3f milliseconds\n", aggregated_elapsed / 5 * 1000);
   // }

   MPI_Finalize();

   return 0;
}  /* main */

void Read_vector_from_input_file(int* global_A, int global_n) {

   FILE* file_ptr = fopen(INPUT_FILE_NAME, "r");
   if (file_ptr == NULL) {
      printf("ERRO. O arquivo %s nao foi encontrado.\n", INPUT_FILE_NAME);
      exit(-1);
   }

   for (
      int i = 0, retval = 1;
      i < global_n && retval == 1;
      i++)
   {
      retval = fscanf(file_ptr, "%d", &global_A[i]);
   }

   fclose(file_ptr);
}

void Write_vector_to_output_file(int A[]) {

   FILE* file_ptr = fopen(OUTPUT_FILE_NAME, "w");
   if (file_ptr == NULL) {
      printf("ERRO. O arquivo %s nao foi encontrado.\n", OUTPUT_FILE_NAME);
      exit(-1);
   }

   for (int i = 0; i < ELEMENTS_IN_SOURCE_VECTOR; i++)
      fprintf(file_ptr, "%d ", A[i]);

   fclose(file_ptr);
}

void Print_global_list(int local_A[], int local_n, int my_rank, int p, MPI_Comm comm) {
   int* A;
   int i, n;

   if (my_rank == 0) {
      n = p * local_n;
      A = (int*)malloc(n * sizeof(int));
      MPI_Gather(local_A, local_n, MPI_INT, A, local_n, MPI_INT, 0, comm);
      printf("Global list:\n");
      for (i = 0; i < n; i++)
         printf("%d ", A[i]);

      printf("\n\n");
      free(A);
   }
   else {
      MPI_Gather(local_A, local_n, MPI_INT, A, local_n, MPI_INT, 0, comm);
   }

}

/*-------------------------------------------------------------------
 * Function:   Generate_list
 * Purpose:    Fill list with random ints
 * Input Args: local_n, my_rank
 * Output Arg: local_A
 */
void Generate_list(int local_A[], int local_n, int my_rank) {
   int i;

   srandom(my_rank + 1);
   for (i = 0; i < local_n; i++)
      local_A[i] = random() % RANDOM_NUMBER_UPPER_BOUND;

}  /* Generate_list */

/*-------------------------------------------------------------------
 * Function:  Usage
 * Purpose:   Print command line to start program
 * In arg:    program:  name of executable
 * Note:      Purely local, run only by process 0;
 */
void Usage(char* program) {
   fprintf(stderr, "usage:  mpirun -np <p> %s <g|i> <global_n>\n",
      program);
   fprintf(stderr, "   - p: the number of processes \n");
   fprintf(stderr, "   - g: generate random, distributed list\n");
   fprintf(stderr, "   - i: user will input list on process 0\n");
   fprintf(stderr, "   - global_n: number of elements in global list");
   fprintf(stderr, " (must be evenly divisible by p)\n");
   fflush(stderr);
}  /* Usage */

/*-------------------------------------------------------------------
 * Function:    Compare
 * Purpose:     Compare 2 ints, return -1, 0, or 1, respectively, when
 *              the first int is less than, equal, or greater than
 *              the second.  Used by qsort.
 */
int Compare(const void* a_p, const void* b_p) {
   int a = *((int*)a_p);
   int b = *((int*)b_p);

   if (a < b)
      return -1;
   else if (a == b)
      return 0;
   else /* a > b */
      return 1;
}  /* Compare */

/*-------------------------------------------------------------------
 * Function:    Sort
 * Purpose:     Sort local list, use odd-even sort to sort
 *              global list.
 * Input args:  local_n, my_rank, p, comm
 * In/out args: local_A
 */
void Sort(int local_A[], int local_n, int my_rank, int p, MPI_Comm comm) {
   int phase;
   int* temp_B, * temp_C;
   int even_partner;  /* phase is even or left-looking */
   int odd_partner;   /* phase is odd or right-looking */

   /* Temporary storage used in merge-split */
   temp_B = (int*)malloc(local_n * sizeof(int));
   temp_C = (int*)malloc(local_n * sizeof(int));

   /* Find partners:  negative rank => do nothing during phase */
   if (my_rank % 2 != 0) {
      even_partner = my_rank - 1;
      odd_partner = my_rank + 1;
      if (odd_partner == p) odd_partner = MPI_PROC_NULL;  // Idle during odd phase
   }
   else {
      even_partner = my_rank + 1;
      if (even_partner == p) even_partner = MPI_PROC_NULL;  // Idle during even phase
      odd_partner = my_rank - 1;
   }

   /* Sort local list using built-in quick sort */
   qsort(local_A, local_n, sizeof(int), Compare);

#  ifdef DEBUG
   printf("Proc %d > before loop in sort\n", my_rank);
   fflush(stdout);
#  endif

   for (phase = 0; phase < p; phase++)
      Odd_even_iter(local_A, temp_B, temp_C, local_n, phase,
         even_partner, odd_partner, my_rank, p, comm);

   free(temp_B);
   free(temp_C);
}  /* Sort */


/*-------------------------------------------------------------------
 * Function:    Odd_even_iter
 * Purpose:     One iteration of Odd-even transposition sort
 * In args:     local_n, phase, my_rank, p, comm
 * In/out args: local_A
 * Scratch:     temp_B, temp_C
 */
void Odd_even_iter(int local_A[], int temp_B[], int temp_C[],
   int local_n, int phase, int even_partner, int odd_partner,
   int my_rank, int p, MPI_Comm comm) {
   MPI_Status status;

   if (phase % 2 == 0) {
      if (even_partner >= 0) {
         MPI_Sendrecv(local_A, local_n, MPI_INT, even_partner, 0,
            temp_B, local_n, MPI_INT, even_partner, 0, comm,
            &status);
         if (my_rank % 2 != 0)
            Merge_high(local_A, temp_B, temp_C, local_n);
         else
            Merge_low(local_A, temp_B, temp_C, local_n);
      }
   }
   else { /* odd phase */
      if (odd_partner >= 0) {
         MPI_Sendrecv(local_A, local_n, MPI_INT, odd_partner, 0,
            temp_B, local_n, MPI_INT, odd_partner, 0, comm,
            &status);
         if (my_rank % 2 != 0)
            Merge_low(local_A, temp_B, temp_C, local_n);
         else
            Merge_high(local_A, temp_B, temp_C, local_n);
      }
   }
}  /* Odd_even_iter */

/*-------------------------------------------------------------------
 * Function:    Merge_low
 * Purpose:     Merge the smallest local_n elements in my_keys
 *              and recv_keys into temp_keys.  Then copy temp_keys
 *              back into my_keys.
 * In args:     local_n, recv_keys
 * In/out args: my_keys
 * Scratch:     temp_keys
 */
void Merge_low(
   int  my_keys[],     /* in/out    */
   int  recv_keys[],   /* in        */
   int  temp_keys[],   /* scratch   */
   int  local_n        /* = n/p, in */) {
   int m_i, r_i, t_i;

   m_i = r_i = t_i = 0;
   while (t_i < local_n) {
      if (my_keys[m_i] <= recv_keys[r_i]) {
         temp_keys[t_i] = my_keys[m_i];
         t_i++; m_i++;
      }
      else {
         temp_keys[t_i] = recv_keys[r_i];
         t_i++; r_i++;
      }
   }

   memcpy(my_keys, temp_keys, local_n * sizeof(int));
}  /* Merge_low */

/*-------------------------------------------------------------------
 * Function:    Merge_high
 * Purpose:     Merge the largest local_n elements in local_A
 *              and temp_B into temp_C.  Then copy temp_C
 *              back into local_A.
 * In args:     local_n, temp_B
 * In/out args: local_A
 * Scratch:     temp_C
 */
void Merge_high(int local_A[], int temp_B[], int temp_C[],
   int local_n) {
   int ai, bi, ci;

   ai = local_n - 1;
   bi = local_n - 1;
   ci = local_n - 1;
   while (ci >= 0) {
      if (local_A[ai] >= temp_B[bi]) {
         temp_C[ci] = local_A[ai];
         ci--; ai--;
      }
      else {
         temp_C[ci] = temp_B[bi];
         ci--; bi--;
      }
   }

   memcpy(local_A, temp_C, local_n * sizeof(int));
}  /* Merge_high */