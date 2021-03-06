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
#include <ezbus_mac_timer.h>
#include <ezbus_log.h>
#include <ezbus_platform.h>

static bool ezbus_timer_timeout   ( ezbus_timer_t* timer );
static void ezbus_timer_do_pausing( ezbus_timer_t* timer );
static void ezbus_timer_do_paused ( ezbus_timer_t* timer );
static void ezbus_timer_do_resume ( ezbus_timer_t* timer );

static bool ezbus_timer_append    ( ezbus_mac_t* mac, ezbus_timer_t* timer );
static bool ezbus_timer_remove    ( ezbus_mac_t* mac, ezbus_timer_t* timer );
static int  ezbus_timer_indexof   ( ezbus_mac_t* mac, ezbus_timer_t* timer );

extern void ezbus_mac_timer_init( ezbus_mac_t* mac )
{
    ezbus_mac_timer_t* mac_timer = ezbus_mac_get_timer( mac );
    ezbus_platform.callback_memset( mac_timer, 0, sizeof(ezbus_mac_timer_t) );
}

extern void ezbus_mac_timer_run( ezbus_mac_t* mac )
{
    ezbus_mac_timer_t* mac_timer = ezbus_mac_get_timer(mac);
    
    for( int index=0; index < mac_timer->ezbus_timers_count; index++ )
    {
        ezbus_timer_t* timer = mac_timer->ezbus_timers[index];
        switch( timer->state )
        {
            case state_timer_stopping:
                // EZBUS_LOG( EZBUS_LOG_TIMERS, "state_timer_stopping  [%08X,%08X] - %s", timer->callback, timer->arg, ezbus_timer_get_key( timer ) );
                ezbus_timer_set_state( timer, state_timer_stopped );
                break;
            case state_timer_stopped:
                break;
            case state_timer_starting:
                // EZBUS_LOG( EZBUS_LOG_TIMERS, "state_timer_starting [%08X,%08X] - %s", timer->callback, timer->arg, ezbus_timer_get_key( timer ) );
                timer->start = ezbus_timer_get_ticks( timer );
                ezbus_timer_set_state( timer, state_timer_started );
                break;
            case state_timer_started:
                if ( ezbus_timer_timeout( timer ) )
                {
                    EZBUS_LOG( EZBUS_LOG_TIMERS, "state_timer_started  [%08X,%08X] - %s", timer->callback, timer->arg, ezbus_timer_get_key( timer ) );
                    ezbus_timer_set_state( timer, state_timer_expiring );
                }
                break;
            case state_timer_pausing:
                EZBUS_LOG( EZBUS_LOG_TIMERS, "state_timer_pausing  [%08X,%08X] - %s", timer->callback, timer->arg, ezbus_timer_get_key( timer ) );
                ezbus_timer_do_pausing( timer );
                ezbus_timer_set_state( timer, state_timer_paused );
                break;
            case state_timer_paused:
                ezbus_timer_do_paused( timer );
                break;
            case state_timer_resume:
                EZBUS_LOG( EZBUS_LOG_TIMERS, "state_timer_resume   [%08X,%08X] - %s", timer->callback, timer->arg, ezbus_timer_get_key( timer ) );
                ezbus_timer_do_resume( timer );
                //ezbus_timer_set_state( timer, state_timer_started );
                break;
            case state_timer_expiring:
                EZBUS_LOG( EZBUS_LOG_TIMERS, "state_timer_expiring [%08X,%08X] - %s", timer->callback, timer->arg, ezbus_timer_get_key( timer ) );
                ezbus_timer_set_state( timer, state_timer_expired );
                break;
            case state_timer_expired:
                EZBUS_LOG( EZBUS_LOG_TIMERS, "state_timer_expired  [%08X,%08X] - %s", timer->callback, timer->arg, ezbus_timer_get_key( timer ) );
                if ( timer->callback )
                    timer->callback( timer, timer->arg );
                //ezbus_timer_set_state( timer, state_timer_stopping );
                break;
        }
    }
}

extern void ezbus_mac_timer_setup( ezbus_mac_t* mac, ezbus_timer_t* timer, bool pausable )
{
    ezbus_platform.callback_memset( timer, 0, sizeof(ezbus_timer_t) );
    ezbus_timer_append( mac, timer );
    ezbus_timer_set_pausable( timer, pausable );
}

static bool ezbus_timer_append( ezbus_mac_t* mac, ezbus_timer_t* timer )
{
    ezbus_mac_timer_t* mac_timer = ezbus_mac_get_timer(mac);

    if ( ezbus_timer_indexof( mac, timer ) < 0 )
    {
        mac_timer->ezbus_timers = 
            (ezbus_timer_t**)ezbus_platform.callback_realloc( 
                mac_timer->ezbus_timers, 
                ( sizeof(ezbus_timer_t*) * (++mac_timer->ezbus_timers_count) )
            );

        if ( mac_timer->ezbus_timers != NULL )
        {
            int index = mac_timer->ezbus_timers_count-1;
            mac_timer->ezbus_timers[index] = timer;  
            return true;
        }

        return false;
    }
    return true;
}

static bool ezbus_timer_remove( ezbus_mac_t* mac, ezbus_timer_t* timer )
{
    ezbus_mac_timer_t* mac_timer = ezbus_mac_get_timer(mac);
    uint32_t index = ezbus_timer_indexof( mac, timer );

    if ( index >= 0 )
    {
        ezbus_platform.callback_memmove( &mac_timer->ezbus_timers[index], &mac_timer->ezbus_timers[index+1], (--mac_timer->ezbus_timers_count)-index );
        mac_timer->ezbus_timers = (ezbus_timer_t**)ezbus_platform.callback_realloc( mac_timer->ezbus_timers, ( sizeof(ezbus_timer_t*) * mac_timer->ezbus_timers_count ) );
        return true;
    }
    return false;
}

static int ezbus_timer_indexof( ezbus_mac_t* mac, ezbus_timer_t* timer )
{
    ezbus_mac_timer_t* mac_timer = ezbus_mac_get_timer(mac);

    if ( mac_timer->ezbus_timers != NULL && mac_timer->ezbus_timers_count > 0 )
    {
        for( uint32_t index=0; index < mac_timer->ezbus_timers_count; index++ )
        {
            if ( mac_timer->ezbus_timers[index] == timer )
            {
                return index;
            }
        }
    }
    return -1; 
}

extern void ezbus_timer_set_state( ezbus_timer_t* timer, ezbus_timer_state_t state )
{
    timer->state = state;
}

extern ezbus_timer_state_t ezbus_timer_get_state( ezbus_timer_t* timer )
{
    return timer->state;
}

extern void ezbus_timer_set_pause_state( ezbus_timer_t* timer, ezbus_timer_state_t pause_state )
{
    timer->pause_state = pause_state;
}

extern ezbus_timer_state_t ezbus_timer_get_pause_state( ezbus_timer_t* timer )
{
    return timer->pause_state;
}


extern void ezbus_timer_set_key( ezbus_timer_t* timer, char* key )
{
    timer->key = key;
}

extern char* ezbus_timer_get_key( ezbus_timer_t* timer )
{
    return timer->key;
}


extern void ezbus_timer_set_period( ezbus_timer_t* timer, ezbus_ms_tick_t period )
{
    timer->period = period;
}

extern ezbus_ms_tick_t ezbus_timer_get_period( ezbus_timer_t* timer )
{
    return timer->period;
}

extern void ezbus_timer_set_callback( ezbus_timer_t* timer, ezbus_timer_callback_t callback, void* arg )
{
    timer->callback = callback;
    timer->arg = arg;
}

extern void ezbus_timer_set_pausable( ezbus_timer_t* timer, bool pausable )
{
    timer->pausable = pausable;
}

extern bool ezbus_timer_get_pausable( ezbus_timer_t* timer )
{
    return timer->pausable;
}

extern void ezbus_timer_set_pause_duration( ezbus_timer_t* timer, ezbus_ms_tick_t pause_duration )
{
    timer->pause_duration = pause_duration;
}

extern ezbus_ms_tick_t ezbus_timer_get_pause_duration( ezbus_timer_t* timer )
{
    return timer->pause_duration;
}

extern void ezbus_timer_set_pause_start( ezbus_timer_t* timer, ezbus_ms_tick_t pause_start )
{
    timer->pause_start = pause_start;
}

extern ezbus_ms_tick_t ezbus_timer_get_pause_start( ezbus_timer_t* timer )
{
    return timer->pause_start;
}

extern void ezbus_timers_set_pause_duration( ezbus_mac_t* mac, ezbus_ms_tick_t pause_duration )
{
    ezbus_mac_timer_t* mac_timer = ezbus_mac_get_timer(mac);

    for( int index = 0; index < mac_timer->ezbus_timers_count; index++ )
    {
        ezbus_timer_set_pause_duration( mac_timer->ezbus_timers[ index ], pause_duration );
    }
}

extern void ezbus_timers_set_pause_active( ezbus_mac_t* mac, bool active )
{
    ezbus_mac_timer_t* mac_timer = ezbus_mac_get_timer(mac);

    if ( mac_timer->ezbus_timers_pause_active != active )
    {
        for( int index = 0; index < mac_timer->ezbus_timers_count; index++ )
        {
            if ( active )
                ezbus_timer_pause( mac_timer->ezbus_timers[ index ] );
            else
                ezbus_timer_resume( mac_timer->ezbus_timers[ index ] );    
        }
        mac_timer->ezbus_timers_pause_active = active;
    }
}

extern bool ezbus_timers_get_pause_active( ezbus_mac_t* mac )
{
    ezbus_mac_timer_t* mac_timer = ezbus_mac_get_timer(mac);
    return mac_timer->ezbus_timers_pause_active;
}

extern ezbus_ms_tick_t ezbus_timer_get_ticks( ezbus_timer_t* timer )
{
    return ezbus_platform.callback_get_ms_ticks();
}

extern void ezbus_timer_pause( ezbus_timer_t* timer )
{
    if ( ezbus_timer_get_pausable( timer ) )
    {
        ezbus_timer_set_pause_state( timer, ezbus_timer_get_state( timer ) );
        ezbus_timer_set_state((timer),state_timer_pausing);
    }
}

extern void ezbus_timer_resume( ezbus_timer_t* timer )
{
    if ( ezbus_timer_get_pausable( timer ) )
    {
        ezbus_timer_set_state((timer),state_timer_resume);
    }
}

static bool ezbus_timer_timeout( ezbus_timer_t* timer ) 
{
    return (ezbus_timer_get_ticks(timer) - (timer)->start) > timer->period;
}

static void ezbus_timer_do_pausing( ezbus_timer_t* timer )
{
    /* preserve the timer state */
    timer->pause_start = ezbus_timer_get_ticks( timer );
}

static void ezbus_timer_do_paused ( ezbus_timer_t* timer )
{
    if ( ezbus_timer_get_pause_duration( timer ) )
    {
        if ( ( ezbus_timer_get_ticks( timer ) - timer->pause_start ) > ezbus_timer_get_pause_duration( timer ) )
        {
            ezbus_timer_resume( timer );
        }
    }
}

static void ezbus_timer_do_resume( ezbus_timer_t* timer )
{
    ezbus_ms_tick_t pause_delta = (timer->pause_start - timer->start);
    timer->start += pause_delta;
    ezbus_timer_set_state( timer, ezbus_timer_get_pause_state( timer ) );
}

