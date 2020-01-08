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
#include <ezbus_mac_arbiter_transmit.h>
#include <ezbus_mac_transmitter.h>
#include <ezbus_mac_coldboot.h>
#include <ezbus_mac_warmboot.h>
#include <ezbus_mac_token.h>
#include <ezbus_mac_peers.h>
#include <ezbus_packet.h>
#include <ezbus_hex.h>
#include <ezbus_log.h>


extern void ezbus_mac_arbiter_transmit_init  ( ezbus_mac_t* mac )
{
    ezbus_mac_arbiter_transmit_t* arbiter_transmit = ezbus_mac_get_arbiter_transmit( mac );

    ezbus_timer_init( &arbiter_transmit->ack_tx_timer );
    ezbus_timer_set_period( &arbiter_transmit->ack_tx_timer, ezbus_mac_token_ring_time(mac)*4 ); // FIXME *4 ??
}

extern void ezbus_mac_arbiter_transmit_run( ezbus_mac_t* mac )
{
    /* FIXME insert code here */
}



extern void  ezbus_mac_coldboot_signal_silent_start( ezbus_mac_t* mac )
{
    //ezbus_mac_arbiter_set_state( mac, mac_arbiter_state_offline );
    //ezbus_log( EZBUS_LOG_COLDBOOT, "ezbus_mac_coldboot_signal_silent_start\n" );
}

extern void  ezbus_mac_coldboot_signal_silent_continue( ezbus_mac_t* mac )
{
    //ezbus_log( EZBUS_LOG_COLDBOOT, "ezbus_mac_coldboot_signal_silent_continue\n" );
}

extern void  ezbus_mac_coldboot_signal_silent_stop( ezbus_mac_t* mac )
{
    ezbus_log( EZBUS_LOG_COLDBOOT, "ezbus_mac_coldboot_signal_silent_stop\n" );
}


extern void  ezbus_mac_coldboot_signal_start( ezbus_mac_t* mac )
{
    ezbus_mac_arbiter_set_state( mac, mac_arbiter_state_coldboot );
    ezbus_log( EZBUS_LOG_COLDBOOT, "ezbus_mac_coldboot_signal_start\n" );
}

extern void  ezbus_mac_coldboot_signal_continue( ezbus_mac_t* mac )
{
    if ( ezbus_mac_transmitter_get_state( mac ) == transmitter_state_empty )
    {
        ezbus_packet_t packet;

        ezbus_log( EZBUS_LOG_COLDBOOT, "ezbus_mac_coldboot_signal_continue\n" );

        ezbus_packet_init     ( &packet );
        ezbus_packet_set_type ( &packet, packet_type_coldboot );
        ezbus_packet_set_seq  ( &packet, ezbus_mac_coldboot_get_seq( mac ) );
        ezbus_packet_set_src  ( &packet, &ezbus_self_address );

        ezbus_log( EZBUS_LOG_BOOTSTATE, "%ccoldboot> %s %3d | ", ezbus_mac_token_acquired(mac)?'*':' ', ezbus_address_string( ezbus_packet_src( &packet ) ), ezbus_packet_seq( &packet ) );
        #if EZBUS_LOG_BOOTSTATE
            ezbus_mac_peers_log( mac );
        #endif

        ezbus_mac_transmitter_put( mac, &packet );
    }
}

extern void  ezbus_mac_coldboot_signal_stop( ezbus_mac_t* mac )
{
    ezbus_log( EZBUS_LOG_COLDBOOT, "ezbus_mac_coldboot_signal_stop\n" );
}

extern void  ezbus_mac_coldboot_signal_dominant( ezbus_mac_t* mac )
{
    ezbus_mac_warmboot_set_state( mac, state_warmboot_start );
    ezbus_mac_coldboot_set_state( mac, state_coldboot_silent_start);
    ezbus_mac_arbiter_set_state( mac, mac_arbiter_state_warmboot );
    ezbus_log( EZBUS_LOG_DOMINANT, "ezbus_mac_coldboot_signal_dominant\n" );
}



extern void ezbus_mac_warmboot_signal_start( ezbus_mac_t* mac )
{
    ezbus_log( EZBUS_LOG_WARMBOOT, "ezbus_mac_warmboot_signal_start\n" );
}

extern void ezbus_mac_warmboot_signal_continue( ezbus_mac_t* mac )
{
    ezbus_log( EZBUS_LOG_WARMBOOT, "ezbus_mac_warmboot_signal_continue\n" );
}

extern void ezbus_mac_warmboot_signal_stop( ezbus_mac_t* mac )
{
    if ( ezbus_mac_transmitter_get_state( mac ) == transmitter_state_empty )
    {
        ezbus_packet_t packet;

        ezbus_log( EZBUS_LOG_WARMBOOT, "ezbus_mac_warmboot_signal_stop\n" );

        ezbus_packet_init     ( &packet );
        ezbus_packet_set_type ( &packet, packet_type_warmboot_rq );
        ezbus_packet_set_seq  ( &packet, ezbus_mac_warmboot_get_seq( mac ) );
        ezbus_packet_set_src  ( &packet, &ezbus_self_address );

        ezbus_log( EZBUS_LOG_BOOTSTATE, "%warmboot> %s %3d | ", ezbus_mac_token_acquired(mac)?'*':' ', ezbus_address_string( ezbus_packet_src( &packet ) ), ezbus_packet_seq( &packet ) );
        #if EZBUS_LOG_BOOTSTATE
            ezbus_mac_peers_log( mac );
        #endif

        ezbus_mac_transmitter_put( mac, &packet );
        ezbus_mac_coldboot_set_state( mac, state_coldboot_silent_start);
    }
}

extern void ezbus_mac_warmboot_signal_finished( ezbus_mac_t* mac )
{
    ezbus_log( EZBUS_LOG_WARMBOOT, "ezbus_mac_warmboot_signal_finished\n" );
}



extern void ezbus_mac_warmboot_signal_idle( ezbus_mac_t* mac )
{
    //ezbus_log( EZBUS_LOG_TRANSMITTER, "ezbus_mac_warmboot_signal_idle\n" );
}

extern void ezbus_mac_transmitter_signal_empty( ezbus_mac_t* mac )
{
    ezbus_log( EZBUS_LOG_TRANSMITTER, "ezbus_mac_transmitter_signal_empty\n" );
}

extern void ezbus_mac_transmitter_signal_full( ezbus_mac_t* mac )
{   
    ezbus_log( EZBUS_LOG_TRANSMITTER, "ezbus_mac_transmitter_signal_full\n" );
}

extern void ezbus_mac_transmitter_signal_sent( ezbus_mac_t* mac )
{
    ezbus_log( EZBUS_LOG_TRANSMITTER, "ezbus_mac_transmitter_signal_sent\n" );
}

extern void ezbus_mac_transmitter_signal_wait( ezbus_mac_t* mac )
{
   
    ezbus_log( EZBUS_LOG_TRANSMITTER, "ezbus_mac_transmitter_signal_wait\n" );
}

extern void ezbus_mac_transmitter_signal_fault( ezbus_mac_t* mac )
{
    ezbus_log( EZBUS_LOG_TRANSMITTER, "ezbus_mac_transmitter_signal_fault %s\n",ezbus_fault_str( ezbus_mac_transmitter_get_err( mac ) ) );
    ezbus_mac_transmitter_set_state( mac, transmitter_state_empty );
}
