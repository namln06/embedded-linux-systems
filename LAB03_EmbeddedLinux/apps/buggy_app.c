#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    int data[5] = {10, 20, 30, 40, 50};
    int *ptr = malloc(sizeof(int) * 5);

    if (ptr == NULL) {
        perror("malloc");
        return 1;
    }

    printf("[GDB LAB] Application started\n");

    for (int i = 0; i <= 5; i++) {
        printf("data[%d] = %d\n", i, data[i]);
    }

    free(ptr);

    printf("[GDB LAB] Application finished\n");
    return 0;
}
