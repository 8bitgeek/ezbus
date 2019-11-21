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
#include <ezbus_packet_transceiver.h>
#include <ezbus_hex.h>

static bool ezbus_packet_transceiver_tx_callback( ezbus_transmitter_t* packet_transmitter, void* arg );
static bool ezbus_packet_transceiver_rx_callback( ezbus_receiver_t*    packet_receiver,    void* arg );


void ezbus_packet_transceiver_init( ezbus_packet_transceiver_t* packet_transceiver, ezbus_port_t* port )
{
    ezbus_packet_transmitter_init ( &packet_transceiver->packet_transmitter, port, eezbus_packet_transceiver_tx_callback, packet_transceiver );
    ezbus_packet_receiver_init    ( &packet_transceiver->packet_receiver,    port, eezbus_packet_transceiver_rx_callback, packet_transceiver );
}

void ezbus_packet_transceiver_run  ( ezbus_packet_transceiver_t* packet_transceiver )
{
    ezbus_packet_receiver_run( packet_transceiver->packet_receiver );
    ezbus_packet_receiver_run( packet_transceiver->packet_transmitter );    
}


static bool ezbus_packet_transceiver_tx_callback( ezbus_transmitter_t* packet_transmitter, void* arg )
{
    bool rc = false;
    ezbus_packet_transceiver_t* packet_transceiver = (ezbus_packet_transceiver_t*)arg;

    switch( ezbus_packet_transmitter_get_state( packet_transmitter ) )
    {
        case transmitter_state_empty:
             /*
             * In the event the callback would like to transmit, it should store a packet, and return 'true'.
             */
            rc = ezbus_level1_transmitter_callback( packet_transceiver );
            break;
        case transmitter_state_full:
            /*
             * Timeout if we remain in this state indicates stalled token condition ??
             */
            break;
        case transmitter_state_send:
            /* 
             * callback should examine fault, return true to reset fault, and/or take remedial action. 
             */
           break;
        case transmitter_state_give_token:
            /* 
             * callback should give up the token without disturning the contents of the transmitter.
             * i.e. it should use port directly to transmit.. 
             * callback should return 'true' upon giving up token.
             */
            break;
        case transmitter_state_wait_ack:
            /* 
             * callback should determine if the packet requires an acknowledge, and return 'true' when it arrives. 
             * else upon timeout or ack not required, then callback should reset transmitter state accordingly.
             */
            break;
    }
    return rc;
}

static bool ezbus_packet_transceiver_rx_callback( ezbus_receiver_t* packet_receiver, void* arg )
{
    ezbus_packet_transceiver_t* packet_transceiver = (zbus_packet_transceiver_t*)arg;

    switch ( ezbus_packet_receiver_get_state( packet_receiver ) )
    {
        case receiver_state_empty:
            break;
        case receiver_state_full:
            break;
    }
}
