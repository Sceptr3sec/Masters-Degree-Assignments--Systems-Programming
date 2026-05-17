#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

int print_digits(unsigned int num) {
    int count = 0;
    if (num / 10 != 0) {
        count += print_digits(num / 10);
    }
    putchar(num % 10 + '0');
    count++;
    return count;
}

int print_uint(unsigned int num) {
    int count = 0;
    if (num == 0) {
        putchar('0');
        count++;
    } else {
        count += print_digits(num);
    }
    return count;
}

int print_int(int num) {
    int count = 0;
    if (num == 0) {
        putchar('0');
        count++;
    } else {
        if (num < 0) {
            putchar('-');
            count++;
            num = -num;
        }
        count += print_digits(num);
    }
    return count;
}

int print_char(char ch) {
    putchar(ch);
    return 1;
}

int print_pointer(void *ptr) {
    unsigned long addr = (unsigned long)ptr;
    int count = 0;
    count += print_char('0');
    count += print_char('x');
    
    int leading_zeros = 1;
    for (int i = sizeof(void *) * 2 - 1; i >= 0; i--) {
        int digit = (addr >> (i * 4)) & 0xF;
        if (digit != 0 || !leading_zeros) {
            count += print_char(digit < 10 ? '0' + digit : 'a' + digit - 10);
            leading_zeros = 0;
        }
    }
    if (leading_zeros)
        count += print_char('0');
    
    return count;
}

int print_octal(int num) {
    int count = 0;
    if (num == 0) {
        putchar('0');
        count++;
    } else {
        int octal[20];
        int i = 0;
        while (num != 0) {
            octal[i++] = num % 8;
            num /= 8;
        }
        for (int j = i - 1; j >= 0; j--) {
            putchar('0' + octal[j]);
            count++;
        }
    }
    return count;
}

int print_hex(int num, int uppercase) {
    int count = 0;
    if (num == 0) {
        putchar('0');
        count++;
    } else {
        char hex[20];
        int i = 0;

        while (num != 0) {
            int rem = num % 16;
            if (rem < 10)
                hex[i++] = rem + '0';
            else {
                if (uppercase)
                    hex[i++] = rem - 10 + 'A';
                else
                    hex[i++] = rem - 10 + 'A';
            }
            num /= 16;
        }
        for (int j = i - 1; j >= 0; j--) {
            putchar(hex[j]);
            count++;
        }
    }
    return count;
}

int print_string_upper(char *str) {
    int count = 0;
    while (*str != '\0') {
        putchar(toupper(*str));
        str++;
        count++;
    }
    return count;
}

int string_size(char *str) {
    int size = 0;
    while (*str != '\0') {
        str++;
        size++;
    }
    return size;
}

int my_printf(const char *format, ...) {
    int count = 0;
    va_list args;
    va_start(args, format);

    for (int i = 0; format[i] != '\0'; i++) {
        if (format[i] != '%') {
            putchar(format[i]);
            count++;
        } else {
            i++;
            if (format[i] == 's') {
                char *str = va_arg(args, char *);
                if (str == NULL) {
                    const char *null_string = "(null)";
                    while (*null_string != '\0') {
                        putchar(*null_string);
                        null_string++;
                        count++;
                    }
                } else {
                    while (*str != '\0') {
                        putchar(*str);
                        str++;
                        count++;
                    }
                }
            } else if (format[i] == 'S') {
                char *str = va_arg(args, char *);
                if (str == NULL) {
                    const char *null_string = "(null)";
                    while (*null_string != '\0') {
                        putchar(*null_string);
                        null_string++;
                        count++;
                    }
                    } else {
                    count += print_string_upper(str);
                }
            } else if (format[i] == 'd') {
                int num = va_arg(args, int);
                count += print_int(num);
            } else if (format[i] == 'u') {
                unsigned int num = va_arg(args, unsigned int);
                count += print_uint(num);
            } else if (format[i] == 'c') {
                char ch = va_arg(args, int);
                count += print_char(ch);
            } else if (format[i] == 'p') {
                void *ptr = va_arg(args, void *);
                count += print_pointer(ptr);
            } else if (format[i] == 'o') {
                int num = va_arg(args, int);
                count += print_octal(num);
            } else if (format[i] == 'x') {
                int num = va_arg(args, int);
                count += print_hex(num, 0);
            } else if (format[i] == 'X') {
                int num = va_arg(args, int);
                count += print_hex(num, 1);
            }
        }
    }

    va_end(args);
    return count;
}

int main() {
    const char *a = "Hello";
    int b = -123;
    unsigned int c = 456;
    const char *d = "Hi";
    int *ptr = &b;
    my_printf("%s %d %u %S %p %x %X\n", a, b, c, d, ptr, 14, 14);
    return 0;
}
