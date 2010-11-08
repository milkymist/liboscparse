/*
 *  Copyright (C) 2004 Steve Harris
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  $Id$
 */

#ifndef LOP_OSC_TYPES_H
#define LOP_OSC_TYPES_H

/**
 * \file lop_osc_types.h A liblo header defining OSC-related types and
 * constants.
 */

#ifdef _MSC_VER
#define int32_t __int32
#define int64_t __int64
#define uint32_t unsigned __int32
#define uint64_t unsigned __int64
#define uint8_t unsigned __int8
#else
#include <stdint.h>
#endif

/**
 * \addtogroup liblo
 * @{
 */

/**
 * \brief A structure to store OSC TimeTag values.
 */
typedef struct {
	/** The number of seconds since Jan 1st 1900 in the UTC timezone. */
	uint32_t sec;
	/** The fractions of a second offset from above, expressed as 1/2^32nds
         * of a second */
	uint32_t frac;
} lop_timetag;

/**
 * \brief An enumeration of the OSC types liblo can send and receive.
 *
 * The value of the enumeration is the typechar used to tag messages and to
 * specify arguments with lop_send().
 */
typedef enum {
/* basic OSC types */
	/** 32 bit signed integer. */
	LOP_INT32 =     'i',
	/** 32 bit IEEE-754 float. */
	LOP_FLOAT =     'f',
	/** Standard C, NULL terminated string. */
	LOP_STRING =    's',
	/** OSC binary blob type. Accessed using the lop_blob_*() functions. */
	LOP_BLOB =      'b',

/* extended OSC types */
	/** 64 bit signed integer. */
	LOP_INT64 =     'h',
	/** OSC TimeTag type, represented by the lop_timetag structure. */
	LOP_TIMETAG =   't',
	/** 64 bit IEEE-754 double. */
	LOP_DOUBLE =    'd',
	/** Standard C, NULL terminated, string. Used in systems which
	  * distinguish strings and symbols. */
	LOP_SYMBOL =    'S',
	/** Standard C, 8 bit, char variable. */
	LOP_CHAR =      'c',
	/** A 4 byte MIDI packet. */
	LOP_MIDI =      'm',
	/** Sybol representing the value True. */
	LOP_TRUE =      'T',
	/** Sybol representing the value False. */
	LOP_FALSE =     'F',
	/** Sybol representing the value Nil. */
	LOP_NIL =       'N',
	/** Sybol representing the value Infinitum. */
	LOP_INFINITUM = 'I'
} lop_type;


/**
 * \brief Union used to read values from incoming messages.
 *
 * Types can generally be read using argv[n]->t where n is the argument number
 * and t is the type character, with the exception of strings and symbols which
 * must be read with &argv[n]->t.
 */
typedef union {
	/** 32 bit signed integer. */
    int32_t    i;
	/** 32 bit signed integer. */
    int32_t    i32;
	/** 64 bit signed integer. */
    int64_t    h;
	/** 64 bit signed integer. */
    int64_t    i64;
	/** 32 bit IEEE-754 float. */
    float      f;
	/** 32 bit IEEE-754 float. */
    float      f32;
	/** 64 bit IEEE-754 double. */
    double     d;
	/** 64 bit IEEE-754 double. */
    double     f64;
	/** Standard C, NULL terminated string. */
    char       s;
	/** Standard C, NULL terminated, string. Used in systems which
	  * distinguish strings and symbols. */
    char       S;
	/** Standard C, 8 bit, char. */
    unsigned char c;
	/** A 4 byte MIDI packet. */
    uint8_t    m[4];
	/** OSC TimeTag value. */
    lop_timetag t;
} lop_arg;

/** \brief A timetag constant representing "now". */
/* Note: No struct literals in MSVC */
#ifdef _MSC_VER
lop_timetag lop_get_tt_immediate();
#define LOP_TT_IMMEDIATE lop_get_tt_immediate()
#else
#define LOP_TT_IMMEDIATE ((lop_timetag){0U,1U})
#endif

/** @} */

#endif
