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
#include <ezbus_platform.h>
#include <ezbus_address.h>
#include <linux/serial.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h> 
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include <stdbool.h>
#include <time.h>
#include <sys/select.h>
#include <sys/time.h>
#include <asm/termbits.h>

#include <ezbus_hex.h>

#define EZBUS_PACKET_DEBUG	1

static void serial_set_blocking (int fd, int should_block);

int ezbus_platform_open(ezbus_platform_port_t* port,uint32_t speed)
{
	if ( (port->fd = open(port->serial_port_name,O_RDWR)) >=0 )
	{
		struct serial_rs485 rs485conf = {0};

		rs485conf.flags |= SER_RS485_ENABLED;
		rs485conf.flags |= SER_RS485_RTS_ON_SEND;
		rs485conf.flags &= ~(SER_RS485_RTS_AFTER_SEND);

		ioctl (port->fd, TIOCSRS485, &rs485conf);
		ezbus_platform_set_speed(port,speed);
		serial_set_blocking(port->fd,false);
		return 0;
	}
	perror("ezbus_platform_open");
	return -1;
}

int ezbus_platform_send(ezbus_platform_port_t* port,void* bytes,size_t size)
{

	uint8_t* p = (uint8_t*)bytes;
	#if EZBUS_PACKET_DEBUG
		ezbus_hex_dump( "TX:", p, size );
	#endif
	ssize_t sent=0;
	do {
		sent += write(port->fd,p,size-sent);
		if ( sent > 0)
			p += sent;
	} while (sent<size&&sent>=0);
	return sent;
}

int ezbus_platform_recv(ezbus_platform_port_t* port,void* bytes,size_t size)
{
	int rc = read(port->fd,bytes,size);
	#if EZBUS_PACKET_DEBUG
		ezbus_hex_dump( "RX:", bytes, rc );
	#endif
	return rc;
}

int ezbus_platform_getc(ezbus_platform_port_t* port)
{
	uint8_t ch;
	int rc = read(port->fd,&ch,1);
	if ( rc == 1 )
	{
		return (int)ch;
	}
	return -1;
}

void ezbus_platform_close(ezbus_platform_port_t* port)
{
	close(port->fd);
}

void ezbus_platform_drain(ezbus_platform_port_t* port)
{
	while(ezbus_platform_getc(port)>=0);
}

int ezbus_platform_set_speed(ezbus_platform_port_t* port,uint32_t baud)
{
	struct termios2 options;

	if ( port->fd >= 0 )
	{
		fcntl(port->fd, F_SETFL, 0);

		ioctl(port->fd, TCGETS2, &options);

		options.c_cflag &= ~CBAUD;
		options.c_cflag |= BOTHER;
		options.c_ispeed = baud;
		options.c_ospeed = baud;
		
		options.c_cflag |= (CLOCAL | CREAD);
		options.c_cflag &= ~CRTSCTS;
		options.c_cflag &= ~HUPCL;

		options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
		options.c_oflag &= ~OPOST;
		options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
		options.c_cflag &= ~(CSIZE | PARENB);
		options.c_cflag |= CS8;

		ioctl(port->fd, TCSETS2, &options);

		return 0;
	}
	return -1;
}

void ezbus_platform_flush(ezbus_platform_port_t* port)
{
	fsync(port->fd);
}

void* ezbus_platform_memset(void* dest, int c, size_t n)
{
	return memset(dest,c,n);
}

void* ezbus_platform_memcpy(void* dest, const void *src, size_t n)
{
	return memcpy(dest,src,n);
}

void* ezbus_platform_memmove(void* dest, const void *src, size_t n)
{
	return memmove(dest,src,n);
}

int ezbus_platform_memcmp(const void* dest, const void *src, size_t n)
{
	return memcmp(dest,src,n);
}

void* ezbus_platform_malloc( size_t n)
{
	return malloc(n);
}

void* ezbus_platform_realloc(void* src,size_t n)
{
	return realloc(src,n);
}

void ezbus_platform_free(void *src)
{
	free(src);
}

ezbus_ms_tick_t ezbus_platform_get_ms_ticks()
{
	ezbus_ms_tick_t ticks;
	struct timeval tm;
	gettimeofday(&tm,NULL);
	ticks = ((tm.tv_sec*1000)+(tm.tv_usec/1000));
	return ticks;
}

void ezbus_platform_address(ezbus_address_t* address)
{
	static uint32_t b[3] = {0,0,0};
	if ( b[0]==0 && b[1]==0 && b[2]==0 )
	{
		ezbus_platform_srand(time(NULL));
		b[0] = ezbus_platform_rand();
		b[1] = ezbus_platform_rand();
		b[2] = ezbus_platform_rand();
	}
	address->word[0] = b[0];
	address->word[1] = b[1];
	address->word[2] = b[2];
}

static void serial_set_blocking (int fd, int should_block)
{
	struct termios2 options;

	if ( fd >= 0 )
	{
		fcntl(fd, F_SETFL, 0);

		ioctl(fd, TCGETS2, &options);

	   	options.c_cc[VMIN]  = should_block ? 1 : 0;
	    options.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

		ioctl(fd, TCSETS2, &options);
	}
}

extern int ezbus_platform_rand(void)
{
	return rand();
}

extern void ezbus_platform_srand(unsigned int seed)
{
	srand(seed);
}

extern int ezbus_platform_random(int lower, int upper)
{
	int num = (rand() % (upper - lower + 1)) + lower;
	return num;
}

extern void ezbus_platform_rand_init (void)
{
	ezbus_platform_srand( ezbus_platform_get_ms_ticks()&0xFFFFFFFF );
}

extern void ezbus_platform_delay(unsigned int ms)
{
	ezbus_ms_tick_t start = ezbus_platform_get_ms_ticks();
	while ( (ezbus_platform_get_ms_ticks() - start) < ms );
}

void ezbus_platform_port_dump( ezbus_platform_port_t* platform_port, const char* prefix )
{
	fprintf(stderr, "%s.serial_port_name=%s\n", prefix, platform_port->serial_port_name );
	fprintf(stderr, "%s.fd=%d\n",               prefix, platform_port->fd );
	fflush(stderr);
}