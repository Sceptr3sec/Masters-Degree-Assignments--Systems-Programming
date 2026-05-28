#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>

#define MAX_ARGS 1024
#define MAX_PATH 4096

/* env list node */
typedef struct s_env {
    char            *key;
    char            *value;
    struct s_env    *next;
} t_env;

/* shell state */
typedef struct s_shell {
    t_env   *env;
    char    *prev_dir;
    int     last_status;
} t_shell;

/* env list */
t_env   *env_new(const char *key, const char *value);
t_env   *env_find(t_env *list, const char *key);
void    env_set(t_env **list, const char *key, const char *value);
void    env_unset(t_env **list, const char *key);
char    *env_get(t_env *list, const char *key);
t_env   *env_from_array(char **envp);
void    env_free(t_env *list);
char    **env_to_array(t_env *list);

/* parsing */
char    **parse_line(char *line, int *argc, t_env *env);
void    free_args(char **args);
char    *strip_quotes(const char *s);
char    *expand_vars(const char *s, t_env *env);

/* execution */
int     execute(t_shell *sh, char **args, int argc);
char    *find_binary(t_env *env, const char *cmd);

/* builtins */
int     builtin_echo(char **args, int argc);
int     builtin_cd(t_shell *sh, char **args, int argc);
int     builtin_pwd(void);
int     builtin_env(t_shell *sh);
int     builtin_setenv(t_shell *sh, char **args, int argc);
int     builtin_unsetenv(t_shell *sh, char **args, int argc);
int     builtin_exit(t_shell *sh, char **args, int argc);
int     builtin_which(t_shell *sh, char **args, int argc);
int     is_builtin(const char *cmd);
int     run_builtin(t_shell *sh, char **args, int argc);

/* utils */
char    *ft_strdup(const char *s);
char    *ft_strjoin(const char *a, const char *b);
void    print_error(const char *cmd, const char *msg);

#endif