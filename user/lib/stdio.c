#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "__zyr_impl.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

int getchar()
{
    char byte = 0;
    if (1 == read(stdin, &byte, 1)) {
        return byte;
    } else {
        return EOF;
    }
}

#define MAX_FD __ZYR_MAX_FD
#define LINE_WIDTH __ZYR_LINE_WIDTH

static char buffer[MAX_FD][LINE_WIDTH];
static int buffer_len[MAX_FD];

// Returns: number of chars written, negative for failure
// Warn: buffer_len[f] will not be changed
int __zyr_write_buffer(int f) {
    if (buffer_len[f] == 0) return 0;
    int r = write(f, buffer[f], buffer_len[f]);
    return r;
}

// Clear buffer_len[f]
void __zyr_clear_buffer(int f) {
    buffer_len[f] = 0;
}

int fflush(int fd) {
    if (0 <= fd && fd < MAX_FD) {
        int r = __zyr_write_buffer(fd);
        __zyr_clear_buffer(fd);
        return r >= 0 ? 0 : r;
    } else {
        return -1;
    }
}

static int out(int f, const char *s, size_t l)
{
    // return write(f, s, l);
    int ret = 0;
    for (size_t i = 0; i < l; i++) {
        char c = s[i];
        buffer[f][buffer_len[f]++] = c;
        if (buffer_len[f] == LINE_WIDTH || c == '\n') {
            int r = __zyr_write_buffer(f);
            int len = buffer_len[f];
            __zyr_clear_buffer(f);
            if (r < 0) return r;
            if (r < buffer_len[f]) return ret + r;
            ret += r;
        }
    }

    return ret;
}

int putchar(int c)
{
    char byte = c;
    return out(stdout, &byte, 1);
}

int puts(const char *s)
{
    int r;
    r = -(out(stdout, s, strlen(s)) < 0 || putchar('\n') < 0);
    return r;
}

static char digits[] = "0123456789abcdef";

static void printint(int xx, int base, int sign)
{
    char buf[16 + 1];
    int i;
    uint x;

    if (sign && (sign = xx < 0))
        x = -xx;
    else
        x = xx;

    buf[16] = 0;
    i = 15;
    do
    {
        buf[i--] = digits[x % base];
    } while ((x /= base) != 0);

    if (sign)
        buf[i--] = '-';
    i++;
    if (i < 0)
        puts("printint error");
    out(stdout, buf + i, 16 - i);
}

static void printptr(uint64 x)
{
    int i = 0, j;
    char buf[32 + 1];
    buf[i++] = '0';
    buf[i++] = 'x';
    for (j = 0; j < (sizeof(uint64) * 2); j++, x <<= 4)
        buf[i++] = digits[x >> (sizeof(uint64) * 8 - 4)];
    buf[i] = 0;
    out(stdout, buf, i);
}

// Print to the console. only understands %d, %x, %p, %s.
void printf(const char *fmt, ...)
{
    va_list ap;
    int l = 0;
    char *a, *z, *s = (char *)fmt;
    int f = stdout;

    va_start(ap, fmt);
    for (;;)
    {
        if (!*s)
            break;
        for (a = s; *s && *s != '%'; s++)
            ;
        for (z = s; s[0] == '%' && s[1] == '%'; z++, s += 2)
            ;
        l = z - a;
        out(f, a, l);
        if (l)
            continue;
        if (s[1] == 0)
            break;
        switch (s[1])
        {
        case 'd':
            printint(va_arg(ap, int), 10, 1);
            break;
        case 'x':
            printint(va_arg(ap, int), 16, 1);
            break;
        case 'p':
            printptr(va_arg(ap, uint64));
            break;
        case 's':
            if ((a = va_arg(ap, char *)) == 0)
                a = "(null)";
            l = strnlen(a, 200);
            out(f, a, l);
            break;
        default:
            // Print unknown % sequence to draw attention.
            putchar('%');
            putchar(s[1]);
            break;
        }
        s += 2;
    }
    va_end(ap);
}