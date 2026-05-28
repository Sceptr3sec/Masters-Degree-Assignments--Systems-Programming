#include <stdbool.h>
#include <string.h>

bool repeatedSubstringPattern(char* s) {
    int n = strlen(s);

    for(int len =1; len <= n/2; len++){
        if(n%len !=0){
            continue;
        }
        bool works = true;

        for(int i=len; i<n; i++){
            if(s[i] != s[i % len]){
                works = false;
                break;
            }
            if (works){
                return true;
            }
        }
    }
    return false;
}