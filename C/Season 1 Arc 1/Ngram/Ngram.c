#include <stdio.h>
#include <string.h>

#define MAX_LENGTH 1000
#define ASCII_SIZE 128

void countOccurrences(const char *str, int *count) { //This REALLY makes me appreciate libraries in JS and Python :(
    while (*str) {
        if ((unsigned char) *str < ASCII_SIZE) {
            count[(int)(*str)]++;
        }
        str++;
    }
}

void printOccurrences(int *count) {
    for (int i = 0; i < ASCII_SIZE; i++) {
        if (count[i] > 0) {
            printf("%c:%d\n", i, count[i]);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s text [text2, text3...]\n", argv[0]);
        return 1;
    }

    int count[ASCII_SIZE] = {0};

    for (int i = 1; i < argc; i++) {
        countOccurrences(argv[i], count);
    }

    printOccurrences(count);

    return 0;
}
