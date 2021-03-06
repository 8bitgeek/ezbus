/*****************************************************************************
* Copyright © 2019-2020 Mike Sharkey <mike@8bitgeek.net>                     *
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
#ifndef EZBUS_MAC_TOKEN_H_
#define EZBUS_MAC_TOKEN_H_

#include <ezbus_types.h>
#include <ezbus_mac.h>
#include <ezbus_mac_timer.h>

typedef struct _ezbus_mac_token_t
{
    ezbus_timer_t   ring_timer;
    uint32_t        ring_count;
    bool            acquired;
} ezbus_mac_token_t;

#ifdef __cplusplus
extern "C" {
#endif

extern void     ezbus_mac_token_init                ( ezbus_mac_t* mac );
extern void     ezbus_mac_token_run                 ( ezbus_mac_t* mac );

extern void     ezbus_mac_token_reset               ( ezbus_mac_t* mac );
extern void     ezbus_mac_token_acquire             ( ezbus_mac_t* mac );
extern void     ezbus_mac_token_relinquish          ( ezbus_mac_t* mac );
extern bool     ezbus_mac_token_acquired            ( ezbus_mac_t* mac );

extern uint32_t ezbus_mac_token_ring_count          ( ezbus_mac_t* mac );
extern bool     ezbus_mac_token_ring_count_timeout  ( ezbus_mac_t* mac, uint32_t start_count, uint32_t timeout_count );

extern uint32_t ezbus_mac_token_ring_time           ( ezbus_mac_t* mac );
extern uint32_t ezbus_mac_token_retransmit_time     ( ezbus_mac_t* mac );

#ifdef __cplusplus
}
#endif

#endif /* EZBUS_MAC_TOKEN_H_ */
