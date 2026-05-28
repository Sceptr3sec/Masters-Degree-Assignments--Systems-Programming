char * mergeAlternately(char * word1, char * word2){
    int len1 = strlen(word1);
    int len2 = strlen(word2);

    char* result = malloc((len1 + len2 + 1) * sizeof(char));

    int i =0;
    int j=0;
    int k=0;

    while (i<len1 || j < len2){
        if(i<len1) {
            result[k] = word1[i];
            i++;
            k++;
        }
        if(j < len2){
            result[k] = word2[j];
            j++;
            k++;
        }
    }

    result[k] = '\0';

    return result;
}