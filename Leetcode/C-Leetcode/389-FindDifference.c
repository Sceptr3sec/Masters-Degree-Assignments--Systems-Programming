char findTheDifference(char* s, char* t){
    int total = 0;

    for (int i = 0; t[i] != '\0'; i++) {
        total += t[i];
    }

    for (int i = 0; s[i] != '\0'; i++) {
        total -= s[i];
    }

    return (char)total;
}