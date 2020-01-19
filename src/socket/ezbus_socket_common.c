/*****************************************************************************
* Copyright © 2019-2020 Mike Sharkey <mike.sharkey@mineairquality.com>       *
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
#include <ezbus_socket_common.h>
#include <ezbus_socket.h>

static EZBUS_ERR  global_socket_err=EZBUS_ERR_OKAY;

extern size_t ezbus_socket_max( void )
{
    return EZBUS_MAX_SOCKETS;
}

extern ezbus_socket_state_t* ezbus_socket_at( size_t index )
{
    if ( index < ezbus_socket_max() )
    {
        return &ezbus_sockets[index];
    }
    return NULL;
}

extern ezbus_mac_t* ezbus_socket_mac( ezbus_socket_t socket )
{
    if ( socket >= 0 && socket < ezbus_socket_max() )
    {
        ezbus_socket_state_t* socket_state = ezbus_socket_at( socket );
        return socket_state->mac;
    }
    else
    {
        global_socket_err=EZBUS_ERR_RANGE;
        return NULL;
    }
}

extern ezbus_packet_t* ezbus_socket_tx_packet ( ezbus_socket_t socket )
{
    ezbus_socket_state_t* socket_state = ezbus_socket_at( socket );
    if ( socket_state != NULL )
    {
        return &socket_state->tx_packet;
    }
    global_socket_err=EZBUS_ERR_RANGE;
    return NULL;
}

extern ezbus_packet_t* ezbus_socket_rx_packet ( ezbus_socket_t socket )
{
    if ( ezbus_socket_is_open(socket) )
    {
        ezbus_mac_t* mac = ezbus_socket_mac( socket );
        return ezbus_mac_get_receiver_packet( mac );
    }
    global_socket_err=EZBUS_ERR_NOTREADY;
    return NULL;
}

extern uint8_t ezbus_socket_tx_seq( ezbus_socket_t socket )
{
    if ( ezbus_socket_is_open(socket) )
    {
        ezbus_socket_state_t* socket_state = ezbus_socket_at( socket );
        return socket_state->tx_seq;
    }
    global_socket_err=EZBUS_ERR_NOTREADY;
    return 0;
}

extern uint8_t ezbus_socket_rx_seq( ezbus_socket_t socket )
{
    if ( ezbus_socket_is_open(socket) )
    {
        ezbus_socket_state_t* socket_state = ezbus_socket_at( socket );
        return socket_state->rx_seq;
    }
    global_socket_err=EZBUS_ERR_NOTREADY;
    return 0;
}

extern void ezbus_socket_set_tx_seq( ezbus_socket_t socket, uint8_t seq)
{
    if ( ezbus_socket_is_open(socket) )
    {
        ezbus_socket_state_t* socket_state = ezbus_socket_at( socket );
        socket_state->tx_seq = seq;
    }
    global_socket_err=EZBUS_ERR_NOTREADY;
}

extern void ezbus_socket_set_rx_seq( ezbus_socket_t socket, uint8_t seq)
{
    if ( ezbus_socket_is_open(socket) )
    {
        ezbus_socket_state_t* socket_state = ezbus_socket_at( socket );
        socket_state->rx_seq = seq;
    }
    global_socket_err=EZBUS_ERR_NOTREADY;
}


extern EZBUS_ERR ezbus_socket_err( ezbus_socket_t socket )
{
    if ( socket >= 0 && socket < ezbus_socket_max() )
    {
        ezbus_socket_state_t* socket_state = ezbus_socket_at( socket );
        if ( socket_state->err != EZBUS_ERR_OKAY )
        {
            return socket_state->err;
        }
    }
    else
    {
        global_socket_err=EZBUS_ERR_RANGE;
    }
    return global_socket_err;
}

extern void ezbus_socket_reset_err( ezbus_socket_t socket )
{
    if ( socket >= 0 && socket < ezbus_socket_max() )
    {
        ezbus_socket_state_t* socket_state = ezbus_socket_at( socket );
        socket_state->err = global_socket_err = EZBUS_ERR_OKAY;
    }
    else
    {
        global_socket_err=EZBUS_ERR_RANGE;
    }
}


