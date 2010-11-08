/*
 *  Copyright (C) 2004 Steve Harris
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 2.1 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#endif

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <sys/socket.h>
#include <sys/un.h>
#endif

#include "lop_types_internal.h"
#include "lo/lo.h"

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#ifdef WIN32
int initWSock();
#endif

#ifdef WIN32
#define geterror() WSAGetLastError()
#else
#define geterror() errno
#endif

static int resolve_address(lop_address a);
static int create_socket(lop_address a);
static int send_data(lop_address a, lop_server from, char *data, const size_t data_len);

// message.c
int lop_message_add_varargs_internal(lop_message m, const char *types, va_list ap,
                                    const char *file, int line);



/* Don't call lop_send_internal directly, use lop_send, a macro wrapping this
 * function with appropriate values for file and line */

#ifdef __GNUC__
int lop_send_internal(lop_address t, const char *file, const int line,
     const char *path, const char *types, ...)
#else
int lop_send(lop_address t, const char *path, const char *types, ...)
#endif
{
    va_list ap;
    int ret;
#ifndef __GNUC__
    const char *file = "";
    int line = 0;
#endif

    lop_message msg = lop_message_new();

    t->errnum = 0;
    t->errstr = NULL;

    va_start(ap, types);
    ret = lop_message_add_varargs_internal(msg, types, ap, file, line);

    if (ret) {
	lop_message_free(msg);
	t->errnum = ret;
	if (ret == -1) t->errstr = "unknown type";
	else t->errstr = "bad format/args";
	return ret;
    }

    ret = lop_send_message(t, path, msg);
    lop_message_free(msg);

    return ret;
}


/* Don't call lop_send_timestamped_internal directly, use lop_send_timestamped, a
 * macro wrapping this function with appropriate values for file and line */

#ifdef __GNUC__
int lop_send_timestamped_internal(lop_address t, const char *file,
                               	 const int line, lop_timetag ts,
				 const char *path, const char *types, ...)
#else
int lop_send_timestamped(lop_address t, lop_timetag ts,
				 const char *path, const char *types, ...)
#endif
{
    va_list ap;
    int ret;

    lop_message msg = lop_message_new();
    lop_bundle b = lop_bundle_new(ts);

#ifndef __GNUC__
    const char *file = "";
    int line = 0;
#endif

    t->errnum = 0;
    t->errstr = NULL;

    va_start(ap, types);
    ret = lop_message_add_varargs_internal(msg, types, ap, file, line);

    if (t->errnum) {
	lop_message_free(msg);
	return t->errnum;
    }

    lop_bundle_add_message(b, path, msg);
    ret = lop_send_bundle(t, b);
    lop_message_free(msg);
    lop_bundle_free(b);

    return ret;
}

/* Don't call lop_send_from_internal directly, use macros wrapping this 
 * function with appropriate values for file and line */

#ifdef __GNUC__
int lop_send_from_internal(lop_address to, lop_server from, const char *file,
                               	 const int line, lop_timetag ts,
				 const char *path, const char *types, ...)
#else
int lop_send_from(lop_address to, lop_server from, lop_timetag ts,
				 const char *path, const char *types, ...)
#endif
{
    lop_bundle b = NULL;
    va_list ap;
    int ret;
    
#ifndef __GNUC__
    const char *file = "";
    int line = 0;
#endif

    lop_message msg = lop_message_new();
    if (ts.sec!=LOP_TT_IMMEDIATE.sec || ts.frac!=LOP_TT_IMMEDIATE.frac)
        b = lop_bundle_new(ts);

    // Clear any previous errors
    to->errnum = 0;
    to->errstr = NULL;

    va_start(ap, types);
    ret = lop_message_add_varargs_internal(msg, types, ap, file, line);

    if (to->errnum) {
	if (b) lop_bundle_free(b);
	lop_message_free(msg);
	return to->errnum;
    }

    if (b) {
	lop_bundle_add_message(b, path, msg);
	ret = lop_send_bundle_from(to, from, b);
    } else {
	ret = lop_send_message_from(to, from, path, msg);
    }
    
    // Free-up memory
    lop_message_free(msg);
    if (b) lop_bundle_free(b);

    return ret;
}


#if 0

This (incomplete) function converts from printf-style formats to OSC typetags,
but I think its dangerous and mislieading so its not available at the moment.

static char *format_to_types(const char *format);

static char *format_to_types(const char *format)
{
    const char *ptr;
    char *types = malloc(sizeof(format) + 1);
    char *out = types;
    int inspec = 0;
    int width = 0;
    int number = 0;

    if (!format) {
	return NULL;
    }

    for (ptr = format; *ptr; ptr++) {
	if (inspec) {
	    if (*ptr == 'l') {
		width++;
	    } else if (*ptr >= '0' && *ptr <= '9') {
		number *= 10;
		number += *ptr - '0';
	    } else if (*ptr == 'd') {
		if (width < 2 && number < 64) {
		    *out++ = LOP_INT32;
		} else {
		    *out++ = LOP_INT64;
		}
	    } else if (*ptr == 'f') {
		if (width < 2 && number < 64) {
		    *out++ = LOP_FLOAT;
		} else {
		    *out++ = LOP_DOUBLE;
		}
	    } else if (*ptr == '%') {
		fprintf(stderr, "liblo warning, unexpected '%%' in format\n");
		inspec = 1;
		width = 0;
		number = 0;
	    } else {
		fprintf(stderr, "liblo warning, unrecognised character '%c' "
			"in format\n", *ptr);
	    }
	} else {
	    if (*ptr == '%') {
		inspec = 1;
		width = 0;
		number = 0;
	    } else if (*ptr == LOP_TRUE || *ptr == LOP_FALSE || *ptr == LOP_NIL ||
		       *ptr == LOP_INFINITUM) {
		*out++ = *ptr;
	    } else {
		fprintf(stderr, "liblo warning, unrecognised character '%c' "
			"in format\n", *ptr);
	    }
	}
    }
    *out++ = '\0';

    return types;
}

#endif


static int resolve_address(lop_address a)
{
    int ret;
    
    if (a->protocol == LOP_UDP || a->protocol == LOP_TCP) {
	struct addrinfo *ai;
	struct addrinfo hints;

	memset(&hints, 0, sizeof(hints));
#ifdef ENABLE_IPV6
	hints.ai_family = PF_UNSPEC;
#else
	hints.ai_family = PF_INET;
#endif	
	hints.ai_socktype = a->protocol == LOP_UDP ? SOCK_DGRAM : SOCK_STREAM;
	
	if ((ret = getaddrinfo(a->host, a->port, &hints, &ai))) {
	    a->errnum = ret;
	    a->errstr = gai_strerror(ret);
	    a->ai = NULL;
	    return -1;
	}
	
	a->ai = ai;
    }

    return 0;
}

static int create_socket(lop_address a)
{
    if (a->protocol == LOP_UDP || a->protocol == LOP_TCP) {

	a->socket = socket(a->ai->ai_family, a->ai->ai_socktype, 0);
	if (a->socket == -1) {
	    a->errnum = geterror();
	    a->errstr = NULL;
	    return -1;
	}

	if (a->protocol == LOP_TCP) {
		// Only call connect() for TCP sockets - we use sendto() for UDP
		if ((connect(a->socket, a->ai->ai_addr, a->ai->ai_addrlen))) {
			a->errnum = geterror();
			a->errstr = NULL;
			close(a->socket);
			a->socket = -1;
			return -1;
		}
	}
	// if UDP and destination address is broadcast allow broadcast on the
	// socket
	else if (a->protocol == LOP_UDP && a->ai->ai_family == AF_INET)
    {
        // If UDP, and destination address is broadcast,
        // then allow broadcast on the socket.
        struct sockaddr_in* si = (struct sockaddr_in*)a->ai->ai_addr;
        unsigned char* ip = (unsigned char*)&(si->sin_addr);

        if (ip[0]==255 && ip[1]==255 && ip[2]==255 && ip[3]==255)
        {
            int opt = 1;
            setsockopt(a->socket, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(int));
        }
	}
	
    }
#ifndef WIN32
    else if (a->protocol == LOP_UNIX) {
	struct sockaddr_un sa;

	a->socket = socket(PF_UNIX, SOCK_DGRAM, 0);
	if (a->socket == -1) {
	    a->errnum = geterror();
	    a->errstr = NULL;
	    return -1;
	}

	sa.sun_family = AF_UNIX;
	strncpy(sa.sun_path, a->port, sizeof(sa.sun_path)-1);

	if ((connect(a->socket, (struct sockaddr *)&sa, sizeof(sa))) < 0) {
	    a->errnum = geterror();
	    a->errstr = NULL;
	    close(a->socket);
	    a->socket = -1;
	    return -1;
	}
    } 
#endif
    else {
	/* unknown protocol */
	return -2;
    }

    return 0;
}

static int send_data(lop_address a, lop_server from, char *data, const size_t data_len)
{
    int ret=0;
    int sock=-1;

#ifdef WIN32
    if(!initWSock()) return -1;
#endif

    if (data_len > LOP_MAX_MSG_SIZE) {
	a->errnum = 99;
	a->errstr = "Attempted to send message in excess of maximum "
		    "message size";
	return -1;
    }
    
    // Resolve the destination address, if not done already
    if (!a->ai) {
	ret = resolve_address( a );
	if (ret) return ret;
    }

    // Re-use existing socket?
    if (from) {
	sock = from->sockets[0].fd;
    } else if (a->protocol == LOP_UDP && lop_client_sockets.udp!=-1) {
	sock = lop_client_sockets.udp;
    } else {
	if (a->socket==-1) {
	    ret = create_socket( a );
	    if (ret) return ret;
    	}
	sock = a->socket;
    }



    // Send Length of the following data
    if (a->protocol == LOP_TCP) {
	int32_t size = htonl(data_len); 
	ret = send(sock, &size, sizeof(size), MSG_NOSIGNAL); 
    }
    
    // Send the data
    if (a->protocol == LOP_UDP) {
        if (a->ttl >= 0) {
            unsigned char ttl = (unsigned char)a->ttl;
            setsockopt(sock,IPPROTO_IP,IP_MULTICAST_TTL,&ttl,sizeof(ttl));
        }
        ret = sendto(sock, data, data_len, MSG_NOSIGNAL,
                     a->ai->ai_addr, a->ai->ai_addrlen);
    } else {
	ret = send(sock, data, data_len, MSG_NOSIGNAL);
    }

    if (a->protocol == LOP_TCP && ret == -1) {
        close(a->socket);
        a->socket=-1;
    }

    if (ret == -1) {
	a->errnum = geterror();
	a->errstr = NULL;
    } else {
	a->errnum = 0;
	a->errstr = NULL;
    }

    return ret;
}


int lop_send_message(lop_address a, const char *path, lop_message msg)
{
    return lop_send_message_from( a, NULL, path, msg );
}

int lop_send_message_from(lop_address a, lop_server from, const char *path, lop_message msg)
{
    const size_t data_len = lop_message_length(msg, path);
    char *data = lop_message_serialise(msg, path, NULL, NULL);

    // Send the message
    int ret = send_data( a, from, data, data_len );

    // For TCP, retry once if it failed.  The first try will return
    // error if the connection was closed, so the second try will
    // attempt to re-open the connection.
    if (ret == -1 && a->protocol == LOP_TCP)
        ret = send_data( a, from, data, data_len );

    // Free the memory allocated by lop_message_serialise
    if (data) free( data );

    return ret;
}


int lop_send_bundle(lop_address a, lop_bundle b)
{
    return lop_send_bundle_from( a, NULL, b );
}


int lop_send_bundle_from(lop_address a, lop_server from, lop_bundle b)
{
    const size_t data_len = lop_bundle_length(b);
    char *data = lop_bundle_serialise(b, NULL, NULL);

    // Send the bundle
    int ret = send_data( a, from, data, data_len );

    // Free the memory allocated by lop_bundle_serialise
    if (data) free( data );

    return ret;
}

/* vi:set ts=8 sts=4 sw=4: */
