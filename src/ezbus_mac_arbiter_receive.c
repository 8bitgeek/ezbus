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
#include <ezbus_mac_arbiter_receive.h>
#include <ezbus_mac_transmitter.h>
#include <ezbus_mac_receiver.h>
#include <ezbus_mac_coldboot.h>
#include <ezbus_mac_warmboot.h>
#include <ezbus_mac_token.h>
#include <ezbus_mac_peers.h>
#include <ezbus_address.h>
#include <ezbus_packet.h>
#include <ezbus_peer.h>
#include <ezbus_hex.h>
#include <ezbus_log.h>

static void do_receiver_packet_type_reset           ( ezbus_mac_t* mac, ezbus_packet_t* packet );
static void do_receiver_packet_type_take_token      ( ezbus_mac_t* mac, ezbus_packet_t* packet );
static void do_receiver_packet_type_give_token      ( ezbus_mac_t* mac, ezbus_packet_t* packet );
static void do_receiver_packet_type_parcel          ( ezbus_mac_t* mac, ezbus_packet_t* packet );
static void do_receiver_packet_type_speed           ( ezbus_mac_t* mac, ezbus_packet_t* packet );
static void do_receiver_packet_type_ack             ( ezbus_mac_t* mac, ezbus_packet_t* packet );
static void do_receiver_packet_type_nack            ( ezbus_mac_t* mac, ezbus_packet_t* packet );
static void do_receiver_packet_type_coldboot        ( ezbus_mac_t* mac, ezbus_packet_t* packet );
static void do_receiver_packet_type_warmboot_rq     ( ezbus_mac_t* mac, ezbus_packet_t* packet );
static void do_receiver_packet_type_warmboot_rp     ( ezbus_mac_t* mac, ezbus_packet_t* packet );
static void do_receiver_packet_type_warmboot_ak     ( ezbus_mac_t* mac, ezbus_packet_t* packet );

static void ezbus_mac_arbiter_warmboot_send_reply   ( ezbus_timer_t* timer, void* arg );
static void ezbus_mac_arbiter_warmboot_send_ack     ( ezbus_mac_t* mac, ezbus_packet_t* rx_packet );
static void ezbus_mac_arbiter_receive_sniff         ( ezbus_mac_t* mac, ezbus_packet_t* rx_packet );


extern void ezbus_mac_arbiter_receive_init  ( ezbus_mac_t* mac )
{
    ezbus_mac_arbiter_receive_t* arbiter_receive = ezbus_mac_get_arbiter_receive( mac );

    ezbus_timer_init( &arbiter_receive->warmboot_timer );
    ezbus_timer_set_key( &arbiter_receive->warmboot_timer, "warmboot_timer" );
    ezbus_timer_set_callback( &arbiter_receive->warmboot_timer, ezbus_mac_arbiter_warmboot_send_reply, mac );

    ezbus_timer_init( &arbiter_receive->ack_rx_timer );
    ezbus_timer_set_key( &arbiter_receive->ack_rx_timer, "ack_rx_timer" );
    ezbus_timer_set_callback( &arbiter_receive->warmboot_timer, ezbus_mac_arbiter_warmboot_send_reply, mac );
    ezbus_timer_set_period( &arbiter_receive->ack_rx_timer, ezbus_mac_token_ring_time(mac)*4 ); // FIXME *4 ??
}


extern void ezbus_mac_arbiter_receive_run( ezbus_mac_t* mac )
{
    ezbus_mac_arbiter_receive_t* arbiter_receive = ezbus_mac_get_arbiter_receive( mac );
    ezbus_timer_run( &arbiter_receive->warmboot_timer );
    ezbus_timer_run( &arbiter_receive->ack_rx_timer );
}


static void do_receiver_packet_type_reset( ezbus_mac_t* mac, ezbus_packet_t* packet )
{
    /* FIXME - write code here */
}

static void do_receiver_packet_type_take_token( ezbus_mac_t* mac, ezbus_packet_t* packet )
{
    ezbus_mac_arbiter_receive_t* arbiter_receive = ezbus_mac_get_arbiter_receive( mac );
    arbiter_receive->warmboot_seq=0;
    if ( ezbus_address_compare( ezbus_packet_dst(packet), &ezbus_self_address ) != 0 )
    {
        ezbus_mac_token_relinquish( mac );
    }
}

static void do_receiver_packet_type_give_token( ezbus_mac_t* mac, ezbus_packet_t* packet )
{
    ezbus_mac_arbiter_receive_t* arbiter_receive = ezbus_mac_get_arbiter_receive( mac );
    arbiter_receive->warmboot_seq=0;
    ezbus_mac_arbiter_receive_signal_token( mac, packet );
}

static void do_receiver_packet_type_parcel( ezbus_mac_t* mac, ezbus_packet_t* packet )
{
    if ( ezbus_address_compare( ezbus_packet_dst(packet), &ezbus_self_address ) == 0 )
    {
        ezbus_mac_arbiter_receive_signal_parcel( mac, packet );
    }
}

static void do_receiver_packet_type_speed( ezbus_mac_t* mac, ezbus_packet_t* packet )
{
    /* FIXME - write code here */
}

static void do_receiver_packet_type_ack( ezbus_mac_t* mac, ezbus_packet_t* packet )
{
    //ezbus_log( 1, "ack 1\n");
    if ( ezbus_address_compare( ezbus_packet_dst(packet), &ezbus_self_address ) == 0 )
    {
        ezbus_packet_t* tx_packet = ezbus_mac_get_transmitter_packet( mac );
        //ezbus_log( 1, "ack 2\n");
        if ( ezbus_address_compare( ezbus_packet_src(packet), ezbus_packet_dst(tx_packet) ) == 0 )
        {
            //ezbus_log( 1, "ack 3\n");
            if ( ezbus_packet_seq(packet) == ezbus_packet_seq(tx_packet) )
            {
                //ezbus_log( 1, "ack 4\n");
                ezbus_mac_transmitter_reset( mac );
            }
            else
            {
                //ezbus_log( 1, "ack 5\n");
                /* FIXME - throw a fault here? */
                ezbus_log( EZBUS_LOG_ARBITER, "recv: ack seq mismatch\n");
            }
        }
        else
        {
            /* FIXME - throw a fauld herre? */
            ezbus_log( EZBUS_LOG_ARBITER, "recv: ack address mismatch\n" );
        }
    }
}

static void do_receiver_packet_type_nack( ezbus_mac_t* mac, ezbus_packet_t* packet )
{
    if ( ezbus_address_compare( ezbus_packet_dst(packet), &ezbus_self_address ) == 0 )
    {
        ezbus_packet_t* tx_packet = ezbus_mac_get_transmitter_packet( mac );
        if ( ezbus_address_compare( ezbus_packet_src(packet), ezbus_packet_dst(tx_packet) ) == 0 )
        {
            if ( ezbus_packet_seq(packet) == ezbus_packet_seq(tx_packet) )
            {
                /* FIXME - cap # re-tries */
                ezbus_mac_transmitter_set_state( mac, transmitter_state_send );
            }
            else
            {
                /* FIXME - throw a fault here? */
                ezbus_log( EZBUS_LOG_ARBITER, "recv: ack seq mismatch\n");
            }
        }
        else
        {
            /* FIXME - throw a fault here? */
            ezbus_log( EZBUS_LOG_ARBITER, "recv: ack address mismatch\n" );
        }
    }
}


static void do_receiver_packet_type_coldboot( ezbus_mac_t* mac, ezbus_packet_t* packet )
{
    ezbus_peer_t peer;
    ezbus_mac_coldboot_t* boot = ezbus_mac_get_coldboot( mac );
    ezbus_mac_arbiter_receive_t* arbiter_receive = ezbus_mac_get_arbiter_receive( mac );

    ezbus_mac_warmboot_set_state( mac, state_warmboot_idle );

    if ( (int)ezbus_mac_coldboot_get_state( mac ) < (int)state_coldboot_silent_stop )
    {
        ezbus_mac_coldboot_set_state( mac, state_coldboot_silent_stop );
    }

    ezbus_log( EZBUS_LOG_BOOTSTATE, "%ccoldboot <%s %3d | ", ezbus_mac_token_acquired(mac)?'*':' ', ezbus_address_string( ezbus_packet_src( packet ) ), ezbus_packet_seq( packet ) );
    ezbus_mac_peers_log( mac );
    
    arbiter_receive->warmboot_seq=0;
    ezbus_peer_init( &peer, ezbus_packet_src( packet ), ezbus_packet_seq( packet ) );

    if ( ezbus_address_compare( &ezbus_self_address, ezbus_packet_src( packet ) ) > 0 )
    {
        ezbus_timer_stop( &boot->coldboot_timer );
        ezbus_mac_coldboot_set_state( mac, state_coldboot_silent_start );
    }
}

/**
 * @brief Receive a wb request from src, and this node's wb seq# does not match the rx seq#,
 * then we must reply. If the seq# matches, then we've already been acknowledged during this
 * session identified by seq#.
 * 
 */
static void do_receiver_packet_type_warmboot_rq( ezbus_mac_t* mac, ezbus_packet_t* packet )
{
    ezbus_mac_arbiter_receive_t* arbiter_receive = ezbus_mac_get_arbiter_receive( mac );

    if ( ezbus_address_compare( ezbus_packet_dst(packet), &ezbus_broadcast_address ) == 0 )
    {
        if ( arbiter_receive->warmboot_seq != ezbus_packet_seq( packet ) )
        {
            ezbus_timer_stop( &arbiter_receive->warmboot_timer );
            ezbus_timer_set_period  ( 
                                        &arbiter_receive->warmboot_timer, 
                                        ezbus_platform_random( EZBUS_WARMBOOT_TIMER_MIN, EZBUS_WARMBOOT_TIMER_MAX ) 
                                    );
            ezbus_timer_restart( &arbiter_receive->warmboot_timer );
        }
        else
        {
            ezbus_timer_stop( &arbiter_receive->warmboot_timer );
        }
    }
}


/**
 * @brief I am the src of the warmboot, and a node has replied.
 */
static void do_receiver_packet_type_warmboot_rp( ezbus_mac_t* mac, ezbus_packet_t* packet )
{
    if ( ezbus_address_compare( ezbus_packet_dst(packet), &ezbus_self_address ) == 0 )
    {   
        ezbus_mac_arbiter_warmboot_send_ack( mac, packet );
    }
}

/**
 * @brief Receive an wb acknolegment from src, and disable replying to this wb sequence#
 */
static void do_receiver_packet_type_warmboot_ak( ezbus_mac_t* mac, ezbus_packet_t* packet )
{
    ezbus_mac_arbiter_receive_t* arbiter_receive = ezbus_mac_get_arbiter_receive( mac );

    if ( ezbus_address_compare( ezbus_packet_dst(packet), &ezbus_self_address ) == 0 )
    {       
        /* acknowleged, stop replying to this seq# */
        arbiter_receive->warmboot_seq=ezbus_packet_seq( packet );
        ezbus_timer_stop( &arbiter_receive->warmboot_timer );
    }
}

static void ezbus_mac_arbiter_warmboot_send_reply( ezbus_timer_t* timer, void* arg )
{
    ezbus_packet_t tx_packet;
    ezbus_mac_t* mac = (ezbus_mac_t*)arg;
    ezbus_mac_arbiter_receive_t* arbiter_receive = ezbus_mac_get_arbiter_receive( mac );
    ezbus_packet_t* rx_packet = ezbus_mac_get_receiver_packet( mac );

    ezbus_timer_stop( &arbiter_receive->warmboot_timer );

    ezbus_packet_init     ( &tx_packet );
    ezbus_packet_set_type ( &tx_packet, packet_type_warmboot_rp );
    ezbus_packet_set_seq  ( &tx_packet, ezbus_packet_seq( rx_packet ) );
    ezbus_packet_set_src  ( &tx_packet, &ezbus_self_address );
    ezbus_packet_set_dst  ( &tx_packet, ezbus_packet_src( rx_packet ) );

    ezbus_mac_transmitter_put( mac, &tx_packet );
}


static void ezbus_mac_arbiter_warmboot_send_ack( ezbus_mac_t* mac, ezbus_packet_t* rx_packet )
{
    ezbus_packet_t tx_packet;
    ezbus_mac_arbiter_receive_t* arbiter_receive = ezbus_mac_get_arbiter_receive( mac );

    ezbus_timer_stop( &arbiter_receive->warmboot_timer );

    ezbus_packet_init     ( &tx_packet );
    ezbus_packet_set_type ( &tx_packet, packet_type_warmboot_ak );
    ezbus_packet_set_seq  ( &tx_packet, ezbus_packet_seq( rx_packet ) );
    ezbus_packet_set_src  ( &tx_packet, &ezbus_self_address );
    ezbus_packet_set_dst  ( &tx_packet, ezbus_packet_src( rx_packet ) );

    ezbus_mac_transmitter_put( mac, &tx_packet );
}

static void ezbus_mac_arbiter_receive_sniff( ezbus_mac_t* mac, ezbus_packet_t* rx_packet )
{
    ezbus_peer_t peer;
    ezbus_peer_init( &peer, ezbus_packet_src( rx_packet ), ezbus_packet_seq( rx_packet ) );
    ezbus_mac_peers_insort( mac, &peer );

    ezbus_peer_init( &peer, ezbus_packet_dst( rx_packet ), ezbus_packet_seq( rx_packet ) );
    ezbus_mac_peers_insort( mac, &peer );
}

extern void ezbus_mac_receiver_signal_full  ( ezbus_mac_t* mac )
{
    ezbus_packet_t* packet  = ezbus_mac_get_receiver_packet( mac );
    
    ezbus_log( EZBUS_LOG_RECEIVER, "ezbus_mac_receiver_signal_full\n" );

    switch( ezbus_packet_type( packet ) )
    {
        case packet_type_reset:       do_receiver_packet_type_reset       ( mac, packet ); break;
        case packet_type_take_token:  do_receiver_packet_type_take_token  ( mac, packet ); break;
        case packet_type_give_token:  do_receiver_packet_type_give_token  ( mac, packet ); break;
        case packet_type_parcel:      do_receiver_packet_type_parcel      ( mac, packet ); break;
        case packet_type_speed:       do_receiver_packet_type_speed       ( mac, packet ); break;
        case packet_type_ack:         do_receiver_packet_type_ack         ( mac, packet ); break;
        case packet_type_nack:        do_receiver_packet_type_nack        ( mac, packet ); break;
        case packet_type_coldboot:    do_receiver_packet_type_coldboot    ( mac, packet ); break;
        case packet_type_warmboot_rq: do_receiver_packet_type_warmboot_rq ( mac, packet ); break;
        case packet_type_warmboot_rp: do_receiver_packet_type_warmboot_rp ( mac, packet ); break;
        case packet_type_warmboot_ak: do_receiver_packet_type_warmboot_ak ( mac, packet ); break;
    }

    ezbus_mac_arbiter_receive_sniff( mac, packet );
    ezbus_mac_coldboot_reset( mac ); 
    ezbus_mac_peers_log( mac );
}


extern void ezbus_mac_receiver_signal_empty( ezbus_mac_t* mac )
{
    //ezbus_log( EZBUS_LOG_RECEIVER, "ezbus_mac_receiver_signal_empty\n" );
}

extern void ezbus_mac_receiver_signal_ack( ezbus_mac_t* mac )
{
}

extern void ezbus_mac_receiver_signal_wait( ezbus_mac_t* mac )
{
    ezbus_log( EZBUS_LOG_RECEIVER, "ezbus_mac_receiver_signal_wait\n" );
    if ( ezbus_mac_transmitter_empty( mac ) )
    {
        ezbus_mac_receiver_set_state( mac, receiver_state_empty );
    }
}

extern void ezbus_mac_receiver_signal_fault( ezbus_mac_t* mac )
{
    if ( ezbus_mac_receiver_get_err( mac ) != EZBUS_ERR_NOTREADY ) // not_ready means rx empty.
    {
        ezbus_log( EZBUS_LOG_RECEIVER, "ezbus_mac_receiver_signal_fault %s\n",ezbus_fault_str( ezbus_mac_receiver_get_err( mac ) ) );
        ezbus_mac_transmitter_reset( mac );
    }
}

