#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

const int ARRAY_SIZE = 16000000;
const int RANDOM_NUMBER_UPPER_BOUND = 100000;
char* OUTPUT_FILE_NAME = "mpi_odd_even_exercicio_7_input.txt";

void Generate_list(int array[]) {
	int i;

	srandom(23);
	for (i = 0; i < ARRAY_SIZE; i++)
		array[i] = random() % RANDOM_NUMBER_UPPER_BOUND;

}

void Write_vector_to_output_file(int array[]) {

	FILE* file_ptr = fopen(OUTPUT_FILE_NAME, "w");
	if (file_ptr == NULL) {
		printf("ERRO. O arquivo %s nao foi encontrado.\n", OUTPUT_FILE_NAME);
		exit(-1);
	}

	for (int i = 0; i < ARRAY_SIZE; i++)
		fprintf(file_ptr, "%d ", array[i]);

	fclose(file_ptr);
}

void main() {


	int* array = (int*)malloc(ARRAY_SIZE * sizeof(int));
	Generate_list(array);
	Write_vector_to_output_file(array);

}