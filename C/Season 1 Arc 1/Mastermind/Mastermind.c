#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define MAX_INPUT 4
#define MAX_TRIES 10

void print_prompt(int round) {
    printf("Please enter a valid guess\n---\nRound %d\n>", round);
}

int read_input(char *buffer, size_t size) {
    char c;
    unsigned int i = 0;
    while (i < size - 1) {
        int r = read(0, &c, 1);
        if (r <= 0) {
            buffer[i] = '\0';
            return -1; // EOF or error
        }
        if (c == '\n') {
            break;
        }
        buffer[i++] = c;
    }
    buffer[i] = '\0';
    return i;
}

int is_valid_input(const char *input) {
    if (strlen(input) != MAX_INPUT)
        return 0;
    int seen[10] = {0};
    for (int i = 0; i < MAX_INPUT; i++) {
        if (input[i] < '0' || input[i] > '8')
            return 0;
        if (seen[input[i] - '0'])
            return 0; // not distinct
        seen[input[i] - '0'] = 1;
    }
    return 1;
}

void evaluate_guess(const char *code, const char *guess, int *well_placed, int *misplaced) {
    *well_placed = 0;
    *misplaced = 0;
    int code_count[10] = {0};
    int guess_count[10] = {0};

    for (int i = 0; i < MAX_INPUT; i++) {
        if (guess[i] == code[i]) {
            (*well_placed)++;
        } else {
            code_count[code[i] - '0']++;
            guess_count[guess[i] - '0']++;
        }
    }

    for (int i = 0; i < 10; i++) {
        *misplaced += (code_count[i] < guess_count[i]) ? code_count[i] : guess_count[i];
    }
}

void generate_random_code(char *code) {
    int used[9] = {0};
    int count = 0;
    while (count < MAX_INPUT) {
        int n = rand() % 9;
        if (!used[n]) {
            code[count++] = '0' + n;
            used[n] = 1;
        }
    }
    code[MAX_INPUT] = '\0';
}

int main(int argc, char *argv[]) {
    setbuf(stdout, NULL);
    srand(time(NULL));

    char secret_code[MAX_INPUT + 1] = {0};
    int attempts = MAX_TRIES;
    int round = 0;

    // Parse args
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            strncpy(secret_code, argv[i + 1], MAX_INPUT);
            secret_code[MAX_INPUT] = '\0';
            if (!is_valid_input(secret_code)) {
                printf("Invalid custom code passed via -c\n");
                return 1;
            }
            i++;
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            attempts = atoi(argv[++i]);
        }
    }

    if (secret_code[0] == '\0') {
        generate_random_code(secret_code);
    }

    printf("Will you find the secret code?\n");

    char guess[MAX_INPUT + 1];

    while (round < attempts) {
        print_prompt(round);
        if (read_input(guess, sizeof(guess)) == -1) {
            break; // EOF
        }

        if (!is_valid_input(guess)) {
            printf("Wrong input!\n");
            continue;
        }

        int well = 0, mis = 0;
        evaluate_guess(secret_code, guess, &well, &mis);
        printf("Well placed pieces: %d\n", well);
        printf("Misplaced pieces: %d\n", mis);

        if (well == MAX_INPUT) {
            printf("Congratz! You did it!\n");
            return 0;
        }

        round++;
    }

    printf("You lose\n");
    return 0;
}
