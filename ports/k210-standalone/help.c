/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2016 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "py/builtin.h"

const char kendryte_k210_help_text[] =
"Welcome to MicroPython on the Sipeed Maix One!\n"
"\n"
"For generic online docs please visit http://docs.micropython.org/\n"
"\n"
"Official website : www.sipeed.com\n"
"\n"
"For access to the hardware use the 'machine' module:\n"
"For access to the system use the 'os' module:\n"
"For access to the fpioa use the  'machine.fpioa' module:\n"
"For access to the burner model use the  'machine.burner' module:\n"
"For access to the ov2640 use the  'machine.ov2640' module:\n"
"For access to the pin use the  'machine.pin' module:\n"
"For access to the pwm use the  'machine.pwm' module:\n"
"For access to the spiflash use the  'machine.spiflash' module:\n"
"For access to the st7789 use the  'machine.st7789' module:\n"
"For access to the timer use the  'machine.timer' module:\n"
"For access to the uart use the  'machine.uart' module:\n"
"For access to the uarths use the  'machine.uarths' module:\n"
"For access to the zmodem use the  'machine.zmodem' module:\n"
"\n"
"Control commands:\n"
"  CTRL-A        -- on a blank line, enter raw REPL mode\n"
"  CTRL-B        -- on a blank line, enter normal REPL mode\n"
"  CTRL-C        -- interrupt a running program\n"
"  CTRL-D        -- on a blank line, do a soft reset of the board\n"
"  CTRL-E        -- on a blank line, enter paste mode\n"
"\n"
"For further help on a specific object, type help(obj)\n"
"For a list of available modules, type help('modules')\n"
;
