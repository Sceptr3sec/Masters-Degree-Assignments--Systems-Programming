#include "shell.h"

t_env   *env_new(const char *key, const char *value)
{
    t_env   *node;

    node = malloc(sizeof(t_env));
    if (!node)
        return (NULL);
    node->key = ft_strdup(key);
    node->value = ft_strdup(value ? value : "");
    node->next = NULL;
    if (!node->key || !node->value)
    {
        free(node->key);
        free(node->value);
        free(node);
        return (NULL);
    }
    return (node);
}

t_env   *env_find(t_env *list, const char *key)
{
    while (list)
    {
        if (strcmp(list->key, key) == 0)
            return (list);
        list = list->next;
    }
    return (NULL);
}

char    *env_get(t_env *list, const char *key)
{
    t_env   *node;

    node = env_find(list, key);
    return (node ? node->value : NULL);
}

void    env_set(t_env **list, const char *key, const char *value)
{
    t_env   *node;
    t_env   *cur;
    char    *new_val;

    node = env_find(*list, key);
    if (node)
    {
        new_val = ft_strdup(value ? value : "");
        free(node->value);
        node->value = new_val;
        return;
    }
    node = env_new(key, value);
    if (!node)
        return;
    if (!*list)
    {
        *list = node;
        return;
    }
    cur = *list;
    while (cur->next)
        cur = cur->next;
    cur->next = node;
}

void    env_unset(t_env **list, const char *key)
{
    t_env   *cur;
    t_env   *prev;

    if (!list || !*list)
        return;
    cur = *list;
    prev = NULL;
    while (cur)
    {
        if (strcmp(cur->key, key) == 0)
        {
            if (prev)
                prev->next = cur->next;
            else
                *list = cur->next;
            free(cur->key);
            free(cur->value);
            free(cur);
            return;
        }
        prev = cur;
        cur = cur->next;
    }
}

t_env   *env_from_array(char **envp)
{
    t_env   *list;
    char    *eq;
    char    *key;
    char    *value;
    int     i;

    list = NULL;
    i = 0;
    while (envp && envp[i])
    {
        eq = strchr(envp[i], '=');
        if (eq)
        {
            key = strndup(envp[i], eq - envp[i]);
            value = ft_strdup(eq + 1);
            env_set(&list, key, value);
            free(key);
            free(value);
        }
        i++;
    }
    return (list);
}

void    env_free(t_env *list)
{
    t_env   *next;

    while (list)
    {
        next = list->next;
        free(list->key);
        free(list->value);
        free(list);
        list = next;
    }
}

char    **env_to_array(t_env *list)
{
    t_env   *cur;
    char    **arr;
    char    *tmp;
    int     count;
    int     i;

    count = 0;
    cur = list;
    while (cur)
    {
        count++;
        cur = cur->next;
    }
    arr = malloc(sizeof(char *) * (count + 1));
    if (!arr)
        return (NULL);
    cur = list;
    i = 0;
    while (cur)
    {
        tmp = ft_strjoin(cur->key, "=");
        arr[i] = ft_strjoin(tmp, cur->value);
        free(tmp);
        i++;
        cur = cur->next;
    }
    arr[i] = NULL;
    return (arr);
}