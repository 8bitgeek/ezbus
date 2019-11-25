/*****************************************************************************
* Copyright 2019 Mike Sharkey <mike.sharkey@mineairquality.com>              *
*                                                                            *
* Permission is hereby granted, free of charge, to any person obtaining a    *
* copy of this software and associated documentation files (the "Software"), *
* to deal in the Software without restriction, including without limitation  *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
* and/or sell copies of the Software, and to permit persons to whom the      *
* Software is furnished to do so, subject to the following conditions:       *
*                                                                            *
* The above copyright notice and this permission notice shall be included in *
* all copies or substantial portions of the Software.                        *
*                                                                            *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
* DEALINGS IN THE SOFTWARE.                                                  *
*****************************************************************************/
#ifndef __EZBUS_STRING_H__
#define __EZBUS_STRING_H__

#include <ezbus_platform.h>

typedef struct 
{
	char* 		chars;
} ezbus_string_t;


#ifdef __cplusplus
extern "C" {
#endif

extern void  		ezbus_string_init		( ezbus_string_t* string );
extern void 		ezbus_string_deinit 	( ezbus_string_t* string );

extern const char* 	ezbus_string_str 		( ezbus_string_t* string );
extern const char* 	ezbus_string_length		( ezbus_string_t* string );
extern const char* 	ezbus_string_set_length	( ezbus_string_t* string, uint16_t len );

extern const char* 	ezbus_string_copy		( ezbus_string_t* dst, ezbus_string_t* src );
extern const char* 	ezbus_string_cat		( ezbus_string_t* dst, ezbus_string_t* src );

extern const char*  ezbus_string_left 		( ezbus_string_t* dst, ezbus_string_t* src, uint16_t cnt );
extern const char*  ezbus_string_right 		( ezbus_string_t* dst, ezbus_string_t* src, uint16_t cnt );

extern void  		ezbus_string_import_str ( ezbus_string_t* string, const char* str );
extern void  		ezbus_string_export_str ( ezbus_string_t* string, const char* str, uint16_t max );


#ifdef __cplusplus
}
#endif

#endif



