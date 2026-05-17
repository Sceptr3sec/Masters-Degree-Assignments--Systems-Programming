#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>

// Global variables (maximum 2 allowed)
char *g_buffer = NULL;  // Our storage buffer
int READLINE_READ_SIZE; // Read chunk size (will be set externally)

void init_my_readline(void)
{
    if (g_buffer != NULL)
    {
        free(g_buffer);
        g_buffer = NULL;
    }
}

char *extract_line(char **buffer)
{
    char *line;
    char *newline_pos;
    char *new_buffer;
    int line_len;
    int remaining_len;
    int i;

    if (!buffer || !*buffer)
        return NULL;

    // Find newline character
    newline_pos = NULL;
    i = 0;
    while ((*buffer)[i])
    {
        if ((*buffer)[i] == '\n')
        {
            newline_pos = &((*buffer)[i]);
            break;
        }
        i++;
    }

    if (newline_pos)
    {
        // Calculate line length (without newline)
        line_len = newline_pos - *buffer;
        
        // Allocate memory for the line
        line = malloc(line_len + 1);
        if (!line)
            return NULL;
        
        // Copy line content
        i = 0;
        while (i < line_len)
        {
            line[i] = (*buffer)[i];
            i++;
        }
        line[line_len] = '\0';
        
        // Calculate remaining buffer length
        remaining_len = 0;
        i = line_len + 1; // Skip the newline
        while ((*buffer)[i])
        {
            remaining_len++;
            i++;
        }
        
        if (remaining_len > 0)
        {
            // Create new buffer with remaining content
            new_buffer = malloc(remaining_len + 1);
            if (!new_buffer)
            {
                free(line);
                return NULL;
            }
            
            i = 0;
            while (i < remaining_len)
            {
                new_buffer[i] = (*buffer)[line_len + 1 + i];
                i++;
            }
            new_buffer[remaining_len] = '\0';
            
            free(*buffer);
            *buffer = new_buffer;
        }
        else
        {
            free(*buffer);
            *buffer = NULL;
        }
        
        return line;
    }
    
    return NULL; // No complete line found
}

char *join_buffers(char *old_buffer, char *new_data, int new_size)
{
    char *result;
    int old_len = 0;
    int total_len;
    int i;

    // Calculate old buffer length
    if (old_buffer)
    {
        while (old_buffer[old_len])
            old_len++;
    }

    total_len = old_len + new_size;
    result = malloc(total_len + 1);
    if (!result)
        return NULL;

    // Copy old buffer
    i = 0;
    while (i < old_len)
    {
        result[i] = old_buffer[i];
        i++;
    }

    // Copy new data
    i = 0;
    while (i < new_size)
    {
        result[old_len + i] = new_data[i];
        i++;
    }
    
    result[total_len] = '\0';

    if (old_buffer)
        free(old_buffer);
    
    return result;
}

char *my_readline(int fd)
{
    char *temp_buffer;
    char *line;
    int bytes_read;

    // Allocate temporary buffer for reading
    temp_buffer = malloc(READLINE_READ_SIZE);
    if (!temp_buffer)
        return NULL;

    // Try to extract a line from existing buffer first
    line = extract_line(&g_buffer);
    if (line)
    {
        free(temp_buffer);
        return line;
    }

    // Read more data
    bytes_read = read(fd, temp_buffer, READLINE_READ_SIZE);
    
    if (bytes_read <= 0)
    {
        free(temp_buffer);
        
        // If we have data in buffer and reached EOF, return it
        if (g_buffer)
        {
            line = g_buffer;
            g_buffer = NULL;
            // Check if the buffer is not empty
            if (line[0] != '\0')
                return line;
            free(line);
        }
        return NULL;
    }

    // Add null terminator (temp buffer is not necessarily null-terminated)
    // We need to be careful here - we'll create a properly terminated string
    char *null_term_buffer = malloc(bytes_read + 1);
    if (!null_term_buffer)
    {
        free(temp_buffer);
        return NULL;
    }
    
    int i = 0;
    while (i < bytes_read)
    {
        null_term_buffer[i] = temp_buffer[i];
        i++;
    }
    null_term_buffer[bytes_read] = '\0';
    free(temp_buffer);

    // Join with existing buffer
    g_buffer = join_buffers(g_buffer, null_term_buffer, bytes_read);
    free(null_term_buffer);
    
    if (!g_buffer)
        return NULL;

    // Try to extract line again
    line = extract_line(&g_buffer);
    return line;
}

// Test program
int main(int ac, char **av)
{
    char *str = NULL;
    int fd;

    READLINE_READ_SIZE = 512;
    init_my_readline();

    if (ac > 1)
    {
        fd = open(av[1], O_RDONLY);
        if (fd == -1)
        {
            printf("Error opening file\n");
            return 1;
        }
    }
    else
    {
        fd = 0; // stdin
    }

    while ((str = my_readline(fd)) != NULL)
    {
        printf("Line: %s\n", str);
        free(str);
    }

    if (fd != 0)
        close(fd);
    
    init_my_readline(); // Clean up
    return 0;
}