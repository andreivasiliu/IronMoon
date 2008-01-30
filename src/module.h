/*
 * Copyright (c) 2004, 2005  Andrei Vasiliu
 *
 *
 * This file is part of MudBot.
 *
 * MudBot is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MudBot is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MudBot; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#define MODULE_ID "$Name: HEAD $ $Id: module.h,v 1.1 2008/01/23 12:10:22 andreivasiliu Exp $"

/* Module header file, to be used with all module source files. */


#include "header.h"

#define ENTRANCE( name ) void (name)( MODULE *self )


/*** Global functions. ***/

/* Line operators. */
void prefix_at( int line, char *string );
void suffix_at( int line, char *string );
void replace_at( int line, char *string );
void insert_at( int line, int pos, char *string );
void hide_line_at( int line );
void prefix( char *string );
void suffix( char *string );
void replace( char *string );
void insert( int pos, char *string );
void hide_line( );
void set_line( int line );
void prefixf( char *string, ... ) __attribute__ ( ( format( printf, 1, 2 ) ) );
void suffixf( char *string, ... ) __attribute__ ( ( format( printf, 1, 2 ) ) );

/* Communication */
MODULE *get_modules( );
void DEBUG( char * );
void debugf( char *string, ... ) __attribute__ ( ( format( printf, 1, 2 ) ) );
void debugerr( char *string );
void logff( char *type, char *string, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
void clientf( char *string );
void clientfr( char *string );
void clientff( char *string, ... ) __attribute__ ( ( format( printf, 1, 2 ) ) );
void send_to_server( char *string );
void show_prompt( );
void mxp( char *string, ... ) __attribute__ ( ( format( printf, 1, 2 ) ) );
int mxp_tag( int tag );
int mxp_stag( int tag, char *dest );
char *get_poscolor( int pos );
char *get_poscolor_at( int line, int pos );
void *get_variable( char *name );

/* Utility */
#if defined( FOR_WINDOWS )
int gettimeofday( struct timeval *tv, void * );
#endif
char *get_string( const char *argument, char *arg_first, int max );
int cmp( char *trigger, char *string );
void extract_wildcard( int nr, char *dest, int max );

/* Timers */
TIMER *get_timers( );
int get_timer( );
TIMER *add_timer( const char *name, float delay, void (*cb)( TIMER *self ), int d0, int d1, int d2 );
void del_timer( const char *name );

/* Networking */
DESCRIPTOR *get_descriptors( );
int mb_connect( const char *hostname, int port );
char *get_connect_error( );
void add_descriptor( DESCRIPTOR *desc );
void remove_descriptor( DESCRIPTOR *desc );
int c_read( int fd, void *buf, size_t count );
int c_write( int fd, const void *buf, size_t count );
int c_close( int fd );

/* Config */
CONFIG_ELEMENT *config_getlist( char *varpath );
const char *config_getvalue( char *varpath );
void config_delitem( char *varpath );
void config_setdescription( char *varpath, char *description );
void config_setvalue( char *varpath, char *value );
void config_addvalue( char *varpath, char *value );
void config_setlist( char *varpath );
int read_config( const char *section, const char *file );
void save_config( const char *section, const char *file );

