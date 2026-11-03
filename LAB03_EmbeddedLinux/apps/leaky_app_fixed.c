#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    char *buffer1 = malloc(100);
    char *buffer2 = malloc(200);
    int *array = malloc(50 * sizeof(int));

    if (!buffer1 || !buffer2 || !array) {
        perror("malloc");
        free(buffer1);
        free(buffer2);
        free(array);
        return 1;
    }

    strcpy(buffer1, "This memory is fixed");
    strcpy(buffer2, "Valgrind memory debugging");

    for (int i = 0; i < 50; i++) {
        array[i] = i;
    }

    printf("Buffer 1: %s\n", buffer1);
    printf("Buffer 2: %s\n", buffer2);
    printf("Array last element: %d\n", array[49]);

    free(array);
    free(buffer2);
    free(buffer1);

    return 0;
}
