int strStr(char* haystack, char* needle) {
    int len = strlen(haystack);
    int needlelen = strlen(needle);

    for(int i = 0;i<=len - needlelen; i++){
        int j = 0;

        while (j < needlelen && haystack[i+j] == needle[j]){
            j++;
        }

        if(j==needlelen){
            return i;
        }
    }
    return -1;
}