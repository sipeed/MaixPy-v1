/**
 * File: printf.h
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
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This library is really just two files: 'tinyprintf.h' and 'tinyprintf.c'.
 *
 * They provide a simple and small (+400 loc) printf functionality to be used
 * in embedded systems.
 *
 * I've found them so useful in debugging that I do not bother with a debugger
 * at all.
 *
 * They are distributed in source form, so to use them, just compile them into
 * your project.
 *
 * Two printf variants are provided: printf and the 'sprintf' family of
 * functions ('snprintf', 'sprintf', 'vsnprintf', 'vsprintf').
 *
 * The formats supported by this implementation are: 'c' 'd' 'i' 'o' 'p' 'u'
 * 's' 'x' 'X'.
 *
 * Zero padding, field width, and precision are also supported.
 *
 * If the library is compiled with 'PRINTF_SUPPORT_LONG' defined, then the
 * long specifier is also supported. Note that this will pull in some long
 * math routines (pun intended!) and thus make your executable noticeably
 * longer. Likewise with 'PRINTF_LONG_LONG_SUPPORT' for the long long
 * specifier, and with 'PRINTF_SIZE_T_SUPPORT' for the size_t specifier.
 *
 * The memory footprint of course depends on the target CPU, compiler and
 * compiler options, but a rough guesstimate (based on a H8S target) is about
 * 1.4 kB for code and some twenty 'int's and 'char's, say 60 bytes of stack
 * space. Not too bad. Your mileage may vary. By hacking the source code you
 * can get rid of some hundred bytes, I'm sure, but personally I feel the
 * balance of functionality and flexibility versus  code size is close to
 * optimal for many embedded systems.
 *
 * To use the printf, you need to supply your own character output function,
 * something like :
 *
 * void putc ( void* p, char c) { while (!SERIAL_PORT_EMPTY) ;
 * SERIAL_PORT_TX_REGISTER = c; }
 *
 * Before you can call printf, you need to initialize it to use your character
 * output function with something like:
 *
 * init_printf(NULL,putc);
 *
 * Notice the 'NULL' in 'init_printf' and the parameter 'void* p' in 'putc',
 * the NULL (or any pointer) you pass into the 'init_printf' will eventually
 * be passed to your 'putc' routine. This allows you to pass some storage
 * space (or anything really) to the character output function, if necessary.
 * This is not often needed but it was implemented like that because it made
 * implementing the sprintf function so neat (look at the source code).
 *
 * The code is re-entrant, except for the 'init_printf' function, so it is
 * safe to call it from interrupts too, although this may result in mixed
 * output. If you rely on re-entrancy, take care that your 'putc' function is
 * re-entrant!
 *
 * The printf and sprintf functions are actually macros that translate to
 * 'tfp_printf' and 'tfp_sprintf' when 'TINYPRINTF_OVERRIDE_LIBC' is set
 * (default). Setting it to 0 makes it possible to use them along with
 * 'stdio.h' printf's in a single source file. When 'TINYPRINTF_OVERRIDE_LIBC'
 * is set, please note that printf/sprintf are not function-like macros, so if
 * you have variables or struct members with these names, things will explode
 * in your face.  Without variadic macros this is the best we can do to wrap
 * these function. If it is a problem, just give up the macros and use the
 * functions directly, or rename them.
 *
 * It is also possible to avoid defining tfp_printf and/or tfp_sprintf by
 * clearing 'TINYPRINTF_DEFINE_TFP_PRINTF' and/or
 * 'TINYPRINTF_DEFINE_TFP_SPRINTF' to 0. This allows for example to export
 * only tfp_format, which is at the core of all the other functions.
 *
 * For further details see source code.
 *
 * regs Kusti, 23.10.2004
 */

#ifndef _BSP_PRINTF_H
#define _BSP_PRINTF_H

#include <stdarg.h>
#include <stddef.h>

/* Global configuration */

/* Set this to 0 if you do not want to provide tfp_printf */
#ifndef TINYPRINTF_DEFINE_TFP_PRINTF
#define TINYPRINTF_DEFINE_TFP_PRINTF 1
#endif

/**
 * Set this to 0 if you do not want to provide
 * tfp_sprintf/snprintf/vsprintf/vsnprintf
 */
#ifndef TINYPRINTF_DEFINE_TFP_SPRINTF
#define TINYPRINTF_DEFINE_TFP_SPRINTF 1
#endif

/**
 * Set this to 0 if you do not want tfp_printf and
 * tfp_{vsn,sn,vs,s}printf to be also available as
 * printf/{vsn,sn,vs,s}printf
 */
#ifndef TINYPRINTF_OVERRIDE_LIBC
#define TINYPRINTF_OVERRIDE_LIBC 0
#endif

/* Optional external types dependencies */

/* Declarations */

#if defined(__GNUC__)
#define _TFP_SPECIFY_PRINTF_FMT(fmt_idx, arg1_idx) \
    __attribute__((format(printf, fmt_idx, arg1_idx)))
#else
#define _TFP_SPECIFY_PRINTF_FMT(fmt_idx, arg1_idx)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*putcf)(void*, char);

/**
 * 'tfp_format' really is the central function for all tinyprintf. For
 * each output character after formatting, the 'putf' callback is
 * called with 2 args:
 *   - an arbitrary void* 'putp' param defined by the user and
 *     passed unmodified from 'tfp_format',
 *   - the character.
 * The 'tfp_printf' and 'tfp_sprintf' functions simply define their own
 * callback and pass to it the right 'putp' it is expecting.
 */
void tfp_format(void *putp, putcf putf, const char *fmt, va_list va);

#if TINYPRINTF_DEFINE_TFP_SPRINTF
int tfp_vsnprintf(char *str, size_t size, const char *fmt, va_list ap);
int tfp_snprintf(char *str, size_t size, const char *fmt, ...)
    _TFP_SPECIFY_PRINTF_FMT(3, 4);
int tfp_vsprintf(char *str, const char *fmt, va_list ap);
int tfp_sprintf(char *str, const char *fmt, ...) _TFP_SPECIFY_PRINTF_FMT(2, 3);
#if TINYPRINTF_OVERRIDE_LIBC
#define vsnprintf tfp_vsnprintf
#define snprintf tfp_snprintf
#define vsprintf tfp_vsprintf
#define sprintf tfp_sprintf
#endif
#endif

#if TINYPRINTF_DEFINE_TFP_PRINTF
void init_printf(void *putp, putcf putf);
void tfp_printf(char *fmt, ...) _TFP_SPECIFY_PRINTF_FMT(1, 2);
#if TINYPRINTF_OVERRIDE_LIBC
#define printf tfp_printf

#ifdef __cplusplus
#include <forward_list>
namespace std
{
    template <typename... Args>
    auto tfp_printf(Args&&... args) -> decltype(::tfp_printf(std::forward<Args>(args)...))
    {
        return ::tfp_printf(std::forward<Args>(args)...);
    }
}
#endif
#endif
#endif

int printk(const char *format, ...) _TFP_SPECIFY_PRINTF_FMT(1, 2);

#ifdef __cplusplus
}
#endif

#endif /* _BSP_PRINTF_H */

