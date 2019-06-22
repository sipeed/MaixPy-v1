/**
 * File: printf.c
 *
 * Copyright (c) 2004,2012 Kustaa Nyholm / SpareTimeLabs
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Kustaa Nyholm or SpareTimeLabs nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stddef.h>
#include "printf.h"
#include "atomic.h"
#include "uarths.h"


/**
 * Configuration
 */

/* Enable long int support */
#define PRINTF_LONG_SUPPORT

/* Enable long long int support (implies long int support) */
#define PRINTF_LONG_LONG_SUPPORT

/* Enable %z (size_t) support */
#define PRINTF_SIZE_T_SUPPORT

/**
 * Configuration adjustments
 */
#if defined(PRINTF_LONG_LONG_SUPPORT)
#define PRINTF_LONG_SUPPORT
#endif

/* __SIZEOF_<type>__ defined at least by gcc */
#if defined(__SIZEOF_POINTER__)
#define SIZEOF_POINTER __SIZEOF_POINTER__
#endif
#if defined(__SIZEOF_LONG_LONG__)
#define SIZEOF_LONG_LONG __SIZEOF_LONG_LONG__
#endif
#if defined(__SIZEOF_LONG__)
#define SIZEOF_LONG __SIZEOF_LONG__
#endif
#if defined(__SIZEOF_INT__)
#define SIZEOF_INT __SIZEOF_INT__
#endif

#if defined(__GNUC__)
#define _TFP_GCC_NO_INLINE_ __attribute__((noinline))
#else
#define _TFP_GCC_NO_INLINE_
#endif


#if defined(PRINTF_LONG_SUPPORT)
#define BF_MAX 20 /* long = 64b on some architectures */
#else
#define BF_MAX 10 /* int = 32b on some architectures */
#endif


#define IS_DIGIT(x) ((x) >= '0' && (x) <= '9')

/* Clear unused warnings for actually unused variables */
#define UNUSED(x) (void)(x)



/**
 * Implementation
 */
struct param
{
    char lz : 1;         /* Leading zeros */
    char alt : 1;        /* alternate form */
    char uc : 1;         /* Upper case (for base16 only) */
    char align_left : 1; /* 0 == align right (default), 1 == align left */
    int width;           /* field width */
    int prec;            /* precision */
    char sign;           /* The sign to display (if any) */
    unsigned int base;   /* number base (e.g.: 8, 10, 16) */
    char *bf;            /* Buffer to output */
    size_t bf_len;       /* Buffer length */
};

static corelock_t lock = CORELOCK_INIT;



#if defined(PRINTF_LONG_LONG_SUPPORT)
static void _TFP_GCC_NO_INLINE_ ulli2a(unsigned long long int num, struct param *p)
{
    unsigned long long int d = 1;
    char *bf = p->bf;
    if ((p->prec == 0) && (num == 0))
        return;
    while (num / d >= p->base)
    {
        d *= p->base;
    }
    while (d != 0)
    {
        int dgt = num / d;
        num %= d;
        d /= p->base;
        *bf++ = dgt + (dgt < 10 ? '0' : (p->uc ? 'A' : 'a') - 10);
    }
    p->bf_len = bf - p->bf;
}

static void lli2a(long long int num, struct param *p)
{
    if (num < 0)
    {
        num = -num;
        p->sign = '-';
    }
    ulli2a(num, p);
}
#endif

#if defined(PRINTF_LONG_SUPPORT)
static void uli2a(unsigned long int num, struct param *p)
{
    unsigned long int d = 1;
    char *bf = p->bf;
    if ((p->prec == 0) && (num == 0))
        return;
    while (num / d >= p->base)
    {
        d *= p->base;
    }
    while (d != 0)
    {
        int dgt = num / d;
        num %= d;
        d /= p->base;
        *bf++ = dgt + (dgt < 10 ? '0' : (p->uc ? 'A' : 'a') - 10);
    }
    p->bf_len = bf - p->bf;
}

static void li2a(long num, struct param *p)
{
    if (num < 0)
    {
        num = -num;
        p->sign = '-';
    }
    uli2a(num, p);
}
#endif

static void ui2a(unsigned int num, struct param *p)
{
    unsigned int d = 1;
    char *bf = p->bf;
    if ((p->prec == 0) && (num == 0))
        return;
    while (num / d >= p->base)
    {
        d *= p->base;
    }
    while (d != 0)
    {
        int dgt = num / d;
        num %= d;
        d /= p->base;
        *bf++ = dgt + (dgt < 10 ? '0' : (p->uc ? 'A' : 'a') - 10);
    }
    p->bf_len = bf - p->bf;
}

static void i2a(int num, struct param *p)
{
    if (num < 0)
    {
        num = -num;
        p->sign = '-';
    }
    ui2a(num, p);
}

static int a2d(char ch)
{
    if (IS_DIGIT(ch))
        return ch - '0';
    else if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    else if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    else
        return -1;
}

static char a2u(char ch, const char **src, int base, unsigned int *nump)
{
    const char *p = *src;
    unsigned int num = 0;
    int digit;
    while ((digit = a2d(ch)) >= 0)
    {
        if (digit > base)
            break;
        num = num * base + digit;
        ch = *p++;
    }
    *src = p;
    *nump = num;
    return ch;
}

static void putchw(void *putp, putcf putf, struct param *p)
{
    char ch;
    int width = p->width;
    int prec = p->prec;
    char *bf = p->bf;
    size_t bf_len = p->bf_len;

    /* Number of filling characters */
    width -= bf_len;
    prec -= bf_len;
    if (p->sign)
        width--;
    if (p->alt && p->base == 16)
        width -= 2;
    else if (p->alt && p->base == 8)
        width--;
    if (prec > 0)
        width -= prec;

    /* Fill with space to align to the right, before alternate or sign */
    if (!p->lz && !p->align_left)
    {
        while (width-- > 0)
            putf(putp, ' ');
    }

    /* print sign */
    if (p->sign)
        putf(putp, p->sign);

    /* Alternate */
    if (p->alt && p->base == 16)
    {
        putf(putp, '0');
        putf(putp, (p->uc ? 'X' : 'x'));
    }
    else if (p->alt && p->base == 8)
    {
        putf(putp, '0');
    }

    /* Fill with zeros, after alternate or sign */
    while (prec-- > 0)
        putf(putp, '0');
    if (p->lz)
    {
        while (width-- > 0)
            putf(putp, '0');
    }

    /* Put actual buffer */
    while ((bf_len-- > 0) && (ch = *bf++))
        putf(putp, ch);

    /* Fill with space to align to the left, after string */
    if (!p->lz && p->align_left)
    {
        while (width-- > 0)
            putf(putp, ' ');
    }
}

void tfp_format(void *putp, putcf putf, const char *fmt, va_list va)
{
    struct param p;
    char bf[BF_MAX];
    char ch;

    while ((ch = *(fmt++)))
    {
        if (ch != '%')
        {
            putf(putp, ch);
        }
        else
        {
#if defined(PRINTF_LONG_SUPPORT)
            char lng = 0; /* 1 for long, 2 for long long */
#endif
            /* Init parameter struct */
            p.lz = 0;
            p.alt = 0;
            p.uc = 0;
            p.align_left = 0;
            p.width = 0;
            p.prec = -1;
            p.sign = 0;
            p.bf = bf;
            p.bf_len = 0;

            /* Flags */
            while ((ch = *(fmt++)))
            {
                switch (ch)
                {
                    case '-':
                        p.align_left = 1;
                        continue;
                    case '0':
                        p.lz = 1;
                        continue;
                    case '#':
                        p.alt = 1;
                        continue;
                    default:
                        break;
                }
                break;
            }

            if (p.align_left)
                p.lz = 0;

            /* Width */
            if (ch == '*')
            {
                ch = *(fmt++);
                p.width = va_arg(va, int);
                if (p.width < 0)
                {
                    p.align_left = 1;
                    p.width = -p.width;
                }
            }
            else if (IS_DIGIT(ch))
            {
                unsigned int width;
                ch = a2u(ch, &fmt, 10, &(width));
                p.width = width;
            }

            /* Precision */
            if (ch == '.')
            {
                ch = *(fmt++);
                if (ch == '*')
                {
                    int prec;
                    ch = *(fmt++);
                    prec = va_arg(va, int);
                    if (prec < 0)
                        /* act as if precision was
                         * omitted */
                        p.prec = -1;
                    else
                        p.prec = prec;
                }
                else if (IS_DIGIT(ch))
                {
                    unsigned int prec;
                    ch = a2u(ch, &fmt, 10, &(prec));
                    p.prec = prec;
                }
                else
                {
                    p.prec = 0;
                }
            }
            if (p.prec >= 0)
                /* precision causes zero pad to be ignored */
                p.lz = 0;

#if defined(PRINTF_SIZE_T_SUPPORT)
#if defined(PRINTF_LONG_SUPPORT)
            if (ch == 'z')
            {
                ch = *(fmt++);
                if (sizeof(size_t) == sizeof(unsigned long int))
                    lng = 1;
#if defined(PRINTF_LONG_LONG_SUPPORT)
                else if (sizeof(size_t) == sizeof(unsigned long long int))
                    lng = 2;
#endif
            }
            else
#endif
#endif

#if defined(PRINTF_LONG_SUPPORT)
                if (ch == 'l')
            {
                ch = *(fmt++);
                lng = 1;
#if defined(PRINTF_LONG_LONG_SUPPORT)
                if (ch == 'l')
                {
                    ch = *(fmt++);
                    lng = 2;
                }
#endif
            }
#endif
            switch (ch)
            {
                case 0:
                    goto abort;
                case 'u':
                    p.base = 10;
                    if (p.prec < 0)
                        p.prec = 1;
#if defined(PRINTF_LONG_SUPPORT)
#if defined(PRINTF_LONG_LONG_SUPPORT)
                    if (2 == lng)
                        ulli2a(va_arg(va, unsigned long long int), &p);
                    else
#endif
                        if (1 == lng)
                        uli2a(va_arg(va, unsigned long int), &p);
                    else
#endif
                        ui2a(va_arg(va, unsigned int), &p);
                    putchw(putp, putf, &p);
                    break;
                case 'd':  /* No break */
                case 'i':
                    p.base = 10;
                    if (p.prec < 0)
                        p.prec = 1;
#if defined(PRINTF_LONG_SUPPORT)
#if defined(PRINTF_LONG_LONG_SUPPORT)
                    if (2 == lng)
                        lli2a(va_arg(va, long long int), &p);
                    else
#endif
                        if (1 == lng)
                        li2a(va_arg(va, long int), &p);
                    else
#endif
                        i2a(va_arg(va, int), &p);
                    putchw(putp, putf, &p);
                    break;
#if defined(SIZEOF_POINTER)
                case 'p':
                    p.alt = 1;
#if defined(SIZEOF_INT) && SIZEOF_POINTER <= SIZEOF_INT
                    lng = 0;
#elif defined(SIZEOF_LONG) && SIZEOF_POINTER <= SIZEOF_LONG
                    lng = 1;
#elif defined(SIZEOF_LONG_LONG) && SIZEOF_POINTER <= SIZEOF_LONG_LONG
                    lng = 2;
#endif
#endif
                    /* No break */
                case 'x':  /* No break */
                case 'X':
                    p.base = 16;
                    p.uc = (ch == 'X') ? 1 : 0;
                    if (p.prec < 0)
                        p.prec = 1;
#if defined(PRINTF_LONG_SUPPORT)
#if defined(PRINTF_LONG_LONG_SUPPORT)
                    if (2 == lng)
                        ulli2a(va_arg(va, unsigned long long int), &p);
                    else
#endif
                        if (1 == lng)
                        uli2a(va_arg(va, unsigned long int), &p);
                    else
#endif
                        ui2a(va_arg(va, unsigned int), &p);
                    putchw(putp, putf, &p);
                    break;
                case 'o':
                    p.base = 8;
                    if (p.prec < 0)
                        p.prec = 1;
                    ui2a(va_arg(va, unsigned int), &p);
                    putchw(putp, putf, &p);
                    break;
                case 'c':
                    putf(putp, (char)(va_arg(va, int)));
                    break;
                case 's':
                {
                    unsigned int prec = p.prec;
                    char *b;
                    p.bf = va_arg(va, char*);
                    b = p.bf;
                    while ((prec-- != 0) && *b++)
                    {
                        p.bf_len++;
                    }
                    p.prec = -1;
                    putchw(putp, putf, &p);
                }
                break;
                case '%':
                    putf(putp, ch);
                    break;
                default:
                    break;
            }
        }
    }
abort:;
}

#if defined(TINYPRINTF_DEFINE_TFP_PRINTF)
static putcf stdout_putf;
static void *stdout_putp;

void init_printf(void *putp, putcf putf)
{
    stdout_putf = putf;
    stdout_putp = putp;
}

void tfp_printf(char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    tfp_format(stdout_putp, stdout_putf, fmt, va);
    va_end(va);
}
#endif

#if defined(TINYPRINTF_DEFINE_TFP_SPRINTF)
struct _vsnprintf_putcf_data
{
    size_t dest_capacity;
    char *dest;
    size_t num_chars;
};

static void _vsnprintf_putcf(void *p, char c)
{
    struct _vsnprintf_putcf_data *data = (struct _vsnprintf_putcf_data*)p;
    if (data->num_chars < data->dest_capacity)
        data->dest[data->num_chars] = c;
    data->num_chars++;
}

int tfp_vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
    struct _vsnprintf_putcf_data data;

    if (size < 1)
        return 0;

    data.dest = str;
    data.dest_capacity = size - 1;
    data.num_chars = 0;
    tfp_format(&data, _vsnprintf_putcf, format, ap);

    if (data.num_chars < data.dest_capacity)
        data.dest[data.num_chars] = '\0';
    else
        data.dest[data.dest_capacity] = '\0';

    return data.num_chars;
}

int tfp_snprintf(char *str, size_t size, const char *format, ...)
{
    va_list ap;
    int retval;

    va_start(ap, format);
    retval = tfp_vsnprintf(str, size, format, ap);
    va_end(ap);
    return retval;
}

struct _vsprintf_putcf_data
{
    char *dest;
    size_t num_chars;
};

static void _vsprintf_putcf(void *p, char c)
{
    struct _vsprintf_putcf_data *data = (struct _vsprintf_putcf_data*)p;
    data->dest[data->num_chars++] = c;
}

int tfp_vsprintf(char *str, const char *format, va_list ap)
{
    struct _vsprintf_putcf_data data;
    data.dest = str;
    data.num_chars = 0;
    tfp_format(&data, _vsprintf_putcf, format, ap);
    data.dest[data.num_chars] = '\0';
    return data.num_chars;
}

int tfp_sprintf(char *str, const char *format, ...)
{
    va_list ap;
    int retval;

    va_start(ap, format);
    retval = tfp_vsprintf(str, format, ap);
    va_end(ap);
    return retval;
}
#endif

static void uart_putf(void *unused, char c)
{
    UNUSED(unused);
    uarths_putchar(c);
}

int printk(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    /* Begin protected code */
    corelock_lock(&lock);
    tfp_format(stdout_putp, uart_putf, format, ap);
    /* End protected code */
    corelock_unlock(&lock);
    va_end(ap);

    return 0;
}

