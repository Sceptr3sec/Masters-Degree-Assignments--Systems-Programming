#ifndef MY_MOUSE_H
# define MY_MOUSE_H

# include <stdio.h>
# include <stdlib.h>
# include <string.h>

# define MAX_LINE 2048

typedef struct s_point
{
	int	y;
	int	x;
}	t_point;

typedef struct s_maze
{
	char	**lines;
	int		height;
	int		width;
	char	ch_full;
	char	ch_empty;
	char	ch_path;
	char	ch_enter;
	char	ch_exit;
	char	header[MAX_LINE];
}	t_maze;

/* maze.c */
int		parse_header(const char *line, t_maze *m);
void	maze_free(t_maze *m);
int		load_maze(FILE *f, t_maze *m);
t_point	find_char(t_maze *m, char ch);

/* bfs.c */
int		bfs(t_maze *m, t_point start, t_point *prev);
int		trace_path(t_maze *m, t_point start, t_point *prev);

/* utils.c */
void	map_error(void);
int		idx(int y, int x, int w);
int		strip_nl(char *buf);

#endif