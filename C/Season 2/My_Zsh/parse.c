#include "shell.h"

/*
** Strip surrounding single or double quotes from a token.
** Returns a newly allocated string.
*/
char    *strip_quotes(const char *s)
{
    size_t  len;
    char    q;

    if (!s)
        return (NULL);
    len = strlen(s);
    if (len >= 2)
    {
        q = s[0];
        if ((q == '\'' || q == '"') && s[len - 1] == q)
            return (strndup(s + 1, len - 2));
    }
    return (ft_strdup(s));
}

/*
** Split line into tokens on whitespace, respecting single/double quotes.
** Returns NULL-terminated array; sets *argc.
*/
char **parse_line(char *line, int *argc, t_env *env)
{
    char    **args;
    char    buf[MAX_PATH];
    int     bi;
    int     ai;
    int     in_sq;
    int     in_dq;
    int     i;
    char    c;
    char    *expanded;

    args = malloc(sizeof(char *) * MAX_ARGS);
    if (!args)
        return (NULL);
    bi = 0;
    ai = 0;
    in_sq = 0;
    in_dq = 0;
    i = 0;
    while ((c = line[i]) != '\0')
    {
        if (c == '\'' && !in_dq)
            in_sq = !in_sq;
        else if (c == '"' && !in_sq)
            in_dq = !in_dq;
        else if ((c == ' ' || c == '\t') && !in_sq && !in_dq)
        {
            if (bi > 0)
            {
                buf[bi] = '\0';
                expanded = expand_vars(buf, env);
                args[ai++] = expanded;
                bi = 0;
            }
        }
        else
        {
            if (bi < MAX_PATH - 1)
                buf[bi++] = c;
        }
        i++;
    }
    if (bi > 0)
    {
        buf[bi] = '\0';
        expanded = expand_vars(buf, env);
        args[ai++] = expanded;
    }
    args[ai] = NULL;
    *argc = ai;
    return (args);
}

/*
** Expand $VAR references using the env list.
** Returns a newly allocated string.
*/
char    *expand_vars(const char *s, t_env *env)
{
    char    result[MAX_PATH * 4];
    char    varname[256];
    int     ri;
    int     i;
    int     vi;
    char    *val;

    ri = 0;
    i = 0;
    while (s[i] && ri < (int)sizeof(result) - 1)
    {
        if (s[i] == '$' && s[i + 1] &&
            (s[i+1] == '_' || (s[i+1] >= 'A' && s[i+1] <= 'Z') ||
             (s[i+1] >= 'a' && s[i+1] <= 'z')))
        {
            i++;
            vi = 0;
            while (s[i] && (s[i] == '_' ||
                   (s[i] >= 'A' && s[i] <= 'Z') ||
                   (s[i] >= 'a' && s[i] <= 'z') ||
                   (s[i] >= '0' && s[i] <= '9')))
                varname[vi++] = s[i++];
            varname[vi] = '\0';
            val = env_get(env, varname);
            if (val)
            {
                int vl = (int)strlen(val);
                if (ri + vl < (int)sizeof(result) - 1)
                {
                    memcpy(result + ri, val, vl);
                    ri += vl;
                }
            }
        }
        else
            result[ri++] = s[i++];
    }
    result[ri] = '\0';
    return (ft_strdup(result));
}

void    free_args(char **args)
{
    int i;

    if (!args)
        return;
    i = 0;
    while (args[i])
        free(args[i++]);
    free(args);
}