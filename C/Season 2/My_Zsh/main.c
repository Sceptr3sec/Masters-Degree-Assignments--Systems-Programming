#include "shell.h"

static void print_prompt(t_shell *sh)
{
    char    cwd[MAX_PATH];
    char    *pwd;

    pwd = env_get(sh->env, "PWD");
    if (!pwd)
    {
        if (getcwd(cwd, sizeof(cwd)))
            pwd = cwd;
        else
            pwd = "?";
    }
    printf("[%s]> ", pwd);
    fflush(stdout);
}

static void handle_sigint(int sig)
{
    (void)sig;
    write(STDOUT_FILENO, "\n", 1);
}

static void shell_loop(t_shell *sh)
{
    char    *line;
    size_t  len;
    ssize_t nread;
    char    **args;
    int     argc;

    line = NULL;
    len = 0;
    while (1)
    {
        if (isatty(STDIN_FILENO))
            print_prompt(sh);

        nread = getline(&line, &len, stdin);
        if (nread == -1)
        {
            /* EOF (Ctrl+D) */
            if (isatty(STDIN_FILENO))
                printf("\n");
            break;
        }
        /* strip trailing newline */
        if (nread > 0 && line[nread - 1] == '\n')
            line[nread - 1] = '\0';

        /* skip blank lines */
        argc = 0;
        args = parse_line(line, &argc, sh->env);
        if (!args || argc == 0)
        {
            free_args(args);
            continue;
        }

        sh->last_status = execute(sh, args, argc);
        free_args(args);
    }
    free(line);
}

int main(int ac, char **av, char **envp)
{
    t_shell sh;

    (void)ac;
    (void)av;

    sh.env = env_from_array(envp);
    sh.prev_dir = NULL;
    sh.last_status = 0;

    /* update PWD if not already set */
    if (!env_get(sh.env, "PWD"))
    {
        char cwd[MAX_PATH];
        if (getcwd(cwd, sizeof(cwd)))
            env_set(&sh.env, "PWD", cwd);
    }

    signal(SIGINT, handle_sigint);

    shell_loop(&sh);

    env_free(sh.env);
    free(sh.prev_dir);
    return (sh.last_status);
}