#include "string.h"

int strlen(const char *s)
{
    int l = 0;
    while(s[l])
        l++;
    return l;
}

int strcmp(const char *s1, const char *s2)
{
    int l1 = strlen(s1);
    int l2 = strlen(s2);

    if(l1 != l2)
        return l1 - l2;

    int lt = 0;
    while(s1[lt] == s2[lt] && s1[lt])
        lt++;

    return l1 - lt;
}