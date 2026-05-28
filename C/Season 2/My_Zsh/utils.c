#include "shell.h"

char    *ft_strdup(const char *s)
{
    char    *copy;
    size_t  len;

    if (!s)
        return (NULL);
    len = strlen(s);
    copy = malloc(len + 1);
    if (!copy)
        return (NULL);
    memcpy(copy, s, len + 1);
    return (copy);
}

char    *ft_strjoin(const char *a, const char *b)
{
    char    *result;
    size_t  la;
    size_t  lb;

    if (!a || !b)
        return (NULL);
    la = strlen(a);
    lb = strlen(b);
    result = malloc(la + lb + 1);
    if (!result)
        return (NULL);
    memcpy(result, a, la);
    memcpy(result + la, b, lb + 1);
    return (result);
}

void    print_error(const char *cmd, const char *msg)
{
    if (cmd)
        dprintf(STDERR_FILENO, "%s: %s\n", cmd, msg);
    else
        dprintf(STDERR_FILENO, "%s\n", msg);
}