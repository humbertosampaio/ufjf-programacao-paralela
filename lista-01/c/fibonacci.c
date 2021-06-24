#include <stdio.h>

static unsigned long int calcularFibonacci(int n)
{
    return (n < 2) ? n : calcularFibonacci(n - 1) + calcularFibonacci(n - 2);
}

int main()
{
    for (int i = 0; i < 50; i++)
    {
        printf("(%d): %lu\n", i, calcularFibonacci(i));
    }

    return 0;
}