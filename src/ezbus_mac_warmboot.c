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
#include <ezbus_mac_warmboot.h>
#include <ezbus_mac_peers.h>
#include <ezbus_hex.h>
#include <ezbus_log.h>
#include <ezbus_timer.h>

#define ezbus_mac_warmboot_set_emit_count(boot,c)       ((boot)->emit_count=(c))
#define ezbus_mac_warmboot_get_emit_count(boot)         ((boot)->emit_count)
#define ezbus_mac_warmboot_inc_emit_count(boot)         ezbus_mac_warmboot_set_emit_count(boot,ezbus_mac_warmboot_get_emit_count(boot)+1)

#define ezbus_mac_warmboot_set_emit_seq(boot,c)         ((boot)->emit_count=(c))
#define ezbus_mac_warmboot_get_emit_seq(boot)           ((boot)->emit_count)
#define ezbus_mac_warmboot_inc_emit_seq(boot)           ezbus_mac_warmboot_set_emit_count(boot,ezbus_mac_warmboot_get_emit_count(boot)+1)

#define ezbus_mac_warmboot_get_crc(boot)                (&(boot)->warmboot_crc)

static void ezbus_mac_warmboot_timer_callback_reply     ( ezbus_timer_t* timer, void* arg );
static void ezbus_mac_warmboot_timer_callback_send      ( ezbus_timer_t* timer, void* arg );

static void ezbus_mac_warmboot_init_peers               ( ezbus_mac_t* mac );

static void ezbus_mac_warmboot_signal_peer_seen_src     ( ezbus_mac_t* mac, ezbus_packet_t* packet );
static void ezbus_mac_warmboot_signal_peer_seen_dst     ( ezbus_mac_t* mac, ezbus_packet_t* packet );

static void do_state_warmboot_silent_start              ( ezbus_mac_t* mac );
static void do_state_warmboot_silent_continue           ( ezbus_mac_t* mac );
static void do_state_warmboot_silent_stop               ( ezbus_mac_t* mac );

static void do_state_warmboot_tx_first                  ( ezbus_mac_t* mac );
static void do_state_warmboot_tx_start                  ( ezbus_mac_t* mac );
static void do_state_warmboot_tx_restart                ( ezbus_mac_t* mac );
static void do_state_warmboot_tx_continue               ( ezbus_mac_t* mac );
static void do_state_warmboot_tx_stop                   ( ezbus_mac_t* mac );

static void do_state_warmboot_rx_start                  ( ezbus_mac_t* mac );
static void do_state_warmboot_rx_continue               ( ezbus_mac_t* mac );
static void do_state_warmboot_rx_stop                   ( ezbus_mac_t* mac );


extern void ezbus_mac_warmboot_init( ezbus_mac_t* mac )
{
    ezbus_mac_warmboot_t* boot = ezbus_mac_get_warmboot( mac );

    ezbus_platform_memset( boot, 0 , sizeof( ezbus_mac_warmboot_t) );

    ezbus_timer_init( &boot->warmboot_reply_timer );
    ezbus_timer_init( &boot->warmboot_send_timer );

    ezbus_timer_set_callback( &boot->warmboot_reply_timer, ezbus_mac_warmboot_timer_callback_reply, mac );
    ezbus_timer_set_callback( &boot->warmboot_send_timer,  ezbus_mac_warmboot_timer_callback_send, mac );
}


extern void ezbus_mac_warmboot_run( ezbus_mac_t* mac )
{
    // ezbus_mac_warmboot_t* boot = ezbus_mac_get_warmboot( mac );

    static ezbus_mac_warmboot_state_t boot_state=(ezbus_mac_warmboot_state_t)0xff;

    if ( ezbus_mac_warmboot_get_state( mac ) != boot_state )
    {
        ezbus_log( EZBUS_LOG_BOOTSTATE, "%s\n", ezbus_mac_warmboot_get_state_str(mac) );
        boot_state = ezbus_mac_warmboot_get_state( mac );
    }

    switch ( ezbus_mac_warmboot_get_state( mac ) )
    {
        case state_warmboot_silent_start:    do_state_warmboot_silent_start    ( mac );  break;
        case state_warmboot_silent_continue: do_state_warmboot_silent_continue ( mac );  break;
        case state_warmboot_silent_stop:     do_state_warmboot_silent_stop     ( mac );  break;

        case state_warmboot_tx_first:        do_state_warmboot_tx_first        ( mac );  break;
        case state_warmboot_tx_start:        do_state_warmboot_tx_start        ( mac );  break;
        case state_warmboot_tx_restart:      do_state_warmboot_tx_restart      ( mac );  break;
        case state_warmboot_tx_continue:     do_state_warmboot_tx_continue     ( mac );  break;
        case state_warmboot_tx_stop:         do_state_warmboot_tx_stop         ( mac );  break;

        case state_warmboot_rx_start:        do_state_warmboot_rx_start        ( mac );  break;
        case state_warmboot_rx_continue:     do_state_warmboot_rx_continue     ( mac );  break;
        case state_warmboot_rx_stop:         do_state_warmboot_rx_stop         ( mac );  break;
    }
}

extern const char* ezbus_mac_warmboot_get_state_str( ezbus_mac_t* mac )
{
    ezbus_mac_warmboot_t* boot = ezbus_mac_get_warmboot( mac );

    switch(boot->state)
    {
        case state_warmboot_silent_start:      return "state_warmboot_silent_start";    break;
        case state_warmboot_silent_continue:   return "state_warmboot_silent_continue"; break;
        case state_warmboot_silent_stop:       return "state_warmboot_silent_stop";     break;
        
        case state_warmboot_tx_first:          return "state_warmboot_tx_first";      break;
        case state_warmboot_tx_start:          return "state_warmboot_tx_start";      break;
        case state_warmboot_tx_restart:        return "state_warmboot_tx_restart";    break;
        case state_warmboot_tx_continue:       return "state_warmboot_tx_continue";   break;
        case state_warmboot_tx_stop:           return "state_warmboot_tx_stop";       break;

        case state_warmboot_rx_start:          return "state_warmboot_rx_start";      break;
        case state_warmboot_rx_continue:       return "state_warmboot_rx_continue";   break;
        case state_warmboot_rx_stop:           return "state_warmboot_rx_stop";       break;

    }
    return "";
}


void ezbus_mac_warmboot_set_state( ezbus_mac_t* mac, ezbus_mac_warmboot_state_t state )
{
    ezbus_mac_warmboot_t* boot = ezbus_mac_get_warmboot( mac );
    boot->state = state;
}


ezbus_mac_warmboot_state_t ezbus_mac_warmboot_get_state( ezbus_mac_t* mac )
{
    ezbus_mac_warmboot_t* boot = ezbus_mac_get_warmboot( mac );
    return boot->state;
}


/**** WARMBOOT SILENT BEGIN ****/

static void do_state_warmboot_silent_start         ( ezbus_mac_t* mac )
{
    /* FIXME insert code here */
}

static void do_state_warmboot_silent_continue      ( ezbus_mac_t* mac )
{
    /* FIXME insert code here */
}

static void do_state_warmboot_silent_stop          ( ezbus_mac_t* mac )
{
    /* FIXME insert code here */
}

/**** WARMBOOT SILENT END ****/



/**** WARMBOOT_TX BEGIN ****/


static void do_state_warmboot_tx_first( ezbus_mac_t* mac )
{
    ezbus_mac_warmboot_signal_tx_first(mac);
    ezbus_mac_warmboot_set_state( mac, state_warmboot_tx_start );
}

static void do_state_warmboot_tx_start( ezbus_mac_t* mac )
{
    ezbus_mac_warmboot_t* boot = ezbus_mac_get_warmboot( mac );
    boot->warmboot_count = 0;
    ezbus_mac_warmboot_init_peers( mac );
    ezbus_mac_warmboot_signal_tx_start( mac );
    ezbus_mac_warmboot_set_state( mac, state_warmboot_tx_restart );
}

static void do_state_warmboot_tx_restart( ezbus_mac_t* mac )
{
    ezbus_mac_warmboot_t* boot = ezbus_mac_get_warmboot( mac );
    ezbus_mac_peers_crc( mac, ezbus_mac_warmboot_get_crc(boot) );
    ezbus_mac_warmboot_set_emit_count( boot, 0 );
 
    ezbus_timer_stop( &boot->warmboot_send_timer );
    ezbus_timer_set_period  ( 
                                &boot->warmboot_send_timer, 
                                EZBUS_WARMBOOT_TIMER_MAX
                            );
 
    ezbus_timer_stop( &boot->warmboot_reply_timer );
    ezbus_timer_set_period  ( 
                                &boot->warmboot_reply_timer,  
                                ezbus_platform_random( EZBUS_WARMBOOT_TIMER_MIN, EZBUS_WARMBOOT_TIMER_MAX )
                            );
 
     ezbus_mac_warmboot_signal_tx_restart(mac);

    ezbus_timer_start( &boot->warmboot_send_timer );
    ezbus_timer_start( &boot->warmboot_reply_timer );

    ezbus_mac_warmboot_set_state( mac, state_warmboot_tx_continue );
}

static void do_state_warmboot_tx_continue( ezbus_mac_t* mac )
{
     ezbus_mac_warmboot_signal_tx_continue(mac);
}

static void do_state_warmboot_tx_stop( ezbus_mac_t* mac )
{
    ezbus_mac_warmboot_t* boot = ezbus_mac_get_warmboot( mac );
    ezbus_crc_t peers_crc;
    ezbus_timer_stop( &boot->warmboot_send_timer );
    ezbus_timer_stop( &boot->warmboot_reply_timer );

    /* has the peer list changed? */
    ezbus_mac_peers_crc( mac, &peers_crc );
    if ( ezbus_crc_equal( &peers_crc, ezbus_mac_warmboot_get_crc(boot) ) )
    {
        if ( ++boot->warmboot_count >= EZBUS_EMIT_CYCLES  )
        {
            ezbus_mac_warmboot_signal_tx_stop(mac);
            ezbus_mac_warmboot_set_state( mac, state_warmboot_silent_start );            
        }
        else
        {
            ezbus_mac_warmboot_set_state( mac, state_warmboot_tx_restart ); 
        }
    }
    else
    {
        boot->warmboot_count = 0;
        ezbus_mac_warmboot_set_state( mac, state_warmboot_tx_restart ); 
    }
}

static void ezbus_mac_warmboot_timer_callback_send( ezbus_timer_t* timer, void* arg )
{
    ezbus_mac_t* mac = (ezbus_mac_t*)arg;
    ezbus_mac_warmboot_set_state( mac, state_warmboot_tx_stop );
}

/**** WARMBOOT_TX END ****/



/**** WARMBOOT_RX  BEGIN ****/

static void do_state_warmboot_rx_start( ezbus_mac_t* mac )
{
    ezbus_mac_warmboot_t* boot = ezbus_mac_get_warmboot(mac);

    ezbus_mac_warmboot_set_emit_count( boot, 0 );
    ezbus_timer_stop( &boot->warmboot_reply_timer );

    ezbus_timer_set_period  ( 
                                &boot->warmboot_reply_timer,  
                                ezbus_platform_random( EZBUS_WARMBOOT_TIMER_MIN, EZBUS_WARMBOOT_TIMER_MAX )
                            );
    ezbus_mac_warmboot_signal_rx_start(mac);
    ezbus_timer_start( &boot->warmboot_reply_timer );
    ezbus_mac_warmboot_set_state( mac, state_warmboot_rx_continue );
}

static void do_state_warmboot_rx_continue( ezbus_mac_t* mac )
{
    ezbus_mac_warmboot_signal_rx_continue(mac);
}

static void do_state_warmboot_rx_stop( ezbus_mac_t* mac )
{
    ezbus_mac_warmboot_t* boot = ezbus_mac_get_warmboot(mac);
    
    ezbus_timer_stop( &boot->warmboot_reply_timer );
    ezbus_mac_warmboot_signal_rx_stop(mac);
    ezbus_mac_warmboot_set_state( mac, state_warmboot_silent_start );
}

static void ezbus_mac_warmboot_timer_callback_reply( ezbus_timer_t* timer, void* arg )
{
    ezbus_mac_t* mac = (ezbus_mac_t*)arg;
    switch( ezbus_mac_warmboot_get_state( mac ) )
    {
        case state_warmboot_tx_stop:  

            ezbus_mac_warmboot_set_state( mac, state_warmboot_tx_stop );
            break;

        case state_warmboot_rx_continue:    

            ezbus_mac_warmboot_set_state( mac, state_warmboot_rx_stop );
            break;

        default:
            break;
    }
 }

/* WARMBOOT_RX END */



/**
 * @brief The source address is the warmboot designate. Meaning we should reply to warmboot adderss.
 */
static void ezbus_mac_warmboot_signal_peer_seen_src( ezbus_mac_t* mac, ezbus_packet_t* packet )
{
    ezbus_peer_t peer;
    ezbus_mac_warmboot_t* boot = ezbus_mac_get_warmboot(mac);

    ezbus_peer_init( &peer, ezbus_packet_dst( packet ), ezbus_packet_seq( packet ) );

    ezbus_mac_peers_clean( mac, ezbus_packet_seq( packet ) );
    ezbus_mac_peers_insort( mac, &peer );

    if ( ezbus_address_compare( ezbus_packet_dst( packet ), &ezbus_broadcast_address ) == 0 )
    {
        ezbus_mac_warmboot_set_state( mac, state_warmboot_rx_start );
    }
    else
    if ( ezbus_address_compare( ezbus_packet_dst( packet ), &ezbus_self_address ) == 0 )
    {
        if ( ezbus_packet_seq( packet ) != boot->seq )
        {
            ezbus_mac_warmboot_set_state( mac, state_warmboot_silent_start );
            boot->seq = ezbus_packet_seq( packet );
        }
    }
}

/**
 * @brief The destination address is the warmboot designate. Meaning we should sniff the source address as a peer..
 */
static void ezbus_mac_warmboot_signal_peer_seen_dst( ezbus_mac_t* mac, ezbus_packet_t* packet )
{
    ezbus_peer_t peer;

    ezbus_peer_init( &peer, ezbus_packet_src( packet ), ezbus_packet_seq( packet ) );

    ezbus_mac_peers_clean( mac, ezbus_packet_seq( packet ) );
    ezbus_mac_peers_insort( mac, &peer );
}


extern void ezbus_mac_warmboot_signal_peer_seen( ezbus_mac_t* mac, ezbus_packet_t* packet )
{
    if ( ezbus_address_compare( &ezbus_warmboot_address, ezbus_packet_src( packet ) ) == 0 )
    {
        ezbus_mac_warmboot_signal_peer_seen_src( mac, packet );
    }
    else
    if ( ezbus_address_compare( &ezbus_warmboot_address, ezbus_packet_dst( packet ) ) == 0 )
    {
        ezbus_mac_warmboot_signal_peer_seen_dst( mac, packet );
    }
}

extern void ezbus_mac_warmboot_signal_token_seen( ezbus_mac_t* mac, ezbus_packet_t* packet )
{
    ezbus_peer_t peer;

    ezbus_peer_init( &peer, ezbus_packet_src( packet ), ezbus_packet_seq( packet ) );
    ezbus_mac_peers_clean( mac, ezbus_packet_seq( packet ) );
    ezbus_mac_peers_insort( mac, &peer );

    ezbus_mac_warmboot_set_state( mac, state_warmboot_silent_start );
}


static void ezbus_mac_warmboot_init_peers( ezbus_mac_t* mac )
{
    ezbus_peer_t self_peer;
    ezbus_mac_warmboot_t* boot = ezbus_mac_get_warmboot(mac);

    ezbus_mac_peers_clear( mac );
    ezbus_peer_init( &self_peer, &ezbus_self_address, boot->seq );
    ezbus_mac_peers_insort( mac, &self_peer );    
}




extern void ezbus_mac_arbitration_receive_signal_warmboot( ezbus_mac_t* mac, ezbus_packet_t* rx_packet )
{
    ezbus_log( EZBUS_LOG_WARMBOOT, "%cwarmboot <%s %3d | ", ezbus_mac_get_token(mac)?'*':' ', ezbus_address_string( ezbus_packet_src( rx_packet ) ), ezbus_packet_seq( rx_packet ) );
    #if EZBUS_LOG_WARMBOOT
        ezbus_mac_peers_log( mac );
    #endif
}