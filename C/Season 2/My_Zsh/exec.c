#include "shell.h"

/*
** Search each directory in PATH for an executable named cmd.
** Returns a newly allocated full path, or NULL if not found.
*/
char    *find_binary(t_env *env, const char *cmd)
{
    char        *path_env;
    char        *path_copy;
    char        *dir;
    char        *full;
    char        *tmp;
    struct stat st;

    /* if cmd contains a slash, use it directly */
    if (strchr(cmd, '/'))
    {
        if (stat(cmd, &st) == 0 && (st.st_mode & S_IXUSR))
            return (ft_strdup(cmd));
        return (NULL);
    }

    path_env = env_get(env, "PATH");
    if (!path_env)
        return (NULL);

    path_copy = ft_strdup(path_env);
    if (!path_copy)
        return (NULL);

    dir = strtok(path_copy, ":");
    while (dir)
    {
        tmp = ft_strjoin(dir, "/");
        full = ft_strjoin(tmp, cmd);
        free(tmp);
        if (full && stat(full, &st) == 0 && (st.st_mode & S_IXUSR))
        {
            free(path_copy);
            return (full);
        }
        free(full);
        dir = strtok(NULL, ":");
    }
    free(path_copy);
    return (NULL);
}

/*
** Fork and exec a binary. Parent waits for child to finish.
** Returns child's exit status.
*/
static int exec_binary(t_shell *sh, char **args, const char *path)
{
    pid_t   pid;
    pid_t   wpid;
    int     status;
    char    **envp;

    envp = env_to_array(sh->env);
    pid = fork();
    if (pid < 0)
    {
        perror("fork");
        if (envp)
        {
            int i = 0;
            while (envp[i]) free(envp[i++]);
            free(envp);
        }
        return (1);
    }
    if (pid == 0)
    {
        /* child */
        execve(path, args, envp);
        dprintf(STDERR_FILENO, "%s: %s\n", args[0], strerror(errno));
        exit(EXIT_FAILURE);
    }
    /* parent: free envp and wait */
    if (envp)
    {
        int i = 0;
        while (envp[i]) free(envp[i++]);
        free(envp);
    }
    do {
        wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    (void)wpid;

    if (WIFEXITED(status))
        return (WEXITSTATUS(status));
    if (WIFSIGNALED(status))
    {
        if (WTERMSIG(status) == SIGSEGV)
            dprintf(STDERR_FILENO, "Segmentation fault\n");
        return (128 + WTERMSIG(status));
    }
    return (1);
}

/*
** Main execution dispatcher.
** Returns the command's exit status.
*/
int execute(t_shell *sh, char **args, int argc)
{
    char    *path;
    int     ret;

    if (argc == 0 || !args || !args[0])
        return (0);

    /* builtins run in-process */
    if (is_builtin(args[0]))
        return (run_builtin(sh, args, argc));

    /* find binary in PATH */
    path = find_binary(sh->env, args[0]);
    if (!path)
    {
        dprintf(STDERR_FILENO, "%s: command not found\n", args[0]);
        return (127);
    }
    ret = exec_binary(sh, args, path);
    free(path);
    return (ret);
}