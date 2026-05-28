#include "shell.h"

/* ------------------------------------------------------------------ echo */
int builtin_echo(char **args, int argc)
{
    int i;
    int newline;
    int start;

    newline = 1;
    start = 1;
    if (argc > 1 && strcmp(args[1], "-n") == 0)
    {
        newline = 0;
        start = 2;
    }
    i = start;
    while (i < argc)
    {
        if (i > start)
            write(STDOUT_FILENO, " ", 1);
        write(STDOUT_FILENO, args[i], strlen(args[i]));
        i++;
    }
    if (newline)
        write(STDOUT_FILENO, "\n", 1);
    return (0);
}

/* -------------------------------------------------------------------- cd */
int builtin_cd(t_shell *sh, char **args, int argc)
{
    char    cwd[MAX_PATH];
    char    *target;

    if (argc > 2)
    {
        print_error("cd", "too many arguments");
        return (1);
    }
    /* cd with no arg → HOME */
    if (argc == 1 || strcmp(args[1], "~") == 0)
    {
        target = env_get(sh->env, "HOME");
        if (!target)
        {
            print_error("cd", "HOME not set");
            return (1);
        }
    }
    else if (strcmp(args[1], "-") == 0)
    {
        if (!sh->prev_dir)
        {
            print_error("cd", "no previous directory");
            return (1);
        }
        target = sh->prev_dir;
        printf("%s\n", target);
    }
    else
        target = args[1];

    /* save current dir before changing */
    if (!getcwd(cwd, sizeof(cwd)))
    {
        perror("cd: getcwd");
        return (1);
    }
    if (chdir(target) != 0)
    {
        dprintf(STDERR_FILENO, "cd: %s: %s\n", target, strerror(errno));
        return (1);
    }
    free(sh->prev_dir);
    sh->prev_dir = ft_strdup(cwd);

    /* update PWD in env */
    if (getcwd(cwd, sizeof(cwd)))
        env_set(&sh->env, "PWD", cwd);
    return (0);
}

/* ------------------------------------------------------------------- pwd */
int builtin_pwd(void)
{
    char cwd[MAX_PATH];

    if (!getcwd(cwd, sizeof(cwd)))
    {
        perror("pwd");
        return (1);
    }
    printf("%s\n", cwd);
    return (0);
}

/* ------------------------------------------------------------------- env */
int builtin_env(t_shell *sh)
{
    t_env *cur;

    cur = sh->env;
    while (cur)
    {
        printf("%s=%s\n", cur->key, cur->value);
        cur = cur->next;
    }
    return (0);
}

/* ---------------------------------------------------------------- setenv */
int builtin_setenv(t_shell *sh, char **args, int argc)
{
    char    *eq;
    char    *key;
    char    *value;

    if (argc == 1)
        return (builtin_env(sh));
    /* support both: setenv KEY VALUE  and  setenv KEY=VALUE */
    if (argc == 2)
    {
        eq = strchr(args[1], '=');
        if (!eq)
        {
            print_error("setenv", "usage: setenv VAR VALUE  or  setenv VAR=VALUE");
            return (1);
        }
        key = strndup(args[1], eq - args[1]);
        value = ft_strdup(eq + 1);
        env_set(&sh->env, key, value);
        free(key);
        free(value);
        return (0);
    }
    if (argc == 3)
    {
        env_set(&sh->env, args[1], args[2]);
        return (0);
    }
    print_error("setenv", "usage: setenv VAR VALUE  or  setenv VAR=VALUE");
    return (1);
}

/* -------------------------------------------------------------- unsetenv */
int builtin_unsetenv(t_shell *sh, char **args, int argc)
{
    int i;

    if (argc < 2)
    {
        print_error("unsetenv", "usage: unsetenv VAR [VAR...]");
        return (1);
    }
    i = 1;
    while (i < argc)
        env_unset(&sh->env, args[i++]);
    return (0);
}

/* ------------------------------------------------------------------ exit */
int builtin_exit(t_shell *sh, char **args, int argc)
{
    int code;

    code = sh->last_status;
    if (argc > 1)
        code = atoi(args[1]);
    env_free(sh->env);
    free(sh->prev_dir);
    exit(code);
}

/* ----------------------------------------------------------------- which */
int builtin_which(t_shell *sh, char **args, int argc)
{
    char    *path;
    int     i;
    int     ret;

    if (argc < 2)
    {
        print_error("which", "usage: which command [command...]");
        return (1);
    }
    ret = 0;
    i = 1;
    while (i < argc)
    {
        if (is_builtin(args[i]))
        {
            printf("%s: shell built-in command\n", args[i]);
        }
        else
        {
            path = find_binary(sh->env, args[i]);
            if (path)
            {
                printf("%s\n", path);
                free(path);
            }
            else
            {
                dprintf(STDERR_FILENO, "%s not found\n", args[i]);
                ret = 1;
            }
        }
        i++;
    }
    return (ret);
}

/* ------------------------------------------------------- builtin dispatch */
int is_builtin(const char *cmd)
{
    static const char *builtins[] = {
        "echo", "cd", "pwd", "env",
        "setenv", "unsetenv", "exit", "quit", "which", NULL
    };
    int i;

    i = 0;
    while (builtins[i])
    {
        if (strcmp(cmd, builtins[i]) == 0)
            return (1);
        i++;
    }
    return (0);
}

int run_builtin(t_shell *sh, char **args, int argc)
{
    if (strcmp(args[0], "echo") == 0)    return builtin_echo(args, argc);
    if (strcmp(args[0], "cd") == 0)      return builtin_cd(sh, args, argc);
    if (strcmp(args[0], "pwd") == 0)     return builtin_pwd();
    if (strcmp(args[0], "env") == 0)     return builtin_env(sh);
    if (strcmp(args[0], "setenv") == 0)  return builtin_setenv(sh, args, argc);
    if (strcmp(args[0], "unsetenv") == 0)return builtin_unsetenv(sh, args, argc);
    if (strcmp(args[0], "exit") == 0)    return builtin_exit(sh, args, argc);
    if (strcmp(args[0], "quit") == 0)    return builtin_exit(sh, args, argc);
    if (strcmp(args[0], "which") == 0)   return builtin_which(sh, args, argc);
    return (127);
}