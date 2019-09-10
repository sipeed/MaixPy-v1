/*===================================================================*/
/*                                                                   */
/*  InfoNES_Types.h : Type definitions for InfoNES                   */
/*                                                                   */
/*  2000/5/4    InfoNES Project ( based on pNesX )                   */
/*                                                                   */
/*===================================================================*/


#ifndef InfoNES_TYPES_H_INCLUDED
#define InfoNES_TYPES_H_INCLUDED

#include "py/mpconfig.h"

/*-------------------------------------------------------------------*/
/*  Type definition                                                  */
/*-------------------------------------------------------------------*/
#ifndef NES_DWORD
typedef unsigned long  NES_DWORD;
#endif /* !NES_DWORD */

#ifndef WORD
typedef unsigned short WORD;
#endif /* !WORD */

#ifndef BYTE
typedef unsigned char  BYTE;
#endif /* !BYTE */

/*-------------------------------------------------------------------*/
/*  NULL definition                                                  */
/*-------------------------------------------------------------------*/
#ifndef NULL
#define NULL  0
#endif /* !NULL */

#endif /* !InfoNES_TYPES_H_INCLUDED */
