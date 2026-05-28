#include "my_mouse.h"

void	map_error(void)
{
	fprintf(stderr, "MAP ERROR");
}

int	idx(int y, int x, int w)
{
	return (y * w + x);
}

int	strip_nl(char *buf)
{
	int	len;

	len = (int)strlen(buf);
	while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
		buf[--len] = '\0';
	return (len);
}

int	parse_header(const char *line, t_maze *m)
{
	const char	*p;
	int			h;
	int			w;
	int			i;
	int			j;

	p = line;
	h = 0;
	w = 0;
	while (*p >= '0' && *p <= '9')
		h = h * 10 + (*p++ - '0');
	if (*p != 'x' || h <= 0)
		return (0);
	p++;
	while (*p >= '0' && *p <= '9')
		w = w * 10 + (*p++ - '0');
	if (w <= 0)
		return (0);
	if ((int)strlen(p) < 5)
		return (0);
	i = 0;
	while (i < 5)
	{
		j = i + 1;
		while (j < 5)
		{
			if (p[i] == p[j])
				return (0);
			j++;
		}
		i++;
	}
	m->height = h;
	m->width = w;
	m->ch_full = p[0];
	m->ch_empty = p[1];
	m->ch_path = p[2];
	m->ch_enter = p[3];
	m->ch_exit = p[4];
	strncpy(m->header, line, MAX_LINE - 1);
	m->header[MAX_LINE - 1] = '\0';
	return (1);
}

void	maze_free(t_maze *m)
{
	int	i;

	if (!m->lines)
		return ;
	i = 0;
	while (i < m->height)
		free(m->lines[i++]);
	free(m->lines);
	m->lines = NULL;
}

int	load_maze(FILE *f, t_maze *m)
{
	char	buf[MAX_LINE];
	int		enters;
	int		exits;
	int		row;
	int		len;
	int		c;
	char	ch;

	enters = 0;
	exits = 0;
	memset(m, 0, sizeof(t_maze));
	if (!fgets(buf, MAX_LINE, f))
		return (0);
	strip_nl(buf);
	if (!parse_header(buf, m))
		return (0);
	m->lines = calloc(m->height, sizeof(char *));
	if (!m->lines)
		return (0);
	row = 0;
	while (row < m->height)
	{
		if (!fgets(buf, MAX_LINE, f))
			return (0);
		len = strip_nl(buf);
		if (len != m->width)
			return (0);
		c = 0;
		while (c < len)
		{
			ch = buf[c];
			if (ch == m->ch_enter)
				enters++;
			else if (ch == m->ch_exit)
				exits++;
			else if (ch != m->ch_full && ch != m->ch_empty)
				return (0);
			c++;
		}
		m->lines[row] = malloc(len + 1);
		if (!m->lines[row])
			return (0);
		memcpy(m->lines[row], buf, len + 1);
		row++;
	}
	if (enters != 1 || exits != 1)
		return (0);
	return (1);
}

t_point	find_char(t_maze *m, char ch)
{
	t_point	p;
	int		y;
	int		x;

	p.y = -1;
	p.x = -1;
	y = 0;
	while (y < m->height)
	{
		x = 0;
		while (x < m->width)
		{
			if (m->lines[y][x] == ch)
			{
				p.y = y;
				p.x = x;
				return (p);
			}
			x++;
		}
		y++;
	}
	return (p);
}

static const int	g_dy[4] = {-1, 0, 0, 1};
static const int	g_dx[4] = {0, -1, 1, 0};

int	bfs(t_maze *m, t_point start, t_point *prev)
{
	int		total;
	int		*vis;
	t_point	*queue;
	int		head;
	int		tail;
	int		found;
	t_point	cur;
	int		d;
	int		ny;
	int		nx;
	int		ni;
	char	ch;

	total = m->height * m->width;
	vis = calloc(total, sizeof(int));
	queue = malloc(sizeof(t_point) * (total + 1));
	if (!vis || !queue)
	{
		free(vis);
		free(queue);
		return (0);
	}
	head = 0;
	tail = 0;
	found = 0;
	d = 0;
	while (d < total)
	{
		prev[d].y = -1;
		prev[d].x = -1;
		d++;
	}
	vis[idx(start.y, start.x, m->width)] = 1;
	queue[tail++] = start;
	while (head < tail)
	{
		cur = queue[head++];
		if (m->lines[cur.y][cur.x] == m->ch_exit)
		{
			found = 1;
			break ;
		}
		d = 0;
		while (d < 4)
		{
			ny = cur.y + g_dy[d];
			nx = cur.x + g_dx[d];
			if (ny >= 0 && ny < m->height && nx >= 0 && nx < m->width)
			{
				ni = idx(ny, nx, m->width);
				if (!vis[ni])
				{
					ch = m->lines[ny][nx];
					if (ch == m->ch_empty || ch == m->ch_exit)
					{
						vis[ni] = 1;
						prev[ni].y = cur.y;
						prev[ni].x = cur.x;
						queue[tail++] = (t_point){ny, nx};
					}
				}
			}
			d++;
		}
	}
	free(vis);
	free(queue);
	return (found);
}

int	trace_path(t_maze *m, t_point start, t_point *prev)
{
	t_point	ex;
	t_point	cur;
	t_point	p;
	int		steps;

	ex = find_char(m, m->ch_exit);
	if (ex.y == -1)
		return (-1);
	steps = 0;
	cur = ex;
	while (!(cur.y == start.y && cur.x == start.x))
	{
		p = prev[idx(cur.y, cur.x, m->width)];
		if (p.y == -1)
			return (-1);
		if (m->lines[p.y][p.x] == m->ch_empty)
		{
			m->lines[p.y][p.x] = m->ch_path;
			steps++;
		}
		cur = p;
	}
	return (steps);
}

static int	process_maze(const char *filename)
{
	FILE	*f;
	t_maze	m;
	t_point	start;
	t_point	*prev;
	int		total;
	int		steps;
	int		y;

	f = fopen(filename, "r");
	if (!f)
	{
		map_error();
		return (1);
	}
	if (!load_maze(f, &m))
	{
		fclose(f);
		maze_free(&m);
		map_error();
		return (1);
	}
	fclose(f);
	start = find_char(&m, m.ch_enter);
	if (start.y == -1)
	{
		maze_free(&m);
		map_error();
		return (1);
	}
	total = m.height * m.width;
	prev = malloc(sizeof(t_point) * total);
	if (!prev)
	{
		maze_free(&m);
		map_error();
		return (1);
	}
	if (!bfs(&m, start, prev))
	{
		free(prev);
		maze_free(&m);
		map_error();
		return (1);
	}
	steps = trace_path(&m, start, prev);
	free(prev);
	if (steps < 0)
	{
		maze_free(&m);
		map_error();
		return (1);
	}
	printf("%s\n", m.header);
	y = 0;
	while (y < m.height)
		printf("%s\n", m.lines[y++]);
	printf("%d STEPS\n", steps);
	maze_free(&m);
	return (0);
}

int	main(int argc, char **argv)
{
	int	i;

	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s <mapfile> [mapfile2 ...]\n", argv[0]);
		return (1);
	}
	i = 1;
	int	err;

	err = 0;
	while (i < argc)
		if (process_maze(argv[i++]))
			err = 1;
	return (err);
}