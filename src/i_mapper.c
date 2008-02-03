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


/* Imperian internal mapper. */

#define I_MAPPER_ID "$Name: HEAD $ $Id: i_mapper.c,v 1.1 2008/01/23 12:10:21 andreivasiliu Exp $"
#define BUILTIN_MODULE 1

#include "module.h"
#include "i_mapper.h"

#include <sys/stat.h>

int mapper_version_major = 1;
int mapper_version_minor = 5;


char *i_mapper_id = I_MAPPER_ID "\r\n" I_MAPPER_H_ID "\r\n" HEADER_ID "\r\n" MODULE_ID "\r\n";

char *room_color = "\33[33m";
int room_color_len = 5;

char map_file[256] = "IMap";
char map_file_bin[256] = "IMap.bin";

/* A few things we'll need. */
char *dir_name[] = 
{ "none", "north", "northeast", "east", "southeast", "south",
     "southwest", "west", "northwest", "up", "down", "in", "out", NULL };

char *dir_small_name[] = 
{ "-", "n", "ne", "e", "se", "s", "sw", "w", "nw", "u", "d", "in", "out", NULL };

int reverse_exit[] =
{ 0, EX_S, EX_SW, EX_W, EX_NW, EX_N, EX_NE, EX_E, EX_SE,
     EX_D, EX_U, EX_OUT, EX_IN, 0 };


ROOM_TYPE *room_types;
ROOM_TYPE *null_room_type;

/*
 ROOM_TYPE room_types[256] = 
{  { "Unknown",                 C_r,    1, 1, 0, 0 },
   { "Undefined",               C_R,    1, 1, 0, 0 },
   { "Road",                    C_D,    1, 1, 0, 0 },
   { "Path",                    C_D,    1, 1, 0, 0 },
   { "Natural underground",     C_y,    1, 1, 0, 0 },
   { "River",                   C_B,    3, 3, 1, 0 },
   { "Ocean",                   C_B,    3, 3, 1, 0 },
   { "Grasslands",              C_G,    1, 1, 0, 0 },
   { "Forest",                  C_g,    1, 1, 0, 0 },
   { "Beach",                   C_Y,    1, 1, 0, 0 },
   { "Garden",                  C_G,    1, 1, 0, 0 },
   { "Urban",                   C_D,    1, 1, 0, 0 },
   { "Hills",                   C_y,    1, 1, 0, 0 },
   { "Mountains",               C_y,    1, 1, 0, 0 },
   { "Desert",                  C_y,    1, 1, 0, 0 },
   { "Jungle",                  C_g,    1, 1, 0, 0 },
   { "Valley",                  C_y,    1, 1, 0, 0 },
   { "Freshwater",              C_B,    3, 3, 1, 0 },
   { "Constructed underground", C_y,    1, 1, 0, 0 },
   { "Swamp",                   C_y,    1, 1, 0, 0 },
   { "Underworld",              C_R,    1, 1, 0, 0 },
   { "Sewer",                   C_D,    1, 1, 0, 0 },
   { "Tundra",                  C_W,    1, 1, 0, 0 },
   { "Sylayan city",            C_D,    1, 1, 0, 0 },
   { "Crags",                   C_D,    1, 1, 0, 0 },
   { "Deep Ocean",              C_B,    1, 1, 0, 1 },
   { "Dark Forest",             C_D,    1, 1, 0, 0 },
   { "Polar",                   C_W,    1, 1, 0, 0 },
   { "Warrens",                 C_y,    1, 1, 0, 0 },
   { "Dwarven city",            C_D,    1, 1, 0, 0 },
   { "Underground Lake",        C_B,    3, 3, 1, 0 },
   { "Tainted Water",           C_R,    5, 5, 1, 0 },
   { "Farmland",                C_Y,    1, 1, 0, 0 },
   { "Village",                 C_c,    1, 1, 0, 0 },
   { "Academia",                C_c,    1, 1, 0, 0 },
   { "the Stream of Consciousness",     C_m,    1, 1, 0, 0 },
   
   { NULL, NULL, 0, 0, 0 }
};*/


COLOR_DATA color_names[] =
{
     { "normal",         C_0,           "\33[0m", 4 },
     { "red",            C_r,           "\33[31m", 5 },
     { "green",          C_g,           "\33[32m", 5 },
     { "brown",          C_y,           "\33[33m", 5 },
     { "blue",           C_b,           "\33[34m", 5 },
     { "magenta",        C_m,           "\33[35m", 5 },
     { "cyan",           C_c,           "\33[36m", 5 },
     { "white",          C_0,           "\33[37m", 5 },
     { "dark",           C_D,           C_D, 7 },
     { "bright-red",     C_R,           C_R, 7 },
     { "bright-green",   C_G,           C_G, 7 },
     { "bright-yellow",  C_Y,           C_Y, 7 },
     { "bright-blue",    C_B,           C_B, 7 },
     { "bright-magenta", C_M,           C_M, 7 },
     { "bright-cyan",    C_C,           C_C, 7 },
     { "bright-white",   C_W,           C_W, 7 },
   
     { NULL, NULL, 0 }
};



/* Misc. */
int parsing_room;

int mode;

int get_unlost_exits;
int get_unlost_detected_exits[13];
int auto_walk;
int pear_defence;
int sense_message;
int scout_list;
int evstatus_list;
int pet_list;
int door_closed;
int door_locked;
int locate_arena;
char cse_command[5120];
char cse_message[5120];
int close_rmtitle_tag;
int similar;
int title_offset;
int floating_map_enabled;
int skip_newline_on_map;
int gag_next_prompt;
int check_for_duplicates;
int godname;
int warn_misaligned;
int warn_environment;
int warn_unlinked;
int allow_title_update;
int capture_special_exit;
int special_exit_vnum;
int special_exit_nocommand;
int special_exit_alias;
char *special_exit_tag;

/* config.mapper.txt options. */
int disable_swimming;
int disable_wholist;
int disable_alertness;
int disable_locating;
int disable_areaname;
int disable_mxp_title;
int disable_mxp_exits;
int disable_mxp_map;
int disable_autolink;
int disable_sewer_grates;
int enable_godrooms;


char *dash_command;

int set_length_to;
int switch_exit_stops_mapping;
int switch_exit_joins_areas;
int update_area_from_survey;
int use_direction_instead;
int unidirectional_exit;

ROOM_DATA *world;
ROOM_DATA *world_last;
ROOM_DATA *current_room;
ROOM_DATA *hash_world[MAX_HASH];

ROOM_DATA *link_next_to;
ROOM_DATA *last_room;

AREA_DATA *areas;
AREA_DATA *areas_last;
AREA_DATA *current_area;

EXIT_DATA *global_special_exits;

ROOM_DATA *map[MAP_X+2][MAP_Y+2];
char extra_dir_map[MAP_X+2][MAP_Y+2];

/* New! */
MAP_ELEMENT map_new[MAP_X][MAP_Y];

/* Newer! */
ELEMENT *exit_tags;

int last_vnum;

/* Path finder chains. */
ROOM_DATA *pf_current_openlist;
ROOM_DATA *pf_new_openlist;

/* New path-finder chains. */
#define MAX_COST 13
int pf_step = 0, pf_rooms;
ROOM_DATA *pf_queue[MAX_COST];


/* Command queue. -1 is look, positive number is direction. */
int queue[256];
int q_top;

int auto_bump;
ROOM_DATA *bump_room;
int bump_exits;


/* Here we register our functions. */

void i_mapper_module_init_data( );
void i_mapper_module_unload( );
void i_mapper_process_server_line( LINE *line );
void i_mapper_process_server_prompt( LINE *line );
void i_mapper_process_server_paragraph( LINES *l );
int  i_mapper_process_client_command( char *cmd );
int  i_mapper_process_client_aliases( char *cmd );
void i_mapper_mxp_enabled( );


ENTRANCE( i_mapper_module_register )
{
   self->name = strdup( "IMapper" );
   self->version_major = mapper_version_major;
   self->version_minor = mapper_version_minor;
   self->id = i_mapper_id;
   
   self->init_data = i_mapper_module_init_data;
   self->unload = i_mapper_module_unload;
   self->process_server_line = i_mapper_process_server_line;
   self->process_server_prompt = i_mapper_process_server_prompt;
   self->process_server_paragraph = i_mapper_process_server_paragraph;
   self->process_client_aliases = i_mapper_process_client_aliases;
   self->mxp_enabled = i_mapper_mxp_enabled;
}



int get_free_vnum( )
{
   last_vnum++;
   return last_vnum;
}



void refind_last_vnum( )
{
   ROOM_DATA *room;
   
   last_vnum = 0;
   
   for ( room = world; room; room = room->next_in_world )
     if ( last_vnum < room->vnum )
       last_vnum = room->vnum;
}



void link_to_area( ROOM_DATA *room, AREA_DATA *area )
{
   DEBUG( "link_to_area" );
   
   /* Link to an area. */
   if ( area )
     {
        if ( area->last_room )
          {
             area->last_room->next_in_area = room;
             room->next_in_area = NULL;
          }
        else
          {
             room->next_in_area = area->rooms;
             area->rooms = room;
          }
        
        area->last_room = room;
        room->area = area;
     }
   else if ( current_area )
     link_to_area( room, current_area );
   else if ( areas )
     {
        debugf( "No current area set, while trying to link a room." );
        link_to_area( room, areas );
     }
   else
     clientfr( "Can't link this room anywhere." );
}



void unlink_from_area( ROOM_DATA *room )
{
   ROOM_DATA *r;
   
   DEBUG( "unlink_from_area" );
   
   /* Unlink from area. */
   if ( room->area->rooms == room )
     {
        room->area->rooms = room->next_in_area;
        
        if ( room->area->last_room == room )
          room->area->last_room = NULL;
     }
   else
     for ( r = room->area->rooms; r; r = r->next_in_area )
       {
          if ( r->next_in_area == room )
            {
               r->next_in_area = room->next_in_area;
               
               if ( room->area->last_room == room )
                 room->area->last_room = r;
               
               break;
            }
       }
}



ROOM_DATA *create_room( int vnum_create )
{
   ROOM_DATA *new_room;
   
   DEBUG( "create_room" );
   
   /* calloc will also clear everything to 0, for us. */
   new_room = calloc( sizeof( ROOM_DATA ), 1 );
   
   /* Init. */
   new_room->name = NULL;
   new_room->special_exits = NULL;
   new_room->room_type = null_room_type;
   if ( vnum_create < 0 )
     new_room->vnum = get_free_vnum( );
   else
     new_room->vnum = vnum_create;
   
   /* Link to main chain. */
   if ( !world )
     {
        world = new_room;
        world_last = new_room;
     }
   else
     world_last->next_in_world = new_room;
   
   new_room->next_in_world = NULL;
   world_last = new_room;
   
   /* Link to hashed table. */
   if ( !hash_world[new_room->vnum % MAX_HASH] )
     {
        hash_world[new_room->vnum % MAX_HASH] = new_room;
        new_room->next_in_hash = NULL;
     }
   else
     {
        new_room->next_in_hash = hash_world[new_room->vnum % MAX_HASH];
        hash_world[new_room->vnum % MAX_HASH] = new_room;
     }
   
   /* Link to current area. */
   link_to_area( new_room, current_area );
   
   return new_room;
}



AREA_DATA *create_area( )
{
   AREA_DATA *new_area;
   
   DEBUG( "create_area" );
   
   new_area = calloc( sizeof( AREA_DATA ), 1 );
   
   /* Init. */
   new_area->name = NULL;
   new_area->rooms = NULL;
   
   /* Link to main chain. */
   if ( !areas )
     {
        areas = new_area;
        areas_last = new_area;
     }
   else
     areas_last->next = new_area;
   
   new_area->next = NULL;
   areas_last = new_area;
   
   return new_area;
}



EXIT_DATA *create_exit( ROOM_DATA *room )
{
   EXIT_DATA *new_exit, *e;
   
   new_exit = calloc( sizeof ( EXIT_DATA ), 1 );
   
   new_exit->vnum = -1;
   
   /* If room is null, create it in the global list. */
   if ( room )
     {
        if ( room->special_exits )
          {
             e = room->special_exits;
             while ( e->next )
               e = e->next;
             e->next = new_exit;
          }
        else
          {
             room->special_exits = new_exit;
          }
        
        new_exit->next = NULL;
        new_exit->owner = room;
     }
   else
     {
        if ( global_special_exits )
          {
             e = global_special_exits;
             while ( e->next )
               e = e->next;
             e->next = new_exit;
          }
        else
          {
             global_special_exits = new_exit;
          }
        
        new_exit->next = NULL;
        new_exit->owner = NULL;
     }
   
   return new_exit;
}



void free_exit( EXIT_DATA *spexit )
{
   if ( spexit->command )
     free( spexit->command );
   if ( spexit->message )
     free( spexit->message );
   free( spexit );
}



void link_element( ELEMENT *elem, ELEMENT **first )
{
   elem->next = *first;
   if ( elem->next )
     elem->next->prev = elem;
   elem->prev = NULL;
   
   *first = elem;
   elem->first = first;
}



void unlink_element( ELEMENT *elem )
{
   if ( elem->prev )
     elem->prev->next = elem->next;
   else
     *(elem->first) = elem->next;
   
   if ( elem->next )
     elem->next->prev = elem->prev;
}



void unlink_rooms( ROOM_DATA *source, int dir, ROOM_DATA *dest )
{
   REVERSE_EXIT *rexit, *r;
   
   if ( !dir )
     return;
   
   for ( rexit = dest->rev_exits; rexit; rexit = rexit->next )
     if ( rexit->from == source && rexit->direction == dir )
       break;
   
   if ( !rexit )
     return;
   
   /* Unlink. */
   if ( rexit == dest->rev_exits )
     dest->rev_exits = rexit->next;
   else
     {
        for ( r = dest->rev_exits; r->next != rexit; r = r->next );
        r->next = rexit->next;
     }
   
   free( rexit );
   
   source->exits[dir] = NULL;
}



void link_rooms( ROOM_DATA *source, int dir, ROOM_DATA *dest )
{
   REVERSE_EXIT *rexit;
   
   if ( !dir )
     return;
   
   /* Just to be sure. */
   if ( source->exits[dir] )
     unlink_rooms( source, dir, source->exits[dir] );
   
   source->exits[dir] = dest;
   
   rexit = calloc( 1, sizeof( REVERSE_EXIT ) );
   rexit->next = dest->rev_exits;
   rexit->from = source;
   rexit->direction = dir;
   rexit->spexit = NULL;
   
   dest->rev_exits = rexit;
}



void link_special_exit( ROOM_DATA *source, EXIT_DATA *spexit, ROOM_DATA *dest )
{
   REVERSE_EXIT *rexit;
   
   if ( !spexit )
     return;
   
   rexit = calloc( 1, sizeof( REVERSE_EXIT ) );
   rexit->next = dest->rev_exits;
   rexit->from = source;
   rexit->direction = 0;
   rexit->spexit = spexit;
   
   dest->rev_exits = rexit;
}



void unlink_special_exit( ROOM_DATA *source, EXIT_DATA *spexit, ROOM_DATA *dest )
{
   REVERSE_EXIT *rexit, *r;
   
   if ( !spexit )
     return;
   
   for ( rexit = dest->rev_exits; rexit; rexit = rexit->next )
     if ( rexit->from == source && rexit->spexit == spexit )
       break;
   
   if ( !rexit )
     return;
   
   /* Unlink. */
   if ( rexit == dest->rev_exits )
     dest->rev_exits = rexit->next;
   else
     {
        for ( r = dest->rev_exits; r->next != rexit; r = r->next );
        r->next = rexit->next;
     }
   
   free( rexit );
}



void destroy_exit( EXIT_DATA *spexit )
{
   ROOM_DATA *room;
   EXIT_DATA *e;
   
   /* Delete its reverse. */
   if ( ( room = spexit->to ) && spexit->owner )
     {
        unlink_special_exit( spexit->owner, spexit, room );
     }
   
   /* Unlink from room, or global. */
   if ( ( room = spexit->owner ) )
     {
        if ( room->special_exits == spexit )
          room->special_exits = spexit->next;
        else
          for ( e = room->special_exits; e; e = e->next )
            if ( e->next == spexit )
              {
                 e->next = spexit->next;
                 break;
              }
     }
   else
     {
        if ( global_special_exits == spexit )
          global_special_exits = spexit->next;
        else
          for ( e = global_special_exits; e; e = e->next )
            if ( e->next == spexit )
              {
                 e->next = spexit->next;
                 break;
              }
     }
   
   /* Free it up. */
   free_exit( spexit );
}


void free_room( ROOM_DATA *room )
{
   int i;
   
   for ( i = 1; dir_name[i]; i++ )
     if ( room->exits[i] )
       unlink_rooms( room, i, room->exits[i] );
   
   while ( room->rev_exits )
     {
        if ( room->rev_exits->direction )
          unlink_rooms( room->rev_exits->from, room->rev_exits->direction,
                        room );
        else
          destroy_exit( room->rev_exits->spexit );
     }
   
   while ( room->special_exits )
     destroy_exit( room->special_exits );
   
   while ( room->tags )
     unlink_element( room->tags );
   
   if ( room->name )
     free( room->name );
   
   free( room );
}



void destroy_room( ROOM_DATA *room )
{
   ROOM_DATA *r;
   
   unlink_from_area( room );
   
   /* Unlink from world. */
   if ( world == room )
     {
        world = room->next_in_world;
        if ( room == world_last )
          world_last = world;
     }
   else
     for ( r = world; r; r = r->next_in_world )
       {
          if ( r->next_in_world == room )
            {
               r->next_in_world = room->next_in_world;
               if ( room == world_last )
                 world_last = r;
               break;
            }
       }
   
   /* Unlink from hash table. */
   if ( room == hash_world[room->vnum % MAX_HASH] )
     hash_world[room->vnum % MAX_HASH] = room->next_in_hash;
   else
     {
        for ( r = hash_world[room->vnum % MAX_HASH]; r; r = r->next_in_hash )
          {
             if ( r->next_in_hash == room )
               {
                  r->next_in_hash = room->next_in_hash;
                  break;
               }
          }
     }
   
   /* Free it up. */
   free_room( room );
}


void destroy_area( AREA_DATA *area )
{
   AREA_DATA *a;
   
   /* Unlink from areas. */
   if ( area == areas )
     {
        areas = area->next;
        if ( area == areas_last )
          areas_last = areas; /* Always null, anyways. */
     }
   else
     for ( a = areas; a; a = a->next )
       {
          if ( a->next == area )
            {
               a->next = area->next;
               if ( area == areas_last )
                 areas_last = a;
               break;
            }
       }
   
   /* Free it up. */
   if ( area->name )
     free( area->name );
   
   free( area );
}



/* Destroy everything. */
void destroy_map( )
{
   int i;
   
   DEBUG( "destroy_map" );
   
   /* Global special exits. */
   while ( global_special_exits )
     destroy_exit( global_special_exits );
   
   /* Rooms. */
   while ( world )
     destroy_room( world );
   
   /* Areas. */
   while ( areas )
     destroy_area( areas );
   
   /* Hash table. */
   for ( i = 0; i < MAX_HASH; i++ )
     hash_world[i] = NULL;
   
   world = world_last = NULL;
   areas = areas_last = NULL;
   current_room = NULL;
   current_area = NULL;
   
   mode = NONE;
   
   refind_last_vnum( );
}


/* Free -everything- up, and prepare to be destroyed. */
void i_mapper_module_unload( )
{
   del_timer( "queue_reset_timer" );
   del_timer( "remove_players" );
   
   destroy_map( );
}


/* This is what the hash table is for. */
ROOM_DATA *get_room( int vnum )
{
   ROOM_DATA *room;
   
   for ( room = hash_world[vnum % MAX_HASH]; room; room = room->next_in_hash )
     if ( room->vnum == vnum )
       return room;
   
   return NULL;
}



void queue_reset( TIMER *self )
{
   if ( !q_top )
     return;
   
   q_top = 0;
   
   clientff( "\r\n" C_R "[" C_D "Mapper's command queue cleared." C_R "]"
             C_0 "\r\n" );
   show_prompt( );
}


void add_queue( int value )
{
   int i;
   
   for ( i = q_top; i > 0; i-- )
     queue[i] = queue[i-1];
   q_top++;
   queue[0] = value;
   
   add_timer( "queue_reset_timer", 10, queue_reset, 0, 0, 0 );
}


void add_queue_top( int value )
{
   queue[q_top] = value;
   q_top++;
   
   add_timer( "queue_reset_timer", 10, queue_reset, 0, 0, 0 );
}


int must_swim( ROOM_DATA *src, ROOM_DATA *dest )
{
   if ( !src || !dest )
     return 0;
   
   if ( disable_swimming )
     return 0;
   
   if ( ( src->underwater || src->room_type->underwater ) &&
        pear_defence )
     return 0;
   
   if ( src->room_type->must_swim || dest->room_type->must_swim )
     return 1;
   
   return 0;
}


int room_cmp( const char *room, const char *smallr )
{
   if ( !strcmp( room, smallr ) )
     return 0;
   
   return 1;
}



ROOM_DATA *locate_nearby_room( const char *title, int dir )
{
   EXIT_DATA *spexit;
   
   if ( !current_room )
     return NULL;
   
   /* 0: Same room. */
   if ( !dir )
     {
        if ( strcmp( current_room->name, title ) )
          return NULL;
        
        return current_room;
     }
   
   /* >0: Around the room. */
   if ( dir != -1 )
     {
        if ( !current_room->exits[dir] ||
             strcmp( current_room->exits[dir]->name, title ) )
          return NULL;
        
        return current_room->exits[dir];
     }
   
   /* -1: To the 'ether'.. Special exits. */
   for ( spexit = current_room->special_exits; spexit; spexit = spexit->next )
     {
        if ( spexit->to && !strcmp( spexit->to->name, title ) )
          return spexit->to;
     }
   
   return NULL;
}



ROOM_DATA *locate_faraway_room( const char *title, int *detected_exits,
                                int *rooms_found )
{
   ROOM_DATA *r, *found = NULL;
   int matches = 0;
   int i;
   
   for ( r = world; r; r = r->next_in_world )
     if ( !strcmp( title, r->name ) )
       {
          if ( detected_exits )
            {
               int perfect_match = 1;
               for ( i = 1; dir_name[i]; i++ )
               {
                  if ( ( detected_exits[i] ? 1 : 0 ) !=
                       ( ( r->exits[i] || r->detected_exits[i] ) ? 1 : 0 ) )
                    {
                       perfect_match = 0;
                       break;
                    }
               }
               if ( !perfect_match )
                 continue;
            }
          
          /* A match! */
          if ( !found )
            found = r;
          
          matches++;
       }
   
   if ( rooms_found )
     *rooms_found = matches;
   
   return found;
}



void check_title( const char *title, int dir, int *detected_exits )
{
   ROOM_DATA *new_room;
   int created = 0;
   
   if ( mode != CREATING )
     return;
   
   if ( !current_room )
     return;
   
   /* Not just a 'look'? */
   if ( dir )
     {
        if ( current_room->exits[dir] )
          {
             /* Just follow around. */
             new_room = current_room->exits[dir];
          }
        else
          {
             char *color = C_G;
             
             /* Check for autolinking. */
             if ( !link_next_to && !disable_autolink && !switch_exit_stops_mapping )
               {
                  ROOM_DATA *get_room_at( int dir, int length );
                  int length;
                  
                  if ( set_length_to == -1 )
                    length = 1;
                  else
                    length = set_length_to + 1;
                  
                  link_next_to = get_room_at( dir, length );
                  
                  if ( link_next_to && strcmp( title, link_next_to->name ) )
                    link_next_to = NULL;
                  
                  if ( link_next_to && link_next_to->exits[reverse_exit[dir]] )
                    link_next_to = NULL;
                  
                  color = C_C;
               }
             
             /* Create or link an exit. */
             if ( link_next_to )
               {
                  new_room = link_next_to;
                  link_next_to = NULL;
                  clientff( C_R " (%slinked" C_R ")" C_0, color );
               }
             else
               {
                  new_room = create_room( -1 );
                  clientf( C_R " (" C_G "created" C_R ")" C_0 );
                  created = 1;
                  if ( !disable_autolink )
                    check_for_duplicates = 1;
               }
             
             link_rooms( current_room, dir, new_room );
             if ( !unidirectional_exit )
               {
                  if ( !new_room->exits[reverse_exit[dir]] )
                    {
                       link_rooms( new_room, reverse_exit[dir], current_room );
                    }
                  else
                    {
                       current_room->exits[dir] = NULL;
                       clientf( C_R " (" C_G "unlinked: reverse error" C_R ")" );
                    }
               }
             else
               unidirectional_exit = 0;
          }
        
        /* Change the length, if asked so. */
        if ( set_length_to )
          {
             if ( set_length_to == -1 )
               set_length_to = 0;
             
             current_room->exit_length[dir] = set_length_to;
             if ( new_room->exits[reverse_exit[dir]] == current_room )
               new_room->exit_length[reverse_exit[dir]] = set_length_to;
             clientf( C_R " (" C_G "l set" C_R ")" C_0 );
             
             set_length_to = 0;
          }
        
        /* Stop mapping from here on? */
        if ( switch_exit_stops_mapping )
          {
             int i;
             
             i = current_room->exit_stops_mapping[dir];
             
             i = !i;
             
             current_room->exit_stops_mapping[dir] = i;
             if ( new_room->exits[reverse_exit[dir]] == current_room )
               new_room->exit_stops_mapping[reverse_exit[dir]] = i;
             
             if ( i )
               clientf( C_R " (" C_G "s set" C_R ")" C_0 );
             else
               clientf( C_R " (" C_G "s unset" C_R ")" C_0 );
             
             switch_exit_stops_mapping = 0;
          }
        
        /* Show rooms even from another area? */
        if ( switch_exit_joins_areas )
          {
             int i;
             
             i = current_room->exit_joins_areas[dir];
             
             i = !i;
             
             if ( i && ( current_room->area == new_room->area ) )
               clientf( C_R " (" C_G "j NOT set" C_R ")" C_0 );
             else
               {
                  current_room->exit_joins_areas[dir] = i;
                  if ( new_room->exits[reverse_exit[dir]] == current_room )
                    new_room->exit_joins_areas[reverse_exit[dir]] = i;
                  
                  if ( i )
                    clientf( C_R " (" C_G "j set" C_R ")" C_0 );
                  else
                    clientf( C_R " (" C_G "j unset" C_R ")" C_0 );
               }
             
             switch_exit_joins_areas = 0;
          }
        
        /* Show this somewhere else on the map, instead? */
        if ( use_direction_instead )
          {
             if ( use_direction_instead == -1 )
               use_direction_instead = 0;
             
             current_room->use_exit_instead[dir] = use_direction_instead;
             if ( new_room->exits[reverse_exit[dir]] == current_room )
               new_room->use_exit_instead[reverse_exit[dir]] = reverse_exit[use_direction_instead];
             if ( use_direction_instead )
               clientf( C_R " (" C_G "u set" C_R ")" C_0 );
             else
               clientf( C_R " (" C_G "u unset" C_R ")" C_0 );
             
             use_direction_instead = 0;
          }
        
        current_room = new_room;
        current_area = new_room->area;
     }
   
   if ( !strcmp( current_room->name, title ) )
     {
        clientff( C_R " (" C_G "%d" C_R ")" C_0, current_room->vnum );
        
     }
   else if ( allow_title_update || created )
     {
        if ( !created )
          clientf( C_R " (" C_G "updated" C_R ")" C_0 );
        
        if ( current_room->name )
          free( current_room->name );
        current_room->name = strdup( title );
        
        allow_title_update = 0;
     }
   else
     {
        clientf( C_R " (Wrong room name! Automapping disabled)" C_0 );
        mode = GET_UNLOST;
        return;
     }
   
   /* Update exits. */
   
   // TODO
}



/* This is a title. Do something with it. */
int parse_title( const char *line )
{
   ROOM_DATA *new_room;
   int created = 0;
   int q, i;
   
   DEBUG( "parse_title" );
   
   /* Capturing mode. */
   if ( mode == CREATING && capture_special_exit && !q_top )
     {
        EXIT_DATA *spexit;
        
        capture_special_exit = 0;
        new_room = NULL;
        
        if ( !current_room )
          {
             clientf( C_R " (Current Room is NULL! Capturing disabled)" C_0 );
             mode = FOLLOWING;
             return 0;
          }
        
        if ( !cse_message[0] )
          {
             clientf( C_R " (No message found! Capturing disabled)" C_0 );
             mode = FOLLOWING;
             return 0;
          }
        
        if ( special_exit_vnum > 0 )
          {
             new_room = get_room( special_exit_vnum );
             
             if ( !new_room )
               {
                  clientf( C_R " (Destination room is now NULL! Capturing disabled)" C_0 );
                  mode = FOLLOWING;
                  return 0;
               }
             
             /* Make sure the destination matches. */
             if ( strcmp( line, new_room->name ) &&
                  !( similar && !room_cmp( new_room->name, line ) ) )
               {
                  clientf( C_R " (Destination does not match! Capturing disabled)" C_0 );
                  mode = FOLLOWING;
                  return 0;
               }
          }
        else if ( !special_exit_vnum )
          {
             new_room = create_room( -1 );
             new_room->name = strdup( line );
          }
        
        spexit = create_exit( current_room );
        
        if ( special_exit_vnum >= 0 )
          {
             clientff( C_R " (" C_W "sp:" C_G "%d" C_R ")" C_0, new_room->vnum );
             spexit->to = new_room;
             spexit->vnum = new_room->vnum;
             link_special_exit( current_room, spexit, new_room );
          }
        else
          {
             mode = GET_UNLOST;
             spexit->vnum = -1;
          }
        
        clientff( C_R "\r\n[Special exit created.]" );
        
        if ( special_exit_nocommand || !cse_command[0] )
          {
             clientff( C_R "\r\nCommand: " C_W "none" C_R "." C_0 );
             spexit->command = NULL;
          }
        else
          {
             clientff( C_R "\r\nCommand: '" C_W "%s" C_R "'" C_0,
                       cse_command[0] ? cse_command : "null" );
             spexit->command = strdup( cse_command );
          }
        
        clientff( C_R "\r\nMessage: '" C_W "%s" C_R "'" C_0,
                  cse_message );
        spexit->message = strdup( cse_message );
        
        if ( special_exit_alias )
          {
             clientff( C_R "\r\nAlias: '" C_W "%s" C_R "'" C_0,
                       dir_name[special_exit_alias] );
             spexit->alias = special_exit_alias;
          }
        
        if ( new_room )
          {
             current_room = new_room;
             current_area = current_room->area;
          }
        
        // Why was it -1? There must've been a reason..
        return 2;
     }
   
   
   /* Following or Mapping mode. */
   if ( mode == FOLLOWING || mode == CREATING )
     {
        /* Queue empty? */
        if ( !q_top )
          {
             /* parsing_room = 0; */
             return 0;
          }
        
        q = queue[q_top-1];
        q_top--;
        
        if ( !q_top )
          del_timer( "queue_reset_timer" );
        
        if ( !current_room )
          {
             clientf( " (Current room is null, while mapping is not!)" );
             mode = NONE;
             /* parsing_room = 0; */
             return 0;
          }
        
        /* Not just a 'look'? */
        if ( q > 0 )
          {
             if ( mode == FOLLOWING )
               {
                  if ( current_room->exits[q] )
                    {
                       current_room = current_room->exits[q];
                       current_area = current_room->area;
                    }
                  else
                    {
                       /* We moved into a strange exit, while not creating. */
                       clientf( C_R " (" C_G "lost" C_R ")" C_0 );
                       current_room = NULL;
                       mode = GET_UNLOST;
                    }
               }
             
             else if ( mode == CREATING )
               {
                  if ( current_room->exits[q] )
                    {
                       /* Just follow around. */
                       new_room = current_room->exits[q];
                    }
                  else
                    {
                       char *color = C_G;
                       
                       /* Check for autolinking. */
                       if ( !link_next_to && !disable_autolink && !switch_exit_stops_mapping )
                         {
                            ROOM_DATA *get_room_at( int dir, int length );
                            int length;
                            
                            if ( set_length_to == -1 )
                              length = 1;
                            else
                              length = set_length_to + 1;
                            
                            link_next_to = get_room_at( q, length );
                            
                            if ( link_next_to && strcmp( line, link_next_to->name ) )
                              link_next_to = NULL;
                            
                            if ( link_next_to && link_next_to->exits[reverse_exit[q]] )
                              link_next_to = NULL;
                            
                            color = C_C;
                         }
                       
                       /* Create or link an exit. */
                       if ( link_next_to )
                         {
                            new_room = link_next_to;
                            link_next_to = NULL;
                            clientff( C_R " (%slinked" C_R ")" C_0, color );
                         }
                       else
                         {
                            new_room = create_room( -1 );
                            clientf( C_R " (" C_G "created" C_R ")" C_0 );
                            created = 1;
                            if ( !disable_autolink )
                              check_for_duplicates = 1;
                         }
                       
                       link_rooms( current_room, q, new_room );
                       if ( !unidirectional_exit )
                         {
                            if ( !new_room->exits[reverse_exit[q]] )
                              {
                                 link_rooms( new_room, reverse_exit[q], current_room );
                              }
                            else
                              {
                                 current_room->exits[q] = NULL;
                                 clientf( C_R " (" C_G "unlinked: reverse error" C_R ")" );
                              }
                         }
                       else
                         unidirectional_exit = 0;
                    }
                  
                  /* Change the length, if asked so. */
                  if ( set_length_to )
                    {
                       if ( set_length_to == -1 )
                         set_length_to = 0;
                       
                       current_room->exit_length[q] = set_length_to;
                       if ( new_room->exits[reverse_exit[q]] == current_room )
                         new_room->exit_length[reverse_exit[q]] = set_length_to;
                       clientf( C_R " (" C_G "l set" C_R ")" C_0 );
                       
                       set_length_to = 0;
                    }
                  
                  /* Stop mapping from here on? */
                  if ( switch_exit_stops_mapping )
                    {
                       i = current_room->exit_stops_mapping[q];
                       
                       i = !i;
                       
                       current_room->exit_stops_mapping[q] = i;
                       if ( new_room->exits[reverse_exit[q]] == current_room )
                         new_room->exit_stops_mapping[reverse_exit[q]] = i;
                       
                       if ( i )
                         clientf( C_R " (" C_G "s set" C_R ")" C_0 );
                       else
                         clientf( C_R " (" C_G "s unset" C_R ")" C_0 );
                       
                       switch_exit_stops_mapping = 0;
                    }
                  
                  /* Show rooms even from another area? */
                  if ( switch_exit_joins_areas )
                    {
                       i = current_room->exit_joins_areas[q];
                       
                       i = !i;
                       
                       if ( i && ( current_room->area == new_room->area ) )
                         clientf( C_R " (" C_G "j NOT set" C_R ")" C_0 );
                       else
                         {
                            current_room->exit_joins_areas[q] = i;
                            if ( new_room->exits[reverse_exit[q]] == current_room )
                              new_room->exit_joins_areas[reverse_exit[q]] = i;
                            
                            if ( i )
                              clientf( C_R " (" C_G "j set" C_R ")" C_0 );
                            else
                              clientf( C_R " (" C_G "j unset" C_R ")" C_0 );
                         }
                       
                       switch_exit_joins_areas = 0;
                    }
                  
                  
                  /* Show this somewhere else on the map, instead? */
                  if ( use_direction_instead )
                    {
                       if ( use_direction_instead == -1 )
                         use_direction_instead = 0;
                       
                       current_room->use_exit_instead[q] = use_direction_instead;
                       if ( new_room->exits[reverse_exit[q]] == current_room )
                         new_room->use_exit_instead[reverse_exit[q]] = reverse_exit[use_direction_instead];
                       if ( use_direction_instead )
                         clientf( C_R " (" C_G "u set" C_R ")" C_0 );
                       else
                         clientf( C_R " (" C_G "u unset" C_R ")" C_0 );
                       
                       use_direction_instead = 0;
                    }
                  
                  current_room = new_room;
                  current_area = new_room->area;
               }
          }
        
        if ( mode == CREATING )
          {
             if ( !current_room->name || strcmp( line, current_room->name ) )
               {
                  if ( !created )
                    clientf( C_R " (" C_G "updated" C_R ")" C_0 );
                  
                  if ( current_room->name )
                    free( current_room->name );
                  current_room->name = strdup( line );
               }
          }
        else if ( mode == FOLLOWING )
          {
             if ( strcmp( line, current_room->name ) &&
                  !( similar && !room_cmp( current_room->name, line ) ) )
               {
                  /* Didn't enter where we expected to? */
                  clientf( C_R " (" C_G "lost" C_R ")" C_0 );
                  current_room = NULL;
                  mode = GET_UNLOST;
               }
          }
     }
   
   if ( mode == GET_UNLOST )
     {
        ROOM_DATA *r, *found = NULL;
        int more = 0;
        
        for ( r = world; r; r = r->next_in_world )
          {
             if ( !strcmp( line, r->name ) )
               {
                  if ( !found )
                    found = r;
                  else
                    {
                       more = 1;
                       break;
                    }
               }
          }
        
        if ( found )
          {
             current_room = found;
             current_area = found->area;
             mode = FOLLOWING;
             
             get_unlost_exits = more;
          }
     }
   
   if ( mode == CREATING )
     {
        clientff( C_R " (" C_G "%d" C_R ")" C_0, current_room->vnum );
     }
   else if ( mode == FOLLOWING )
     {
        if ( !disable_areaname )
          {
             if ( !godname )
               clientff( C_R " (" C_g "%s" C_R "%s)" C_0,
                         current_room->area->name, get_unlost_exits ? "?" : "" );
             else
               clientff( C_R " (" C_g "%d" C_R "%s)" C_0,
                         current_room->vnum, get_unlost_exits ? "?" : "" );
          }
     }
   
   /* We're ready to move. */
   if ( mode == FOLLOWING && auto_walk == 1 )
     auto_walk = 2;
   
   /* parsing_room = 2; */
   return 2;
}



int check_for_title( char *line, int start_offset )
{
   char *p;
   int end_offset;
   
   /* Format:
    *  Title. (road).
    * or
    *  Title. (road)   (GODNAME1)
    */
   
   /* So... Check for the title name. */
   
   if ( line[0] == '(' )
     return -1;
   
   godname = 0;
   
   end_offset = 0;
   p = line + start_offset;
   while ( *p && *p != '(' )
     {
        if ( *p == '(' )
          break;
        
        if ( ( *p < 'A' || *p > 'Z' ) &&
             ( *p < 'a' || *p > 'z' ) &&
             *p != ',' && *p != '-' && *p != ' ' &&
             *p != '.' && *p != '\'' && *p != '\"' &&
             *p != '&' )
          return -1;
        
        if ( *p == '.' )
          end_offset = p - line + 1;
        p++;
     }
   
   if ( !*p && end_offset == p - line )
     end_offset = 0;
   
   /* Make sure the rest is valid too. */
   
   if ( *p == '(' && ( !strncmp( p, "(path)", 6 ) || !strncmp( p, "(road)", 6 ) ) )
     p += 6;
   
   while ( *p == ' ' )
     p++;
   
   if ( *p == '.' )
     return end_offset;
   
   if ( *p == '(' )
     godname = 1;
   
   /* Remove the road from: "Title. (road)." *
   if ( l->len > 9 && l->line[l->len-2] == ')' &&
        ( eol = strstr( l->line, ". (" ) ) )
     {
        end_offset = ( eol - l->line ) + 1;
     }*/
   
   return end_offset;
}



int parse_exits( LINES *l, int start_at )
{
   int exits_line, word_pos = 0, found_exits = 0, c, i;
   char word[256], *color;
   
   for ( exits_line = start_at; exits_line <= l->nr_of_lines; exits_line++ )
     for ( c = 0; c <= l->len[exits_line]; c++ )
       {
          if ( !found_exits )
            {
               color = l->colour[l->line_start[exits_line] + c];
               if ( color && !strcmp( color, C_B ) )
                 {
                    found_exits = 1, word_pos = 0;
                    
                    if ( mode == CREATING )
                      for ( i = 1; dir_name[i]; i++ )
                        current_room->detected_exits[i] = 0;
                    else if ( get_unlost_exits )
                      for ( i = 1; dir_name[i]; i++ )
                        get_unlost_detected_exits[i] = 0;
                 }
               
            }
          
          if ( found_exits )
            {
               char ch = l->line[exits_line][c];
               
               if ( ( ch < 'A' || ch > 'Z' ) &&
                    ( ch < 'a' || ch > 'z' ) )
                 {
                    if ( word_pos )
                      {
                         /* Check the 'word' so far. */
                         word[word_pos] = 0;
                         
                         if ( mode == CREATING )
                           for ( i = 1; dir_name[i]; i++ )
                             {
                                if ( !strcmp( dir_name[i], word ) )
                                  current_room->detected_exits[i] = 1;
                             }
                         else if ( get_unlost_exits )
                           for ( i = 1; dir_name[i]; i++ )
                             {
                                if ( !strcmp( dir_name[i], word ) )
                                  get_unlost_detected_exits[i] = 1;
                             }
                         
                         word_pos = 0;
                      }
                 }
               else
                 word[word_pos++] = ch;
               
               if ( get_poscolor_at( exits_line, c ) != color )
                 return 0;
            }
       }
   
   return 0;
}



void parse_room2( LINES *l )
{
   int title_offset, end_offset = 0;
   int line, i;
   DEBUG( "parse_room2" );
   
   /* Room title check. */
   for ( line = 1; line <= l->nr_of_lines; line++ )
     {
        if ( !l->len[line] )
          continue;
        
        title_offset = 0;
        if ( !strncmp( l->line[line], "In the trees above ", 19 ) )
          title_offset = 19;
        if ( !strncmp( l->line[line], "Flying above ", 13 ) )
          title_offset = 13;
        
        if ( !strcmp( get_poscolor_at( line, title_offset ), room_color ) )
          {
             end_offset = check_for_title( l->line[line], title_offset );
             if ( end_offset < 0 )
               continue;
          }
        else
          continue;
        
        /* So it looks like a title. */

        set_line( line );
        
        if ( !title_offset && !end_offset )
          i = parse_title( l->line[line] );
        else
          {
             char buf[l->len[line]];
             
             strcpy( buf, l->line[line] + title_offset );
             if ( end_offset )
               buf[end_offset - title_offset] = 0;
             
             i = parse_title( buf );
          }
        
        if ( !i )
          continue;
        
        /* So it -still- looks like a title. Look further for exits, then. */
        
        parse_exits( l, line + 1 );
        
        continue;
     }
}


void parse_room( LINE *l, char *line2, int len )
{
   char *line = l ? l->line : line2;
   static int exit_offset;
   int end_offset;
   int i;
   
   DEBUG( "parse_room" );
   
   /* Room title check. */
   
   if ( similar )
     similar = 0;
   if ( title_offset )
     title_offset = 0;
   
   /* Check the beginning, for a color. */
   if ( !parsing_room )
     {
        title_offset = 0;
        if ( !strncmp( line, "In the trees above ", 19 ) )
          title_offset = 19, similar = 1;
        if ( !strncmp( line, "Flying above ", 13 ) )
          title_offset = 13, similar = 1;
        
        if ( !strcmp( get_poscolor( title_offset ), room_color ) )
          {
             parsing_room = 1;
          }
     }
   
   if ( !len || !parsing_room )
     return;
   
   /* Stage one, room name. */
   if ( parsing_room == 1 )
     {
        /* Check if it actually looks like a room. */
        end_offset = check_for_title( line, title_offset );
        
        if ( end_offset < 0 )
          {
             parsing_room = 0;
             return;
          }
        
        if ( end_offset )
          end_offset -= title_offset;
        
        line += title_offset;
        
        /* Disabled for now. Zmud is just really sucky. *
        if ( !disable_mxp_title )
          {
             char tag[256];
             char buf[256];
             
             if ( mxp_stag( TAG_SECURE, tag ) && tag[0] )
               {
                  sprintf( buf, "x%s<RmTitle>", tag );
                  insert( title_offset, buf );
                  sprintf( buf, "%s</RmTitle>x", tag );
                  insert( end_offset ? end_offset : l->len, buf );
               }
          } */
        
//      debugf( "Room!" );
        
        if ( !end_offset )
          i = parse_title( line );
        else
          {
             char buf[256];
             
             strcpy( buf, line );
             buf[end_offset] = 0;
             
             i = parse_title( buf );
          }
        
        if ( i >= 0 )
          parsing_room = i;
     }
   
   /* Stage two, look for start of exits list. */
   if ( parsing_room == 2 )
     {
        static int sub_stage;
        /* Sub stages:
         * 0 - Looking for a color code.
         * 1 - Found it, looking for 'You'.
         */
        
        if ( !current_room )
          {
             parsing_room = 0;
             return;
          }
        
        for ( i = 0; i < len; i++ )
          {
             /* Blue color. */
             
             if ( !sub_stage && *l->rawp[i] && strstr( l->rawp[i], "34" ) )
               sub_stage = 1;
             
             if ( sub_stage == 1 )
               {
                  //debugf( "(%d)", strlen( l->rawp[i] ) );
                  //insert( i, "X" );
                  if ( l->line[i] == ' ' || l->line[i] == '\0' )
                    continue;
                  
                  if ( l->line[i] == 'Y' && !strncmp( l->line + i, "You", 3 ) )
                    i += 3;
                  else if ( !strncmp( l->line + i, "see", 3 ) )
                    i += 3;
                  else if ( !strncmp( l->line + i, "exits", 5 ) )
                    i += 5;
                  else if ( !strncmp( l->line + i, "a", 1 ) )
                    i += 1;
                  else if ( !strncmp( l->line + i, "single", 6 ) )
                    i += 6;
                  else if ( !strncmp( l->line + i, "exit", 4 ) )
                    i += 4;
                  else if ( !strncmp( l->line + i, "leading", 7 ) )
                    i += 7, line = l->line + i, sub_stage = 2, exit_offset = i;
                  else
                    {
                       sub_stage = 0;
                       continue;
                    }
               }
          }
        
        /* Beginning of exits. */
        if ( sub_stage == 2 )
          {
             if ( mode == CREATING )
               for ( i = 1; dir_name[i]; i++ )
                 current_room->detected_exits[i] = 0;
             else if ( get_unlost_exits )
               for ( i = 1; dir_name[i]; i++ )
                 get_unlost_detected_exits[i] = 0;
             
             sub_stage = 0;
             parsing_room = 3;
             
             /*
             if ( !disable_mxp_exits )
               {
                  char tag[256];
                  char buf[256];
                  
                  if ( mxp_stag( TAG_SECURE, tag ) && tag[0] )
                    {
                       sprintf( buf, "x%s<RmExits>", tag );
                       insert( exit_offset, buf );
                    }
               } */
          }
     }
   
   /* Check for exits. If it got here, then current_room is checked. */
   if ( parsing_room == 3 )
     {
        char word[128];
        char *w;
        int j;
        int beginning, end;
        
        i = exit_offset;
        
        while( i < len )
          {
             w = word;
             /* Extract a word. */
             beginning = i;
             while( line[i] && ( line[i] != ',' && line[i] != ' ' && line[i] != '.' ) )
               *w++ = line[i++];
             *w = 0;
             end = i;
             
             /* Skip spaces and weird stuff. */
             while( line[i] == ',' || line[i] == ' ' )
               i++;
             
             for ( j = 1; dir_name[j]; j++ )
               {
                  if ( !strcmp( word, dir_name[j] ) )
                    {
                       if ( mode == CREATING )
                         current_room->detected_exits[j] = 1;
                       else if ( get_unlost_exits )
                         get_unlost_detected_exits[j] = 1;
                       else
                         {
//                          insert( beginning, "<" );
//                          insert( end, ">" );
//                          debugf( "Exit found: [%s]", word );
                         }
                       break;
                    }
               }
             
             if ( line[i] == '.' )
               {
                  parsing_room = 0;
                  
                  /*
                  if ( !disable_mxp_exits )
                    {
                       char tag[256];
                       char buf[256];
                       
                       if ( mxp_stag( TAG_SECURE, tag ) && tag[0] )
                         {
                            sprintf( buf, "%s</RmExits>y", tag );
                            insert( i, buf );
                         }
                    } */
                  
                  break;
               }
          }
        
        exit_offset = 0;
     }
}



ROOM_TYPE *add_room_type( char *name, char *color )
{
   ROOM_TYPE *type;
   
   for ( type = room_types; type; type = type->next )
     if ( !strcmp( name, type->name ) )
       break;
   
   /* New? Create one more. */
   if ( !type )
     {
        ROOM_TYPE *t;
        
        type = calloc( 1, sizeof( ROOM_TYPE ) );
        type->name = strdup( name );
        type->next = NULL;
        
        if ( !room_types )
          room_types = type;
        else
          {
             for ( t = room_types; t->next; t = t->next );
             t->next = type;
          }
     }
   
   if ( !type->color || ( color && strcmp( type->color, color ) ) )
     type->color = color;
   
   return type;
}



void check_area( char *name )
{
   AREA_DATA *area;
   
   if ( !strcmp( name, current_room->area->name ) )
     return;
   
   /* Area doesn't match. What shall we do? */
   
   if ( mode == FOLLOWING && auto_bump )
     {
        clientf( C_R " (" C_W "doesn't match!" C_R ")" C_0 );
        auto_bump = 0;
     }
   
   if ( mode != CREATING || !current_room )
     return;
   
   /* Check if the user wants to update an existing one. */
   
   if ( update_area_from_survey || !strcmp( current_room->area->name, "New area" ) )
     {
        update_area_from_survey = 0;
        
        for ( area = areas; area; area = area->next )
          if ( !strcmp( area->name, name ) )
            {
               clientf( C_R " (" C_W "warning: duplicate!" C_R ")" C_0 );
               break;
            }
        
        area = current_room->area;
        if ( area->name )
          free( area->name );
        area->name = strdup( name );
        clientf( C_R " (" C_G "updated" C_R ")" C_0 );
        
        return;
     }
     
   /* First, check if the area exists. If it does, switch it. */
   
   for ( area = areas; area; area = area->next )
     if ( !strcmp( area->name, name ) )
       {
          unlink_from_area( current_room );
          link_to_area( current_room, area );
          current_room->area = area;
          current_area = area;
          clientf( C_R " (" C_G "switched!" C_R ")" C_0 );
          
          return;
       }
   
   /* If not, create one. */
   
   area = create_area( );
   area->name = strdup( name );
   unlink_from_area( current_room );
   link_to_area( current_room, area );
   current_room->area = area;
   current_area = area;
   clientf( C_R " (" C_G "new area created" C_R ")" C_0 );
}



void parse_survey( char *line )
{
   char buf[256], *p;
   DEBUG( "parse_survey" );
   
   if ( ( mode == CREATING || auto_bump ) && current_room )
     {
        if ( !cmp( "You discern that you are standing in *", line ) ||
             !cmp( "You stand within *", line ) ||
             !cmp( "You are standing in *", line ) )
          {
             p = buf;
             
             extract_wildcard( 0, buf, 256 );
             
             /* Might not have a "the" in there, rare cases though. */
             if ( !strncmp( p, "the ", 4 ) )
               p += 4;
             
             check_area( p );
          }
        else if ( !cmp( "Your environment conforms to that of *.", line ) )
          {
             ROOM_TYPE *type;
             
             extract_wildcard( 0, buf, 256 );
             
             for ( type = room_types; type; type = type->next )
               if ( !strcmp( buf, type->name ) )
                 break;
             
             /* Lusternia - lowercase letters. - Del
             if ( !type )
               {
                  char buf[256];
                  
                  for ( type = room_types; type; type = type->next )
                    {
                       strcpy( buf, type->name );
                       
                       buf[0] -= ( 'A' - 'a' );
                       
                       if ( !strncmp( line, buf, strlen( buf ) ) )
                         break;
                    }
               } */
             
             if ( type != current_room->room_type )
               {
                  if ( mode == CREATING )
                    {
                       if ( type )
                         clientf( C_R " (" C_G "updated" C_R ")" C_0 );
                       else
                         {
                            type = add_room_type( buf, NULL );
                            clientf( C_R " (" C_G "new type created" C_R ")" C_0 );
                            clientf( C_R "\r\n[Don't forget to use 'room type' to properly set this environment!]" C_0 );
                         }
                       
                       current_room->room_type = type;
                    }
                  else if ( auto_bump )
                    {
                       clientf( C_R " (" C_W "doesn't match!" C_R ")" C_0 );
                       auto_bump = 0;
                    }
               }
          }
        else if ( !cmp( "You cannot glean any information about your surroundings.", line ) )
          {
             ROOM_TYPE *type;
             
             for ( type = room_types; type; type = type->next )
               {
                  if ( !strcmp( "Undefined", type->name ) )
                    {
                       if ( type != current_room->room_type )
                         {
                            if ( mode == CREATING )
                              {
                                 current_room->room_type = type;
                                 
                                 clientf( C_R " (" C_G "updated" C_R ")" C_0 );
                              }
                            else if ( auto_bump )
                              {
                                 clientf( C_R " (" C_W "doesn't match!" C_R ")" C_0 );
                                 auto_bump = 0;
                              }
                         }
                       
                       break;
                    }
               }
          }
     }
}

   


int can_dash( ROOM_DATA *room )
{
   int dir;
   
   if ( !dash_command )
     return 0;
   
   dir = room->pf_direction;
   
   /* First of all, check if we have more than one room. */
   if ( dir <= 0 || !room->exits[dir] ||
        room->exits[dir]->pf_direction != dir )
     return 0;
   
   /* Now check if we have a clear way, till the end. */
   while( room )
     {
        if ( room->pf_direction != dir && room->exits[dir] )
          return 0;
        if ( room->exits[dir] && room->exits[dir]->room_type->must_swim )
          return 0;
        
        room = room->exits[dir];
     }
   
   return 1;
}



void go_next( )
{
   EXIT_DATA *spexit;
   char buf[256];
   
   if ( !current_room )
     {
        clientf( C_R "[No current_room.]" C_0 );
        auto_walk = 0;
     }
   else if ( !current_room->pf_parent )
     {
        auto_walk = 0;
        clientff( C_R "(" C_G "Done." C_R ") " C_0 );
     }
   else
     {
        if ( current_room->pf_direction != -1 )
          {
             if ( door_closed )
               {
                  sprintf( buf, "open %s", dir_name[current_room->pf_direction] );
                  clientfr( buf );
                  sprintf( buf, "open door %s\r\n", dir_small_name[current_room->pf_direction] );
                  send_to_server( buf );
               }
             else if ( door_locked )
               {
                  door_locked = 0;
                  door_closed = 0;
                  auto_walk = 0;
                  clientff( C_R "\r\n[Locked room " C_W "%s" C_R " of v" C_W "%d" C_R ". Speedwalking disabled.]\r\n" C_0,
                            dir_name[current_room->pf_direction], current_room->vnum );
                  show_prompt( );
                  return;
               }
             else
               {
                  if ( !gag_next_prompt )
                    clientff( C_R "(" C_r "%d" C_R " - %s) " C_0, current_room->pf_cost - 1, dir_name[current_room->pf_direction] );
                  
                  if ( must_swim( current_room, current_room->pf_parent ) )
                    send_to_server( "swim " );
                  
                  if ( !must_swim( current_room, current_room->pf_parent ) &&
                       dash_command && can_dash( current_room ) )
                    send_to_server( dash_command );
                  else
                    {
                       if ( mode == FOLLOWING || mode == CREATING )
                         add_queue( current_room->pf_direction );
                    }
                  
                  send_to_server( dir_small_name[current_room->pf_direction] );
                  send_to_server( "\r\n" );
               }
          }
        else
          {
             for ( spexit = current_room->special_exits; spexit; spexit = spexit->next )
               {
                  if ( spexit->to == current_room->pf_parent &&
                       spexit->command )
                    {
                       clientff( C_R "(" C_D "%s" C_R ") " C_0, spexit->command );
                       send_to_server( spexit->command );
                       send_to_server( "\r\n" );
                       break;
                    }
               }
          }
        
        auto_walk = 1;
     }
}



void add_exit_char( char *var, int dir )
{
   const char dir_chars[] = 
     { ' ', '|', '/', '-', '\\', '|', '/', '-', '\\' };
   
   if ( ( *var == '|' && dir_chars[dir] == '-' ) ||
        ( *var == '-' && dir_chars[dir] == '|' ) )
     *var = '+';
   else if ( ( *var == '/' && dir_chars[dir] == '\\' ) ||
             ( *var == '\\' && dir_chars[dir] == '/' ) )
     *var = 'X';
   else
     *var = dir_chars[dir];
}



void fill_map( ROOM_DATA *room, AREA_DATA *area, int x, int y )
{
   static int xi[] = { 2, 0, 1, 1, 1, 0, -1, -1, -1, 2, 2, 2, 2, 2 };
   static int yi[] = { 2, -1, -1, 0, 1, 1, 1, 0, -1, 2, 2, 2, 2, 2 };
   int i;
   
   if ( room->area != area )
     return;
   
   if ( room->mapped )
     return;
   
   if ( x < 0 || x >= MAP_X+1 || y < 0 || y >= MAP_Y+1 )
     return;
   
   room->mapped = 1;
   /* We'll have to clean all these room->mapped too. */
   room->area->needs_cleaning = 1;
   
   if ( map[x][y] )
     return;
   
   map[x][y] = room;
   
   for ( i = 1; dir_name[i]; i++ )
     {
        if ( !room->exits[i] )
          continue;
        
        if ( room->exit_stops_mapping[i] )
          continue;
        
        if ( room->exit_length[i] > 0 && !room->use_exit_instead[i] )
          {
             int j;
             int real_x, real_y;
             
             for( j = 1; j <= room->exit_length[i]; j++ )
               {
                  real_x = x + ( xi[i] * j );
                  real_y = y + ( yi[i] * j );
                  
                  if ( real_x < 0 || real_x >= MAP_X+1 ||
                       real_y < 0 || real_y >= MAP_Y+1 )
                    continue;
                  
                  add_exit_char( &extra_dir_map[real_x][real_y], i );
               }
          }
        
        if ( room->use_exit_instead[i] )
          {
             int ex = room->use_exit_instead[i];
             fill_map( room->exits[i], area,
                       x + ( xi[ex] * ( 1 + room->exit_length[i] ) ),
                       y + ( yi[ex] * ( 1 + room->exit_length[i] ) ) );
          }
        else if ( xi[i] != 2 )
          fill_map( room->exits[i], area,
                    x + ( xi[i] * ( 1 + room->exit_length[i] ) ),
                    y + ( yi[i] * ( 1 + room->exit_length[i] ) ) );
     }
}




/* One little nice thing. */
int has_exit( ROOM_DATA *room, int direction )
{
   if ( room )
     {
        if ( room->exits[direction] )
          {
             if ( room->exit_stops_mapping[direction] )
               return 4;
             else if ( room->area != room->exits[direction]->area )
               return 3;
             else if ( room->pf_highlight && room->pf_direction == direction )
               return 5;
             else
               return 1;
          }
        else if ( room->detected_exits[direction] )
          return 2;
     }
   
   return 0;
}


/*
 * 
 * [ ]- [ ]- [ ]
 *  | \  | / 
 * 
 * [ ]- [*]- [ ]
 *  | /  | \ 
 * 
 * [ ]  [ ]  [ ]
 * 
 */

void set_exit( short *ex, ROOM_DATA *room, int dir )
{
   /* Careful. These are |=, not != */
   
   if ( room->exits[dir] )
     {
        *ex |= EXIT_NORMAL;
        
        if ( room->exits[dir]->area != room->area )
          *ex |= EXIT_OTHER_AREA;
     }
   else if ( room->detected_exits[dir] )
       *ex |= EXIT_UNLINKED;
   
   if ( room->locked_exits[dir] )
     *ex |= EXIT_LOCKED;
   
   if ( room->exit_stops_mapping[dir] )
     *ex |= EXIT_STOPPING;
   
   if ( room->pf_highlight && room->pf_parent == room->exits[dir] )
     *ex |= EXIT_PATH;
}



void set_all_exits( ROOM_DATA *room, int x, int y )
{
   int i;
   
   /* East, southeast, south. (current element) */
   set_exit( &map_new[x][y].e, room, EX_E );
   set_exit( &map_new[x][y].se, room, EX_SE );
   set_exit( &map_new[x][y].s, room, EX_S );
   
   /* Northwest. (northwest element) */
   if ( x && y )
     {
        set_exit( &map_new[x-1][y-1].se, room, EX_NW );
     }
   
   /* North, northeast. (north element) */
   if ( y )
     {
        set_exit( &map_new[x][y-1].s, room, EX_N );
        set_exit( &map_new[x][y-1].se_rev, room, EX_NE );
     }
   
   /* West, southwest. (west element) */
   if ( x )
     {
        set_exit( &map_new[x-1][y].e, room, EX_W );
        set_exit( &map_new[x-1][y].se_rev, room, EX_SW );
     }
   
   /* In, out. */
   map_new[x][y].in_out |= ( room->exits[EX_IN] || room->exits[EX_OUT] ) ? 1 : 0;
   
   /* Check for unlinked exits here, and set a warning if needed. */
   if ( mode == CREATING )
     for ( i = 1; dir_name[i]; i++ )
       if ( room->detected_exits[i] && !room->exits[i] &&
            !room->locked_exits[i] )
         {
            map_new[x][y].warn = 2;
            warn_unlinked = 1;
         }
}



/* Remake of fill_map. */
void fill_map_new( ROOM_DATA *room, int x, int y )
{
   const int xi[] = { 2, 0, 1, 1, 1, 0, -1, -1, -1, 2, 2, 2, 2, 2 };
   const int yi[] = { 2, -1, -1, 0, 1, 1, 1, 0, -1, 2, 2, 2, 2, 2 };
   int i;
   
   if ( map_new[x][y].room || room->mapped )
     return;
   
   if ( x < 0 || x >= MAP_X || y < 0 || y >= MAP_Y )
     return;
   
   room->mapped = 1;
   room->area->needs_cleaning = 1;
   map_new[x][y].room = room;
   map_new[x][y].color = room->room_type->color;
   /* Unset? Make it obvious. */
   if ( mode == CREATING &&
        ( !map_new[x][y].color || room->room_type == null_room_type ) )
     {
        warn_environment = 1;
        map_new[x][y].warn = 1;
     }
   if ( !map_new[x][y].color )
     map_new[x][y].color = C_R;
   
   set_all_exits( room, x, y );
   
   for ( i = 1; dir_name[i]; i++ )
     {
        if ( !room->exits[i] || room->exit_stops_mapping[i] ||
             ( ( room->exits[i]->area != room->area ) && !room->exit_joins_areas[i] ) )
          continue;
        
        /* Normal exit. */
        if ( !room->use_exit_instead[i] && !room->exit_length[i] )
          {
             if ( xi[i] != 2 && yi[i] != 2 )
               fill_map_new( room->exits[i], x + xi[i], y + yi[i] );
             continue;
          }
        
        /* Exit changed. */
          {
             int new_i, new_x, new_y;
             
             new_i = room->use_exit_instead[i] ? room->use_exit_instead[i] : i;
             
             if ( xi[new_i] != 2 && yi[new_i] != 2 )
               {
                  int j;
                  
                  new_x = x + xi[new_i] * ( 1 + room->exit_length[i] );
                  new_y = y + yi[new_i] * ( 1 + room->exit_length[i] );
                  
                  fill_map_new( room->exits[i], new_x, new_y );
                  
                  for( j = 1; j <= room->exit_length[i]; j++ )
                    {
                       new_x = x + ( xi[new_i] * j );
                       new_y = y + ( yi[new_i] * j );
                       
                       if ( new_x < 0 || new_x >= MAP_X ||
                            new_y < 0 || new_y >= MAP_Y )
                         break;
                       
                       /* 
                        * This exit will be displayed instead of the center
                        * of a room.
                        */
                       
                       if ( i == EX_N || i == EX_S )
                         map_new[new_x][new_y].extra_s = 1;
                       
                       else if ( i == EX_E || i == EX_W )
                         map_new[new_x][new_y].extra_e = 1;
                       
                       else if ( i == EX_SE || i == EX_NW )
                         map_new[new_x][new_y].extra_se = 1;
                       
                       else if ( i == EX_NE || i == EX_SW )
                         map_new[new_x][new_y].extra_se_rev = 1;
                    }
               }
          }
     }
}


/* Total remake of show_map. */
/* All calls to strcat/sprintf have been replaced with byte by byte
 * processing. The result has been a dramatical increase in speed. */

void show_map_new( ROOM_DATA *room )
{
   AREA_DATA *a;
   ROOM_DATA *r;
   char map_buf[65536], buf[64], *p, *s, *s2;
   char vnum_buf[1024];
   int x, y, len, len2, loop;
   int use_mxp = 0;
   
   DEBUG( "show_map_new" );
   
   if ( !room )
     return;
   
   get_timer( );
   
   for ( x = 0; x < MAP_X; x++ )
     for ( y = 0; y < MAP_Y; y++ )
       memset( &map_new[x][y], 0, sizeof( MAP_ELEMENT ) );
   
   /* Pathfinder - Go around and set which rooms to highlight. */
   for ( r = room->area->rooms; r; r = r->next_in_area )
     r->pf_highlight = 0;
   
   if ( room->pf_parent )
     {
        for ( r = room, loop = 0; r->pf_parent && loop < 100; r = r->pf_parent, loop++ )
          r->pf_highlight = 1;
     }
   
   warn_misaligned = 0;
   warn_environment = 0;
   warn_unlinked = 0;
   
   /* From the current *room, place all other rooms on the map. */
   fill_map_new( room, MAP_X / 2, MAP_Y / 2 );
   
   
   /* Build it up. */
   
   
   /* Are we able to get a SECURE mode on MXP? */
   if ( !disable_mxp_map && mxp_tag( TAG_LOCK_SECURE ) )
     use_mxp = 1;
   
   /* Upper banner. */
   
   map_buf[0] = 0;
   p = map_buf;
   
   sprintf( vnum_buf, "v%d", room->vnum );
   
   s = C_0 "/--" C_C;
   while( *s )
     *(p++) = *(s++);
   s = room->area->name, len = 0;
   while( *s )
     *(p++) = *(s++), len++;
   s = C_0;
   while( *s )
     *(p++) = *(s++);
   len2 = MAP_X * 4 - 7 - strlen( vnum_buf );
   for ( x = len; x < len2; x++ )
     *(p++) = '-';
   s = C_C;
   while( *s )
     *(p++) = *(s++);
   s = vnum_buf;
   while( *s )
     *(p++) = *(s++);
   s = C_0 "--\\\r\n";
   while( *s )
     *(p++) = *(s++);
   *p = 0;
   
   /* The map, row by row. */
   for ( y = 0; y < MAP_Y; y++ )
     {
        /* (1) [o]- */
        /* (2)  | \ */
        
        /* (1) */
        
        if ( y )
          {
             *(p++) = ' ';
             
             for ( x = 0; x < MAP_X; x++ )
               {
                  if ( x )
                    {
                       if ( map_new[x][y].room )
                         {
                            if ( !map_new[x][y].room->underwater )
                              s = map_new[x][y].color;
                            else
                              s = C_C;
                            
                            while( *s )
                              *(p++) = *(s++);
                            *(p++) = '[';
                            
                            if ( use_mxp )
                              {
                                 if ( map_new[x][y].room->person_here )
                                   sprintf( vnum_buf, "<mppers v=%d r=\"%s\" t=\"%s\" p=\"%s\">",
                                            map_new[x][y].room->vnum, map_new[x][y].room->name,
                                            map_new[x][y].room->room_type->name, map_new[x][y].room->person_here );
                                 else
                                   sprintf( vnum_buf, "<mpelm v=%d r=\"%s\" t=\"%s\">",
                                            map_new[x][y].room->vnum, map_new[x][y].room->name,
                                            map_new[x][y].room->room_type->name );
                                 s = vnum_buf;
                                 while( *s )
                                   *(p++) = *(s++);
                              }
                            
                            if ( map_new[x][y].room == current_room )
                              s = C_B "*";
                            else if ( map_new[x][y].warn == 1 )
                              s = C_Y "!";
                            else if ( map_new[x][y].warn == 2 )
                              s = C_r "!";
                            else if ( map_new[x][y].room->person_here )
                              s = C_Y "*";
                            else if ( map_new[x][y].in_out )
                              s = C_r "o";
                            else if ( use_mxp )
                              s = C_d " ";
                            else
                              s = " ";
                            while( *s )
                              *(p++) = *(s++);
                            
                            if ( use_mxp )
                              {
                                 if ( map_new[x][y].room->person_here )
                                   s = "</mppers>";
                                 else
                                   s = "</mpelm>";
                                 while( *s )
                                   *(p++) = *(s++);
                              }
                            
                            if ( !map_new[x][y].room->underwater )
                              s = map_new[x][y].color;
                            else
                              s = C_C;
                            
                            while( *s )
                              *(p++) = *(s++);
                            s = "]" C_0;
                            while( *s )
                              *(p++) = *(s++);
                         }
                       else
                         {
                            *(p++) = ' ';
                            
                            if ( map_new[x][y].extra_e && map_new[x][y].extra_s )
                              *(p++) = '+';
                            else if ( map_new[x][y].extra_se && map_new[x][y].extra_se_rev )
                              *(p++) = 'X';
                            else if ( map_new[x][y].extra_e )
                              *(p++) = '-';
                            else if ( map_new[x][y].extra_s )
                              *(p++) = '|';
                            else if ( map_new[x][y].extra_se )
                              *(p++) = '\\';
                            else if ( map_new[x][y].extra_se_rev )
                              *(p++) = '/';
                            else
                              *(p++) = ' ';
                            
                            *(p++) = ' ';
                         }
                    }
                  
                  /* Exit color. */
                  if ( map_new[x][y].e & EXIT_OTHER_AREA )
                    s = C_W, s2 = C_0;
                  else if ( map_new[x][y].e & EXIT_STOPPING )
                    s = C_R, s2 = C_0;
                  else if ( map_new[x][y].e & EXIT_LOCKED )
                    s = C_r, s2 = C_0;
                  else if ( map_new[x][y].e & EXIT_UNLINKED )
                    s = C_D, s2 = C_0;
                  else if ( map_new[x][y].e & EXIT_PATH )
                    s = C_B, s2 = C_0;
                  else
                    s = "", s2 = "";
                  while( *s )
                    *(p++) = *(s++);
                  
                  *(p++) = map_new[x][y].e ? '-' : ' ';
                  
                  while( *s2 )
                    *(p++) = *(s2++);
               }
             
             *(p++) = '\r';
             *(p++) = '\n';
          }
        
        /* (2) */
        
        for ( x = 0; x < MAP_X; x++ )
          {
             *(p++) = ' ';
             if ( x )
               {
                  /* Exit color. */
                  if ( map_new[x][y].s & EXIT_OTHER_AREA )
                    s = C_W, s2 = C_0;
                  else if ( map_new[x][y].s & EXIT_STOPPING )
                    s = C_R, s2 = C_0;
                  else if ( map_new[x][y].s & EXIT_LOCKED )
                    s = C_r, s2 = C_0;
                  else if ( map_new[x][y].s & EXIT_UNLINKED )
                    s = C_D, s2 = C_0;
                  else if ( map_new[x][y].s & EXIT_PATH )
                    s = C_B, s2 = C_0;
                  else
                    s = "", s2 = "";
                  while( *s )
                    *(p++) = *(s++);
                  
                  *(p++) = map_new[x][y].s ? '|' : ' ';
                  
                  while( *s2 )
                    *(p++) = *(s2++);

                  *(p++) = ' ';
               }
             
             /* Exit color. */
             if ( ( map_new[x][y].se | map_new[x][y].se_rev ) & EXIT_OTHER_AREA )
               s = C_W, s2 = C_0;
             else if ( ( map_new[x][y].se | map_new[x][y].se_rev ) & EXIT_STOPPING )
               s = C_R, s2 = C_0;
             else if ( ( map_new[x][y].se | map_new[x][y].se_rev ) & EXIT_LOCKED )
               s = C_r, s2 = C_0;
             else if ( ( map_new[x][y].se | map_new[x][y].se_rev ) & EXIT_UNLINKED )
               s = C_D, s2 = C_0;
             else if ( ( map_new[x][y].se | map_new[x][y].se_rev ) & EXIT_PATH )
               s = C_B, s2 = C_0;
             else
               s = "", s2 = "";
             while( *s )
               *(p++) = *(s++);
             
             *(p++) = ( map_new[x][y].se ?
                        ( map_new[x][y].se_rev ? 'X' : '\\' ) :
                        map_new[x][y].se_rev ? '/' : ' ' );
             
             while( *s2 )
               *(p++) = *(s2++);
          }
        
        *(p++) = '\r';
        *(p++) = '\n';
     }
   
   /* Lower banner. */
   s = "\\--";
   while( *s )
     *(p++) = *(s++);
   sprintf( buf, "Time: %d usec", get_timer( ) );
   s = buf, len = 0;
   while( *s )
     *(p++) = *(s++), len++;
   for ( x = len; x < MAP_X * 4 - 7 - ( mode == CREATING )*6; x++ )
     *(p++) = '-';
   
   /* Map correctness indicators */
   if ( mode == CREATING )
     {
        s = warn_unlinked ? "-" C_R "l" C_0 : "-" C_C "*" C_0;
        while( *s )
          *(p++) = *(s++);
        s = warn_environment ? "-" C_R "s" C_0 : "-" C_C "*" C_0;
        while( *s )
          *(p++) = *(s++);
        s = warn_misaligned ? "-" C_R "a" C_0 : "-" C_C "*" C_0;
        while( *s )
          *(p++) = *(s++);
     }
   
   s = "--/\r\n";
   while( *s )
     *(p++) = *(s++);
   
   *p = 0;
   
   /* Clear up our mess. */
   for ( a = areas; a; a = a->next )
     if ( a->needs_cleaning )
       {
          for ( r = a->rooms; r; r = r->next_in_area )
            r->mapped = 0;
          a->needs_cleaning = 0;
       }
   
   /* Show it away. */
   clientf( map_buf );
   
   /* And return MXP to default. */
   if ( use_mxp )
     mxp_tag( TAG_DEFAULT );
}



/* This will get the room that would be shown on the map somewhere. */
ROOM_DATA *get_room_at( int dir, int length )
{
   const int xi[] = { 2, 0, 1, 1, 1, 0, -1, -1, -1, 2, 2, 2, 2, 2 };
   const int yi[] = { 2, -1, -1, 0, 1, 1, 1, 0, -1, 2, 2, 2, 2, 2 };
   AREA_DATA *a;
   ROOM_DATA *r;
   int x, y;
   
   if ( !current_room )
     return NULL;
   
   for ( x = 0; x < MAP_X; x++ )
     for ( y = 0; y < MAP_Y; y++ )
       memset( &map_new[x][y], 0, sizeof( MAP_ELEMENT ) );
   
   /* Convert the angle/length to a relative position. */
   x = xi[dir], y = yi[dir];
   if ( x == 2 || y == 2 )
     return NULL;
   x *= length, y *= length;
   
   /* Convert them from relative to absolute. */
   x = ( MAP_X / 2 ) + x;
   y = ( MAP_Y / 2 ) + y;
   
   if ( x < 0 || y < 0 || x >= MAP_X || y >= MAP_Y )
     return NULL;
   
   fill_map_new( current_room, MAP_X / 2, MAP_Y / 2 );
   
   /* Clear up our mess. */
   for ( a = areas; a; a = a->next )
     if ( a->needs_cleaning )
       {
          for ( r = a->rooms; r; r = r->next_in_area )
            r->mapped = 0;
          a->needs_cleaning = 0;
       }
   
   return map_new[x][y].room;
}



/* One big ugly thing. */
/* It was way too ugly, so it was replaced. */
void show_map( ROOM_DATA *room )
{
   ROOM_DATA *troom;
   char big_buffer[4096];
   char dir_map[MAP_X*2+4][MAP_Y*2+4];
   char *dir_color_map[MAP_X*2+4][MAP_Y*2+4];
   char buf[128], room_buf[128];
   int x, y;
   char *dir_colors[] = { "", "", C_D, C_W, C_R, C_B };
   int d1, d2;
   int loop;
   
   DEBUG( "show_map" );
   
   get_timer( );
   get_timer( );
//   debugf( "--map--" );
   
   big_buffer[0] = 0;
   
//   debugf( "1: %d", get_timer( ) );
   
   for ( troom = room->area->rooms; troom; troom = troom->next_in_area )
     troom->mapped = 0;
   
   for ( x = 0; x < MAP_X+1; x++ )
     for ( y = 0; y < MAP_Y+1; y++ )
       map[x][y] = NULL, extra_dir_map[x][y] = ' ';
   
   for ( x = 0; x < MAP_X*2+2; x++ )
     for ( y = 0; y < MAP_Y*2+2; y++ )
       dir_map[x][y] = ' ', dir_color_map[x][y] = "";
   
   /* Pathfinder - Go around and set which rooms to highlight. */
   for ( troom = room->area->rooms; troom; troom = troom->next_in_area )
     troom->pf_highlight = 0;
   
   if ( room->pf_parent )
     {
        for ( troom = room, loop = 0; troom && loop < 100; troom = troom->pf_parent, loop++ )
          troom->pf_highlight = 1;
     }
   
//   debugf( "2: %d", get_timer( ) );
   
   /* Build rooms. */
   fill_map( room, room->area, (MAP_X+1) / 2, (MAP_Y+1) / 2 );
   
//   debugf( "3: %d", get_timer( ) );
   
   /* Build directions. */
   for ( y = 0; y < MAP_Y+1-1; y++ )
     {
        for ( x = 0; x < MAP_X+1-1; x++ )
          {
             /* [ ]-  (01) */
             /*  | X  (11) */
             
             /* d1 and d2 should be: 1-normal, 2-only detected, 3-another area, 4-stopped mapping, 5-pathfinder. */
             dir_map[x*2][y*2] = ' ';
             
             /* South. */
             d1 = map[x][y] ? has_exit( map[x][y], EX_S ) : 0;
             d2 = map[x][y+1] ? has_exit( map[x][y+1], EX_N ) : 0;
             
             if ( d1 || d2 )
               dir_map[x*2][y*2+1] = '|';
             
             dir_color_map[x*2][y*2+1] = dir_colors[d1>d2 ? d1 : d2];
             
             /* East. */
             d1 = map[x][y] ? has_exit( map[x][y], EX_E ) : 0;
             d2 = map[x+1][y] ? has_exit( map[x+1][y], EX_W ) : 0;
             
             if ( d1 || d2 )
               dir_map[x*2+1][y*2] = '-';
             
             dir_color_map[x*2+1][y*2] = dir_colors[d1>d2 ? d1 : d2];
             
             /* Southeast. (\) */
             d1 = map[x][y] ? has_exit( map[x][y], EX_SE ) : 0;
             d2 = map[x+1][y+1] ? has_exit( map[x+1][y+1], EX_NW ) : 0;
             
             if ( d1 || d2 )
               dir_map[x*2+1][y*2+1] = '\\';
             
             dir_color_map[x*2+1][y*2+1] = dir_colors[d1>d2 ? d1 : d2];
             
             /* Southeast. (/) */
             d1 = map[x+1][y] ? has_exit( map[x+1][y], EX_SW ) : 0;
             d2 = map[x][y+1] ? has_exit( map[x][y+1], EX_NE ) : 0;
             
             if ( d1 || d2 )
               {
                  if ( dir_map[x*2+1][y*2+1] == ' ' )
                    dir_map[x*2+1][y*2+1] = '/';
                  else
                    dir_map[x*2+1][y*2+1] = 'X';
               }
             
             if ( dir_color_map[x*2+1][y*2+1] != C_D &&
                  dir_colors[d1>d2 ? d1 : d2][0] != 0 )
               dir_color_map[x*2+1][y*2+1] = dir_colors[d1>d2 ? d1 : d2];
          }
     }
   
//   debugf( "4: %d", get_timer( ) );
   
   /* And now combine them. */
   strcat( big_buffer, "/--" "\33[1;36m" );
   strcat( big_buffer, room->area->name );
   strcat( big_buffer, C_0 );
   for ( x = 2+strlen( room->area->name ); x < (MAP_X-2+1)*4+1; x++ )
     strcat( big_buffer, "-" );
   strcat( big_buffer, "\\\r\n" );
   
//   debugf( "5: %d", get_timer( ) );
   
   for ( y = 0; y < MAP_Y-1+1; y++ )
     {
        if ( y > 0 )
          {
             /* [o]- */
             
             /* *[*o*]*- */
             
             strcat( big_buffer, " " );
             for ( x = 0; x < MAP_X-1+1; x++ )
               {
                  if ( x == 0 )
                    sprintf( buf, "%s%c%s", dir_color_map[x*2+1][y*2], dir_map[x*2+1][y*2],
                             dir_color_map[x*2+1][y*2][0] != 0 ? C_0 : "" );
                  else
                    {
                       char *color;
                       char *center;
                       char str[2];
                       
                       if ( !map[x][y] )
                         color = "";
                       else if ( map[x][y]->underwater )
                         color = "\33[1;36m";
                       else
                         color = map[x][y]->room_type->color;
                       if ( !color )
                         {
                            color = C_R;
                            // warn
                         }
                       
                       if ( !map[x][y] )
                         {
                            /* Ugh! */
                            sprintf( str, "%c", extra_dir_map[x][y] );
                            center = str;
                         }
                       else if ( map[x][y] == current_room )
                         center = C_B "*";
                       else if ( map[x][y]->exits[EX_IN] || map[x][y]->exits[EX_OUT] )
                         center = C_r "o";
                       else
                         center = " ";
                       
                       sprintf( room_buf, "%s%s%s%s%s",
                                color, map[x][y] ? "[" : " ",
                                center,
                                color, map[x][y] ? "]" C_0 : " " );
                       
                       sprintf( buf, "%s%s%c%s", room_buf,
                                dir_color_map[x*2+1][y*2], 
                                dir_map[x*2+1][y*2],
                                dir_color_map[x*2+1][y*2][0] != 0 ? C_0 : "" );
                    }
                  strcat( big_buffer, buf );
               }
             strcat( big_buffer, "\r\n" );
          }
        strcat( big_buffer, " " );
        
        for ( x = 0; x < MAP_X-1+1; x++ )
          {
             /*  | X */
             
             if ( x == 0 )
               sprintf( buf, "%s%c%s", dir_color_map[x*2+1][y*2+1], dir_map[x*2+1][y*2+1],
                        dir_color_map[x*2+1][y*2+1][0] != 0 ? C_0 : "" );
             else
               sprintf( buf, " %s%c%s %s%c%s", dir_color_map[x*2][y*2+1], dir_map[x*2][y*2+1],
                        dir_color_map[x*2][y*2+1][0] != 0 ? C_0 : "", dir_color_map[x*2+1][y*2+1],
                        dir_map[x*2+1][y*2+1], dir_color_map[x*2+1][y*2+1][0] != 0 ? C_0 : "" );
             strcat( big_buffer, buf );
          }
        strcat( big_buffer, "\r\n" );
     }
   
//   debugf( "6: %d", get_timer( ) );
   
   strcat( big_buffer, "\\--" );
   sprintf( buf, "Time: %d usec", get_timer( ) );
   strcat( big_buffer, buf );
   for ( x = 2 + strlen( buf ); x < (MAP_X-2+1)*4+1; x++ )
     strcat( big_buffer, "-" );
   strcat( big_buffer, "/\r\n" );
   
   /* Now let's send this big monster we've just created... */
   clientf( big_buffer );
}



void show_floating_map( ROOM_DATA *room )
{
   if ( !floating_map_enabled || !room )
     return;
   
   if ( !mxp_tag( TAG_LOCK_SECURE ) )
     return;
   
   mxp( "<DEST IMapper X=0 Y=0>" );
   mxp_tag( TAG_LOCK_SECURE );
   
   skip_newline_on_map = 1;
   show_map_new( room );
   skip_newline_on_map = 0;
   
   mxp_tag( TAG_LOCK_SECURE );
   mxp( "</DEST>" );
   mxp_tag( TAG_DEFAULT );
}



char *get_color( char *name )
{
   int i;
   
   for ( i = 0; color_names[i].name; i++ )
     if ( !strcmp( name, color_names[i].name ) )
       return color_names[i].code;
   
   return NULL;
}


int save_settings( char *file )
{
   ROOM_DATA *r;
   ELEMENT *tag;
   FILE *fl;
   int i;
   
   fl = fopen( file, "w" );
   
   if ( !fl )
     return 1;
   
   fprintf( fl, "# File generated by IMapper.\r\n" );
   fprintf( fl, "# Manual changes that are not loaded will be lost on the next rewrite.\r\n\r\n" );
   
   for ( i = 0; color_names[i].name; i++ )
     {
        if ( !strcmp( color_names[i].code, room_color ) )
          {
             fprintf( fl, "Title-Color %s\r\n", color_names[i].name );
             break;
          }
     }
   
   fprintf( fl, "Disable-Swimming %s\r\n", disable_swimming ? "yes" : "no" );
   fprintf( fl, "Disable-WhoList %s\r\n", disable_wholist ? "yes" : "no" );
   fprintf( fl, "Disable-Alertness %s\r\n", disable_alertness ? "yes" : "no" );
   fprintf( fl, "Disable-Locating %s\r\n", disable_locating ? "yes" : "no" );
   fprintf( fl, "Disable-AreaName %s\r\n", disable_areaname ? "yes" : "no" );
   fprintf( fl, "Disable-MXPTitle %s\r\n", disable_mxp_title ? "yes" : "no" );
   fprintf( fl, "Disable-MXPExits %s\r\n", disable_mxp_exits ? "yes" : "no" );
   fprintf( fl, "Disable-MXPMap %s\r\n", disable_mxp_map ? "yes" : "no" );
   fprintf( fl, "Disable-AutoLink %s\r\n", disable_autolink ? "yes" : "no" );
   fprintf( fl, "Disable-SewerGrates %s\r\n", disable_sewer_grates ? "yes" : "no" );
   fprintf( fl, "Enable-GodRooms %s\r\n", enable_godrooms ? "yes" : "no" );
   
   /* Save all room tags. */
   
   fprintf( fl, "\r\n# Room Tags.\r\n\r\n" );
   for ( r = world; r; r = r->next_in_world )
     {
        if ( r->tags )
          {
             /* Save them in reverse. */
             
             fprintf( fl, "# %s (%s)\r\n",
                      r->name, r->area->name );
             for ( tag = r->tags; tag->next; tag = tag->next );
             for ( ; tag; tag = tag->prev )
               fprintf( fl, "Tag %d \"%s\"\r\n", r->vnum, (char *) tag->p );
          }
     }
   
   fclose( fl );
   return 0;
}


int load_settings( char *file )
{
   FILE *fl;
   char line[1024];
   char option[256];
   char value[1024];
   char *p;
   int i;
   
   fl = fopen( file, "r" );
   
   if ( !fl )
     return 1;
   
   while( 1 )
     {
        fgets( line, 1024, fl );
        
        if ( feof( fl ) )
          break;
        
        /* Strip newline. */
          {
             p = line;
             while ( *p != '\n' && *p != '\r' && *p )
               p++;
             
             *p = 0;
          }
        
        if ( line[0] == '#' || line[0] == 0 || line[0] == ' ' )
          continue;
        
        p = get_string( line, option, 256 );
        
        p = get_string( p, value, 1024 );
        
        if ( !strcmp( option, "Title-Color" ) ||
             !strcmp( option, "Title-Colour" ) )
          {
             for ( i = 0; color_names[i].name; i++ )
               {
                  if ( !strcmp( value, color_names[i].name ) )
                    {
                       room_color = color_names[i].code;
                       room_color_len = color_names[i].length;
                       break;
                    }
               }
          }
        
        else if ( !strcmp( option, "Disable-Swimming" ) )
          {
             if ( !strcmp( value, "yes" ) )
               disable_swimming = 1;
             else if ( !strcmp( value, "no" ) )
               disable_swimming = 0;
             else
               debugf( "Parse error in file '%s', expected 'yes' or 'no', got '%s' instead.", file, value );
          }
        
        else if ( !strcmp( option, "Disable-WhoList" ) )
          {
             if ( !strcmp( value, "yes" ) )
               disable_wholist = 1;
             else if ( !strcmp( value, "no" ) )
               disable_wholist = 0;
             else
               debugf( "Parse error in file '%s', expected 'yes' or 'no', got '%s' instead.", file, value );
          }
        
        else if ( !strcmp( option, "Disable-Alertness" ) )
          {
             if ( !strcmp( value, "yes" ) )
               disable_alertness = 1;
             else if ( !strcmp( value, "no" ) )
               disable_alertness = 0;
             else
               debugf( "Parse error in file '%s', expected 'yes' or 'no', got '%s' instead.", file, value );
          }
        
        else if ( !strcmp( option, "Disable-Locating" ) )
          {
             if ( !strcmp( value, "yes" ) )
               disable_locating = 1;
             else if ( !strcmp( value, "no" ) )
               disable_locating = 0;
             else
               debugf( "Parse error in file '%s', expected 'yes' or 'no', got '%s' instead.", file, value );
          }
        
        else if ( !strcmp( option, "Disable-AreaName" ) )
          {
             if ( !strcmp( value, "yes" ) )
               disable_areaname = 1;
             else if ( !strcmp( value, "no" ) )
               disable_areaname = 0;
             else
               debugf( "Parse error in file '%s', expected 'yes' or 'no', got '%s' instead.", file, value );
          }
        
        else if ( !strcmp( option, "Disable-MXPTitle" ) )
          {
             if ( !strcmp( value, "yes" ) )
               disable_mxp_title = 1;
             else if ( !strcmp( value, "no" ) )
               disable_mxp_title = 0;
             else
               debugf( "Parse error in file '%s', expected 'yes' or 'no', got '%s' instead.", file, value );
          }
        
        else if ( !strcmp( option, "Disable-MXPExits" ) )
          {
             if ( !strcmp( value, "yes" ) )
               disable_mxp_exits = 1;
             else if ( !strcmp( value, "no" ) )
               disable_mxp_exits = 0;
             else
               debugf( "Parse error in file '%s', expected 'yes' or 'no', got '%s' instead.", file, value );
          }
        
        else if ( !strcmp( option, "Disable-MXPMap" ) )
          {
             if ( !strcmp( value, "yes" ) )
               disable_mxp_map = 1;
             else if ( !strcmp( value, "no" ) )
               disable_mxp_map = 0;
             else
               debugf( "Parse error in file '%s', expected 'yes' or 'no', got '%s' instead.", file, value );
          }
        
        else if ( !strcmp( option, "Disable-AutoLink" ) )
          {
             if ( !strcmp( value, "yes" ) )
               disable_autolink = 1;
             else if ( !strcmp( value, "no" ) )
               disable_autolink = 0;
             else
               debugf( "Parse error in file '%s', expected 'yes' or 'no', got '%s' instead.", file, value );
          }
        
        else if ( !strcmp( option, "Disable-SewerGrates" ) )
          {
             if ( !strcmp( value, "yes" ) )
               disable_sewer_grates = 1;
             else if ( !strcmp( value, "no" ) )
               disable_sewer_grates = 0;
             else
               debugf( "Parse error in file '%s', expected 'yes' or 'no', got '%s' instead.", file, value );
          }
        
        else if ( !strcmp( option, "Enable-GodRooms" ) )
          {
             if ( !strcmp( value, "yes" ) )
               enable_godrooms = 1;
             else if ( !strcmp( value, "no" ) )
               enable_godrooms = 0;
             else
               debugf( "Parse error in file '%s', expected 'yes' or 'no', got '%s' instead.", file, value );
          }
        
        else if ( !strcmp( option, "Land-Mark" ) )
          {
             ROOM_DATA *room;
             ELEMENT *tag;
             int vnum = atoi( value );
             
             if ( !vnum )
               debugf( "Parse error in file '%s', expected a landmark vnum, got '%s' instead.", file, value );
             else
               {
                  room = get_room( vnum );
                  if ( !room )
                    debugf( "Warning! Unable to landmark room %d, it doesn't exist!", vnum );
                  else
                    {
                       /* Make sure it doesn't exist, first. */
                       for ( tag = room->tags; tag; tag = tag->next )
                         if ( !strcmp( "mark", (char *) tag->p ) )
                           break;
                       
                       if ( !tag )
                         {
                            /* Link it. */
                            tag = calloc( 1, sizeof( ELEMENT ) );
                            tag->p = strdup( "mark" );
                            link_element( tag, &room->tags );
                         }
                    }
               }
          }
        
        else if ( !strcmp( option, "Tag" ) )
          {
             ROOM_DATA *room;
             ELEMENT *tag;
             char buf[256];
             int vnum;
             
             vnum = atoi( value );
             get_string( p, buf, 256 );
             
             if ( !vnum || !buf[0] )
               debugf( "Parse error in file '%s', in a Tag line.", file );
             else
               {
                  room = get_room( vnum );
                  if ( !room )
                    debugf( "Warning! Unable to tag room %d, it doesn't exist!", vnum );
                  else
                    {
                       /* Make sure it doesn't exist, first. */
                       for ( tag = room->tags; tag; tag = tag->next )
                         if ( !strcmp( buf, (char *) tag->p ) )
                           break;
                       
                       if ( !tag )
                         {
                            /* Link it. */
                            tag = calloc( 1, sizeof( ELEMENT ) );
                            tag->p = strdup( buf );
                            link_element( tag, &room->tags );
                         }
                    }
               }
          }
        
        else
          debugf( "Parse error in file '%s', unknown option '%s'.", file, option );
     }
   
   fclose( fl );
   return 0;
}



void save_map( char *file )
{
   AREA_DATA *area;
   ROOM_DATA *room;
   EXIT_DATA *spexit;
   ROOM_TYPE *type;
   FILE *fl;
   char buf[256], *color;
   int i, j, count = 0;
   
   DEBUG( "save_map" );
   
   fl = fopen( file, "w" );
   
   if ( !fl )
     {
        sprintf( buf, "Can't open '%s' to save map: %s.", file,
                 strerror( errno ) );
        clientfr( buf );
        return;
     }
   
   fprintf( fl, "MAPFILE\n\n" );
   
   fprintf( fl, "\n\nROOM-TYPES\n\n"
            "# \"Name\" color [swim] [underwater] [avoid]\n" );
   
   for ( type = room_types; type; type = type->next )
     {
        /* Find the color's name. */
        
        if ( type->color )
          {
             for ( j = 0; color_names[j].name; j++ )
               if ( !strcmp( color_names[j].code, type->color ) )
                 break;
             color = color_names[j].name;
             if ( !color[0] )
               color = "unset";
          }
        else
          color = "unset";
        
        fprintf( fl, "Env: \"%s\" %s%s%s%s\n", type->name, color,
                 type->must_swim ? " swim" : "",
                 type->underwater ? " underwater" : "",
                 type->avoid ? " avoid" : "" );
     }
   
   fprintf( fl, "\n\nMESSAGES\n\n" );
   
   for ( spexit = global_special_exits; spexit; spexit = spexit->next )
     {
        fprintf( fl, "GSE: %d \"%s\" \"%s\"\n", spexit->vnum,
                 spexit->command ? spexit->command : "", spexit->message );
     }
   
   fprintf( fl, "\n\n" );
   
   for ( area = areas; area; area = area->next, count++ )
     {
        fprintf( fl, "AREA\n" );
        fprintf( fl, "Name: %s\n", area->name );
        if ( area->disabled )
          fprintf( fl, "Disabled\n" );
        fprintf( fl, "\n" );
        
        for ( room = area->rooms; room; room = room->next_in_area )
          {
             fprintf( fl, "ROOM v%d\n", room->vnum );
             fprintf( fl, "Name: %s\n", room->name );
             if ( room->room_type != null_room_type )
               fprintf( fl, "Type: %s\n", room->room_type->name );
             if ( room->underwater )
               fprintf( fl, "Underwater\n" );
             for ( i = 1; dir_name[i]; i++ )
               {
                  if ( room->exits[i] )
                    {
                       fprintf( fl, "E: %s %d\n", dir_name[i],
                                room->exits[i]->vnum );
                       if ( room->exit_length[i] )
                         fprintf( fl, "EL: %s %d\n", dir_name[i],
                                  room->exit_length[i] );
                       if ( room->exit_stops_mapping[i] )
                         fprintf( fl, "ES: %s %d\n", dir_name[i],
                                  room->exit_stops_mapping[i] );
                       if ( room->use_exit_instead[i] &&
                            room->use_exit_instead[i] != i )
                         fprintf( fl, "UE: %s %s\n", dir_name[i],
                                  dir_name[room->use_exit_instead[i]] );
                       if ( room->exit_joins_areas[i] )
                         fprintf( fl, "EJ: %s %d\n", dir_name[i],
                                  room->exit_joins_areas[i] );
                    }
                  else if ( room->detected_exits[i] )
                    {
                       if ( room->locked_exits[i] )
                         fprintf( fl, "DEL: %s\n", dir_name[i] );
                       else
                         fprintf( fl, "DE: %s\n", dir_name[i] );
                    }
               }
             
             for ( spexit = room->special_exits; spexit; spexit = spexit->next )
               {
                  fprintf( fl, "SPE: %d %d \"%s\" \"%s\"\n",
                           spexit->vnum, spexit->alias,
                           spexit->command ? spexit->command : "",
                           spexit->message ? spexit->message : "" );
               }
             
             fprintf( fl, "\n" );
          }
        fprintf( fl, "\n\n" );
     }
   
   fprintf( fl, "EOF\n" );
   
   sprintf( buf, "%d areas saved.", count );
   clientfr( buf );
   
   fclose( fl );
}



void remake_vnum_exits( )
{
   ROOM_DATA *room;
   int i;
   
   for ( room = world; room; room = room->next_in_world )
     {
        for ( i = 1; dir_name[i]; i++ )
          {
             if ( room->exits[i] )
               room->vnum_exits[i] = room->exits[i]->vnum;
          }
     }
}



void write_string( char *string, FILE *fl )
{
   int len = 0;
   
   if ( !string )
     {
        fwrite( &len, sizeof( int ), 1, fl );
        return;
     }
   
   len = strlen( string ) + 1;
   
   fwrite( &len, sizeof( int ), 1, fl );
   
   fwrite( string, sizeof( char ), len, fl );
}



char *read_string( FILE *fl )
{
   char *string;
   int len;
   
   fread( &len, sizeof( int ), 1, fl );
   
   if ( !len )
     return NULL;
   
   if ( len > 256 )
     {
        debugf( "Warning! Attempting to read an overly long string." );
        return NULL;
     }
   
   string = malloc( len );
   fread( string, sizeof( char ), len, fl );
   
   return string;
}



void save_binary_map( char *file )
{
   AREA_DATA *area;
   ROOM_DATA *room;
   EXIT_DATA *spexit;
   ROOM_TYPE *type;
   FILE *fl;
   int nr;
   
   DEBUG( "save_binary_map" );
   
   fl = fopen( file, "wb" );
   
   if ( !fl )
     return;
   
   remake_vnum_exits( );
   
   /* Room Types. */
   /* Format: int, N*ROOM_TYPE, ... */
   
   /* Count them. */
   for ( nr = 0, type = room_types; type; type = type->next )
     nr++;
   
   fwrite( &nr, sizeof( int ), 1, fl );
   for ( type = room_types; type; type = type->next )
     {
        fwrite( type, sizeof( ROOM_TYPE ), 1, fl );
        write_string( type->name, fl );
        write_string( type->color ? type->color : "unset", fl );
     }
   
   /* Global Special Exits. */
   /* Format: ..., int, N*EXIT_DATA, ... */
   
   /* Count them. */
   for ( nr = 0, spexit = global_special_exits; spexit; spexit = spexit->next )
     nr++;
   
   fwrite( &nr, sizeof( int ), 1, fl );
   for ( spexit = global_special_exits; spexit; spexit = spexit->next )
     {
        fwrite( spexit, sizeof( EXIT_DATA ), 1, fl );
        write_string( spexit->command, fl );
        write_string( spexit->message, fl );
     }
   
   /* Areas. */
   /* Format: ..., int, N*[AREA_DATA, int, M*[...]]. */
   
   /* Count them. */
   for ( nr = 0, area = areas; area; area = area->next )
     nr++;
   
   fwrite( &nr, sizeof( int ), 1, fl );
   for ( area = areas; area; area = area->next )
     {
        fwrite( area, sizeof( AREA_DATA ), 1, fl );
        write_string( area->name, fl );
        
        /* Rooms. */
        /* Format: int, M*[ROOM_DATA, int, P*EXIT_DATA] */
        
        for ( nr = 0, room = area->rooms; room; room = room->next_in_area )
          nr++;
        
        fwrite( &nr, sizeof( int ), 1, fl );
        for ( room = area->rooms; room; room = room->next_in_area )
          {
             fwrite( room, sizeof( ROOM_DATA ), 1, fl );
             write_string( room->name, fl );
             if ( room->room_type )
               write_string( room->room_type->name, fl );
             
             /* Special Exits. */
             
             /* Count them. */
             for ( nr = 0, spexit = room->special_exits; spexit; spexit = spexit->next )
               nr++;
             
             fwrite( &nr, sizeof( int ), 1, fl );
             for ( spexit = room->special_exits; spexit; spexit = spexit->next )
               {
                  fwrite( spexit, sizeof( EXIT_DATA ), 1, fl );
                  write_string( spexit->command, fl );
                  write_string( spexit->message, fl );
               }
          }
     }
   
   fwrite( "x", sizeof( char ), 1, fl );
   fclose( fl );
}



int check_map( )
{
   DEBUG( "check_map" );
   
   
   return 0;
}



void convert_vnum_exits( )
{
   ROOM_DATA *room, *r;
   EXIT_DATA *spexit;
   int i;
   
   DEBUG( "convert_vnum_exits" );
   
   for ( room = world; room; room = room->next_in_world )
     {
        /* Normal exits. */
        for ( i = 1; dir_name[i]; i++ )
          {
             if ( room->vnum_exits[i] )
               {
                  r = get_room( room->vnum_exits[i] );
                  
                  if ( !r )
                    {
                       debugf( "Can't link room %d (%s) to %d.",
                               room->vnum, room->name, room->vnum_exits[i] );
                       continue;
                    }
                  link_rooms( room, i, r );
                  room->vnum_exits[i] = 0;
               }
          }
        
        /* Special exits. */
        for ( spexit = room->special_exits; spexit; spexit = spexit->next )
          {
             if ( spexit->vnum < 0 )
               spexit->to = NULL;
             else
               {
                  if ( !( spexit->to = get_room( spexit->vnum ) ) )
                    debugf( "Can't link room %d (%s) to %d. (special exit)",
                            room->vnum, room->name, spexit->vnum );
                  else
                    link_special_exit( room, spexit, spexit->to );
               }
          }
     }
   
   for ( spexit = global_special_exits; spexit; spexit = spexit->next )
     {
        if ( spexit->vnum < 0 )
          spexit->to = NULL;
        else
          {
             if ( !( spexit->to = get_room( spexit->vnum ) ) )
               {
                  debugf( "Can't link global special exit '%s' to %d.",
                          spexit->command, spexit->vnum );
               }
          }
     }
}



int load_binary_map( char *file )
{
   AREA_DATA area, *a;
   ROOM_DATA room, *r;
   EXIT_DATA spexit, *spe;
   ROOM_TYPE type, *t;
   FILE *fl;
   char check, *type_name;
   int types, areas, rooms, exits;
   int i, j, k;
   
   destroy_map( );
   
   DEBUG( "load_binary_map" );
   
   fl = fopen( file, "rb" );
   
   if ( !fl )
     return 1;
   
   /* Room Types. */
   fread( &types, sizeof( int ), 1, fl );
   for ( i = 0; i < types; i++ )
     {
        fread( &type, sizeof( ROOM_TYPE ), 1, fl );
        
        type.name = read_string( fl );
        type.color = read_string( fl );
        if ( !type.name || !type.color )
          {
             debugf( "NULL entries where they shouldn't be!" );
             return 1;
          }
        
        // change me // Really change me
        add_room_type( type.name, type.color /*, type.must_swim, type.underwater,
                       0 ); //avoid*/ );
        if ( type.name )
          free( type.name );
        /* Don't free color. */
     }
   
   /* Global Special Exits. */
   fread( &exits, sizeof( int ), 1, fl );
   for ( i = 0; i < exits; i++ )
     {
        fread( &spexit, sizeof( EXIT_DATA ), 1, fl );
        spe = create_exit( NULL );
        
        spe->alias = spexit.alias;
        spe->vnum = spexit.vnum;
        spe->command = read_string( fl );
        spe->message = read_string( fl );
     }
   
   /* Areas. */
   fread( &areas, sizeof( int ), 1, fl );
   for ( i = 0; i < areas; i++ )
     {
        fread( &area, sizeof( AREA_DATA ), 1, fl );
        a = create_area( );
        
        a->disabled = area.disabled;
        a->name = read_string( fl );
        
        current_area = a;
        
        /* Rooms. */
        fread( &rooms, sizeof( int ), 1, fl );
        for ( j = 0; j < rooms; j++ )
          {
             fread( &room, sizeof( ROOM_DATA ), 1, fl );
             r = create_room( room.vnum );
             if ( r->vnum > last_vnum )
               last_vnum = r->vnum;
             
             r->underwater = room.underwater;
             memcpy( r->vnum_exits, room.vnum_exits, sizeof(int)*13 + sizeof(short)*13*6 );
             r->name = read_string( fl );
             
             if ( room.room_type )
               {
                  type_name = read_string( fl );
                  for ( t = room_types; t; t = t->next )
                    if ( !strcmp( type_name, t->name ) )
                      {
                         r->room_type = t;
                         break;
                      }
                  free( type_name );
               }
             
             /* Special Exits. */
             fread( &exits, sizeof( int ), 1, fl );
             for ( k = 0; k < exits; k++ )
               {
                  fread( &spexit, sizeof( EXIT_DATA ), 1, fl );
                  spe = create_exit( r );
                  
                  spe->alias = spexit.alias;
                  spe->vnum = spexit.vnum;
                  spe->command = read_string( fl );
                  spe->message = read_string( fl );
               }
          }
     }
   
   fread( &check, sizeof( char ), 1, fl );
   
   fclose( fl );
   
   if ( check != 'x' )
     {
        debugf( "The binary IMap file is corrupted!" );
        return 1;
     }
   
   convert_vnum_exits( );
   
   return check_map( );
}



int load_map( const char *file )
{
   AREA_DATA *area = NULL;
   ROOM_DATA *room = NULL;
   ROOM_TYPE *type;
   FILE *fl;
   char buf[256];
   char line[256];
   char *eof;
   int section = 0;
   int nr = 0, i;
   
   /* Loading a map over another is a bad thing. */
   destroy_map( );
   
   DEBUG( "load_map" );
   
   fl = fopen( file, "r" );
   
   if ( !fl )
     {
        sprintf( buf, "Can't open '%s' to load map: %s.", file,
                 strerror( errno ) );
        clientfr( buf );
        return 1;
     }
   
   while( 1 )
     {
        eof = fgets( line, 256, fl );
        if ( feof( fl ) )
          break;
        
        if ( !strncmp( line, "EOF", 3 ) )
          break;
        
        nr++;
        
        if ( !line[0] )
          continue;
        
        if ( !strncmp( line, "AREA", 4 ) )
          {
             area = create_area( );
             current_area = area;
             section = 1;
          }
        
        else if ( !strncmp( line, "ROOM v", 6 ) )
          {
             int vnum;
             
             if ( !area )
               {
                  debugf( "Room out of area. Line %d.", nr );
                  return 1;
               }
             
             vnum = atoi( line + 6 );
             room = create_room( vnum );
             if ( vnum > last_vnum )
               last_vnum = vnum;
             section = 2;
          }
        
        else if ( !strncmp( line, "ROOM-TYPES", 10 ) )
          {
             section = 3;
          }
        
        else if ( !strncmp( line, "MESSAGES", 8 ) )
          {
             section = 4;
          }
        
        /* Strip newline. */
          {
             char *p = line;
             while ( *p != '\n' && *p != '\r' && *p )
               p++;
             
             *p = 0;
          }
        
        if ( !strncmp( line, "Name: ", 6 ) )
          {
             if ( section == 1 )
               {
                  if ( area->name )
                    {
                       debugf( "Double name, on area. Line %d.", nr );
                       return 1;
                    }
                  
                  area->name = strdup( line + 6 );
               }
             else if ( section == 2 )
               {
                  if ( room->name )
                    {
                       debugf( "Double name, on room. Line %d.", nr );
                       return 1;
                    }
                  
                  room->name = strdup( line + 6 );
               }
          }
        
        else if ( !strncmp( line, "Type: ", 6 ) )
          {
             /* Type: Road */
             for ( type = room_types; type; type = type->next )
               if ( !strncmp( line+6, type->name, strlen( type->name ) ) )
                 {
                    room->room_type = type;
                    break;
                 }
          }
        
        else if ( !strncmp( line, "E: ", 3 ) )
          {
             /* E: northeast 14 */
             char buf[256];
             int dir;
             
             if ( section != 2 )
               {
                  debugf( "Misplaced exit line. Line %d.", nr );
                  return 1;
               }
             
             sscanf( line, "E: %s %d", buf, &dir );
             
             if ( !dir )
               {
                  debugf( "Syntax error at exit, line %d.", nr );
                  return 1;
               }
             
             for ( i = 1; dir_name[i]; i++ )
               {
                  if ( !strcmp( buf, dir_name[i] ) )
                    {
                       room->vnum_exits[i] = dir;
                       break;
                    }
               }
          }
        
        else if ( !strncmp( line, "DE: ", 4 ) )
          {
             /* DE: northeast */
             char buf[256];
             
             sscanf( line, "DE: %s", buf );
             
             for ( i = 1; dir_name[i]; i++ )
               {
                  if ( !strcmp( buf, dir_name[i] ) )
                    {
                       room->detected_exits[i] = 1;
                       break;
                    }
               }
          }
        
        else if ( !strncmp( line, "DEL: ", 4 ) )
          {
             /* DEL: northeast */
             char buf[256];
             
             sscanf( line, "DEL: %s", buf );
             
             for ( i = 1; dir_name[i]; i++ )
               {
                  if ( !strcmp( buf, dir_name[i] ) )
                    {
                       room->detected_exits[i] = 1;
                       room->locked_exits[i] = 1;
                       break;
                    }
               }
          }
        
        else if ( !strncmp( line, "EL: ", 4 ) )
          {
             /* EL: northeast 1 */
             char buf[256];
             int l;
             
             sscanf( line, "EL: %s %d", buf, &l );
             
             if ( l < 1 )
               {
                  debugf( "Syntax error at exit length, line %d.", nr );
                  return 1;
               }
             
             for ( i = 1; dir_name[i]; i++ )
               {
                  if ( !strcmp( buf, dir_name[i] ) )
                    {
                       room->exit_length[i] = l;
                       break;
                    }
               }
          }
        
        else if ( !strncmp( line, "ES: ", 4 ) )
          {
             /* ES: northeast 1 */
             char buf[256];
             int s;
             
             sscanf( line, "ES: %s %d", buf, &s );
             
             for ( i = 1; dir_name[i]; i++ )
               {
                  if ( !strcmp( buf, dir_name[i] ) )
                    {
                       room->exit_stops_mapping[i] = s;
                       break;
                    }
               }
          }
        
        else if ( !strncmp( line, "EJ: ", 4 ) )
          {
             /* EJ: northeast 1 */
             char buf[256];
             int s;
             
             sscanf( line, "EJ: %s %d", buf, &s );
             
             for ( i = 1; dir_name[i]; i++ )
               {
                  if ( !strcmp( buf, dir_name[i] ) )
                    {
                       room->exit_joins_areas[i] = s;
                       break;
                    }
               }
          }
        
        else if ( !strncmp( line, "UE: ", 4 ) )
          {
             /* UE in east */
             char buf1[256], buf2[256];
             int j;
             
             sscanf( line, "UE: %s %s", buf1, buf2 );
             
             for ( i = 1; dir_name[i]; i++ )
               {
                  if ( !strcmp( dir_name[i], buf1 ) )
                    {
                       for ( j = 1; dir_name[j]; j++ )
                         {
                            if ( !strcmp( dir_name[j], buf2 ) )
                              {
                                 room->use_exit_instead[i] = j;
                                 break;
                              }
                         }
                       break;
                    }
               }
          }
        
        else if ( !strncmp( line, "SPE: ", 5 ) )
          {
             EXIT_DATA *spexit;
             
             char vnum[1024];
             char alias[1024];
             char cmd[1024];
             char msg[1024];
             char *p = line + 5;
             
             /* SPE: -1 0 "pull lever" "You pull a lever, and fall below the ground." */
             
             p = get_string( p, vnum, 1024 );
             p = get_string( p, alias, 1024 );
             p = get_string( p, cmd, 1024 );
             p = get_string( p, msg, 1024 );
             
             spexit = create_exit( room );
             
             if ( vnum[0] == '-' )
               spexit->vnum = -1;
             else
               spexit->vnum = atoi( vnum );
             
             if ( alias[0] )
               spexit->alias = atoi( alias );
             else
               spexit->alias = 0;
             
             if ( cmd[0] )
               spexit->command = strdup( cmd );
             else
               spexit->command = NULL;
             
             if ( msg[0] )
               spexit->message = strdup( msg );
             else
               spexit->message = NULL;
          }
        
        else if ( !strncmp( line, "Disabled", 8 ) )
          {
             if ( section != 1 )
               {
                  debugf( "Wrong section of a Disabled statement." );
                  return 1;
               }
             
             area->disabled = 1;
          }
        
        else if ( !strncmp( line, "Underwater", 10 ) )
          {
             if ( section != 2 )
               {
                  debugf( "Wrong section of an Underwater statement." );
                  return 1;
               }
             
             room->underwater = 1;
          }
        
        /* Deprecated. Here only for backwards compatibility. */
        else if ( !strncmp( line, "Marked", 6 ) )
          {
             ELEMENT *tag;
             
             if ( section != 2 )
               {
                  debugf( "Wrong section of a Marked statement." );
                  return 1;
               }
             
             /* Make sure it doesn't exist, first. */
             for ( tag = room->tags; tag; tag = tag->next )
               if ( !strcmp( "tag", (char *) tag->p ) )
                 break;
             
             if ( !tag )
               {
                  /* Link it. */
                  tag = calloc( 1, sizeof( ELEMENT ) );
                  tag->p = strdup( buf );
                  link_element( tag, &room->tags );
               }
          }
        
        /* Old style - here only for backwards compatibility. */
        else if ( !strncmp( line, "T: ", 3 ) )
          {
             ROOM_TYPE *type;
             char name[512];
             char color_name[512];
             char *color;
             char buf[512];
             char *p = line + 3;
             int cost_in, cost_out;
             int must_swim;
             int underwater;
             int avoid;
             
             if ( section != 3 )
               {
                  debugf( "Misplaced room type. Line %d.", nr );
                  return 1;
               }
             
             p = get_string( p, name, 512 );
             p = get_string( p, color_name, 512 );
             p = get_string( p, buf, 512 );
             cost_in = atoi( buf );
             p = get_string( p, buf, 512 );
             cost_out = atoi( buf );
             p = get_string( p, buf, 512 );
             must_swim = buf[0] == 'y' ? 1 : 0;
             p = get_string( p, buf, 512 );
             underwater = buf[0] == 'y' ? 1 : 0;
             
             if ( !name[0] || !color_name[0] || !cost_in || !cost_out )
               {
                  debugf( "Buggy Room-Type, line %d.", nr );
                  return 1;
               }
             
             color = get_color( color_name );
             
             if ( !color )
               {
                  debugf( "Unknown color name, line %d.", nr );
                  return 1;
               }
             
             avoid = ( cost_in == 5 ) ? 1 : 0;
             
             type = add_room_type( name, color );
             type->must_swim = must_swim;
             type->underwater = underwater;
             type->avoid = avoid;
          }
        
        else if ( !strncmp( line, "Env: ", 5 ) )
          {
             ROOM_TYPE *type;
             char name[512];
             char color_name[512];
             char *color;
             char buf[512];
             char *p = line + 5;
             
             if ( section != 3 )
               {
                  debugf( "Misplaced room type. Line %d.", nr );
                  return 1;
               }
             
             p = get_string( p, name, 512 );
             p = get_string( p, color_name, 512 );
             
             if ( !name[0] || !color_name[0] )
               {
                  debugf( "Buggy Room-Type, line %d.", nr );
                  return 1;
               }
             
             color = get_color( color_name );
             if ( !color && strcmp( color_name, "unset" ) )
               {
                  debugf( "Unknown color name, line %d.", nr );
                  return 1;
               }
             
             type = add_room_type( name, color );
             
             p = get_string( p, buf, 512 );
             while ( buf[0] )
               {
                  if ( !strcmp( buf, "swim" ) )
                    type->must_swim = 1;
                  if ( !strcmp( buf, "underwater" ) )
                    type->underwater = 1;
                  if ( !strcmp( buf, "avoid" ) )
                    type->avoid = 1;
                  
                  p = get_string( p, buf, 512 );
               }
          }
        
        else if ( !strncmp( line, "GSE: ", 5 ) )
          {
             EXIT_DATA *spexit;
             
             char vnum[1024];
             char cmd[1024];
             char msg[1024];
             char *p = line + 5;
             
             if ( section != 4 )
               {
                  debugf( "Misplaced global special exit. Line %d.", nr );
                  return 1;
               }
             
             /* GSE: -1 "brazier" "You feel a moment of disorientation as you are summoned." */
             
             p = get_string( p, vnum, 1024 );
             p = get_string( p, cmd, 1024 );
             p = get_string( p, msg, 1024 );
             
             spexit = create_exit( NULL );
             
             if ( vnum[0] == '-' )
               spexit->vnum = -1;
             else
               spexit->vnum = atoi( vnum );
             
             if ( cmd[0] )
               spexit->command = strdup( cmd );
             else
               spexit->command = NULL;
             
             if ( msg[0] )
               spexit->message = strdup( msg );
             else
               spexit->message = NULL;
          }
     }
   
   fclose( fl );
   
   convert_vnum_exits( );
   
   return check_map( );
}


int get_cost( ROOM_DATA *src, ROOM_DATA *dest )
{
   return src->room_type->must_swim * 2 + src->room_type->avoid * 3 +
         dest->room_type->must_swim * 2 + dest->room_type->avoid * 3 + 1;
}



#if 0

// This was the old version of the path-finder. It was O(n^2), so now its gone.

void init_openlist( ROOM_DATA *room )
{
   ROOM_DATA *r;
   
   if ( !room )
     {
        pf_current_openlist = NULL;
        return;
     }
   
   /* Make sure it's not already there. */
   for ( r = pf_current_openlist; r; r = r->next_in_pfco )
     if ( r == room )
       return;
   
   /* Add it. */
   if ( pf_current_openlist )
     {
        room->next_in_pfco = pf_current_openlist;
        pf_current_openlist = room;
     }
   else
     {
        pf_current_openlist = room;
        room->next_in_pfco = NULL;
     }
}



void add_openlist( ROOM_DATA *src, ROOM_DATA *dest, int dir )
{
   ROOM_DATA *troom;
   
//   DEBUG( "add_openlist" );
   
   /* To add, or not to add... now that is the question. */
   if ( dest->pf_cost == 0 ||
        src->pf_cost + get_cost( dest, src ) < dest->pf_cost )
     {
        int found = 0;
        
        /* Link to the new openlist. */
        /* But first make sure it's not there already. */
        for ( troom = pf_new_openlist; troom; troom = troom->next_in_pfno )
          if ( troom == dest )
            {
               found = 1;
               break;
            }
        
        if ( !found )
          {
             dest->next_in_pfno = pf_new_openlist;
             pf_new_openlist = dest;
          }
        
        dest->pf_cost = src->pf_cost + get_cost( dest, src );
        dest->pf_parent = src;
        dest->pf_direction = dir;
     }
}




void path_finder( )
{
   ROOM_DATA *room, *r;
   EXIT_DATA *spexit;
   int i;
   
   DEBUG( "path_finder" );
   
   if ( !world )
     return;
   
   /* Okay, get ready by clearing it up. */
   for ( room = world; room; room = room->next_in_world )
     {
        room->pf_parent = NULL;
        room->pf_cost = 0;
        room->pf_direction = 0;
     }
   for ( room = pf_current_openlist; room; room = room->next_in_pfco )
     room->pf_cost = 1;
   
   pf_new_openlist = NULL;
   
   /* Were we sent NULL? Then it was just for the above, to clear it up. */
   
   if ( !pf_current_openlist )
     return;
   
   while ( pf_current_openlist )
     {
        /*** This was check_openlist( ) ***/
        
        room = pf_current_openlist;
        
        /* Walk around and create a new openlist. */
        while( room )
          {
             for ( i = 1; dir_name[i]; i++ )
               {
                  /* Normal exits. */
                   if ( room->reverse_exits[i] &&
                        room->reverse_exits[i]->exits[i] == room )
                    {
                       if ( !room->more_reverse_exits[i] )
                         add_openlist( room, room->reverse_exits[i], i );
                       else
                         {
                            /* More rooms lead here. Search for them. */
                            for ( r = world; r; r = r->next_in_world )
                              if ( r->exits[i] == room )
                                add_openlist( room, r, i );
                         }
                    }
               }
             
             /* Special exits. */
             if ( room->pointed_by )
               {
                  for ( r = world; r; r = r->next_in_world )
                    {
                       int found = 0;
                       
                       for ( spexit = r->special_exits; spexit; spexit = spexit->next )
                         {
                            if ( spexit->to == room &&
                                 ( !disable_sewer_grates || !spexit->command || strcmp( spexit->command, "enter grate" ) ) )
                              {
                                 found = 1;
                                 break;
                              }
                         }
                       
                       if ( found )
                         add_openlist( room, r, -1 );
                    }
               }
             
             room = room->next_in_pfco;
          }
        
        /* Replace the current openlist with the new openlist. */
        pf_current_openlist = pf_new_openlist;
        for ( room = pf_new_openlist; room; room = room->next_in_pfno )
          room->next_in_pfco = room->next_in_pfno;
        pf_new_openlist = NULL;
     }
}

#endif


void show_path( ROOM_DATA *current )
{
   ROOM_DATA *room;
   EXIT_DATA *spexit;
   char buf[4096];
   int nr = 0;
   int wrap = 7; /* strlen( "[Path: " ) */
   
   DEBUG( "show_path" );
   
   if ( !current->pf_parent )
     {
        clientfr( "Can't find a path from here." );
        return;
     }
   
   sprintf( buf, C_R "[Path: " C_G );
   for ( room = current; room && room->pf_parent && nr < 100; room = room->pf_parent )
     {
        if ( wrap > 70 )
          {
             strcat( buf, C_R ",\r\n" C_G );
             wrap = 0;
          }
        else if ( nr++ )
          {
             strcat( buf, C_R ", " C_G );
             wrap += 2;
          }
        
        /* We also have special exits. */
        if ( room->pf_direction != -1 )
          {
             if ( must_swim( room, room->pf_parent ) )
               strcat( buf, C_B );
             
             strcat( buf, dir_small_name[room->pf_direction] );
             wrap += strlen( dir_small_name[room->pf_direction] );
          }
        else
          {
             /* Find the special exit, leading to the parent. */
             for ( spexit = room->special_exits; spexit; spexit = spexit->next )
               {
                  if ( spexit->to == room->pf_parent )
                    {
                       if ( spexit->command )
                         {
                            strcat( buf, spexit->command );
                            wrap += strlen( spexit->command );
                         }
                       else
                         {
                            strcat( buf, "?" );
                            wrap++;
                         }
                       
                       break;
                    }
               }
          }
     }
   
   if ( room && room->pf_parent && nr == 100 )
     {
        strcat( buf, C_R ", " C_G "..." );
     }
   
   strcat( buf, C_R ".]\r\n" C_0 );
   clientf( buf );
}



void clean_paths( )
{
   ROOM_DATA *room;
   
   /* Clean up the world. */
   for ( room = world; room; room = room->next_in_world )
     room->pf_parent = NULL, room->pf_cost = 0, room->pf_direction = 0;
   
   for ( pf_step = 0; pf_step < MAX_COST; pf_step++ )
     pf_queue[pf_step] = NULL;
   
   pf_step = pf_rooms = 0;
}



int add_to_pathfinder( ROOM_DATA *room, int cost )
{
   ROOM_DATA **queue;
   
   if ( room->pf_cost && room->pf_cost <= pf_step + cost )
     return 0;
   
   /* Unlink it if it's already somewhere. */
   if ( room->pf_cost )
     {
        if ( room->pf_next )
          room->pf_next->pf_prev = room->pf_prev;
        *(room->pf_prev) = room->pf_next, pf_rooms--;
     }
   
   
   if ( cost < 1 || cost > MAX_COST - 1)
     debugf( "FREAKING OUT!" );
   
   room->pf_cost = pf_step + cost;
   
   /* Now add it to the queue. Wacky, eh? */
   queue = &pf_queue[(pf_step + cost) % MAX_COST];
   
   room->pf_next = *queue;
   room->pf_prev = queue;
   *queue = room;
   if ( room->pf_next )
     room->pf_next->pf_prev = &room->pf_next;
   
   pf_rooms++;
   
   return 1;
}



void path_finder_2( )
{
   ROOM_DATA *room;
   REVERSE_EXIT *rexit;
   
   DEBUG( "path_finder_2" );
   
   while ( pf_rooms )
     {
        for ( room = pf_queue[pf_step % MAX_COST]; room; room = room->pf_next )
          {
             for ( rexit = room->rev_exits; rexit; rexit = rexit->next )
               {
                  if ( add_to_pathfinder( rexit->from, get_cost( rexit->from, room ) ) )
                    {
                       rexit->from->pf_direction = rexit->direction;
                       if ( !rexit->direction )
                         rexit->from->pf_direction = -1;
                       rexit->from->pf_parent = room;
                    }
               }
             
             pf_rooms--;
          }
        
        pf_queue[pf_step % MAX_COST] = NULL;
        pf_step++;
     }
}



void i_mapper_mxp_enabled( )
{
   floating_map_enabled = 0;
   
   mxp_tag( TAG_LOCK_SECURE );
   mxp( "<!element mpelm '<send \"map path &v;|room look &v;\" "
        "hint=\"&r;|Vnum: &v;|Type: &t;\">' ATT='v r t'>" );
   mxp( "<!element mppers '<send \"map path &v;|room look &v;|who &p;\" "
        "hint=\"&p; (&r;)|Vnum: &v;|Type: &t;|Player: &p;\">' ATT='v r t p'>" );
//   mxp( "<!element RmTitle '<font color=red>' TAG='RoomName'>" );
//   mxp( "<!element RmExits TAG='RoomExit'>" );
   mxp_tag( TAG_DEFAULT );
}



void mapper_start( )
{
   null_room_type = add_room_type( "Unknown", get_color( "red" ) );
   add_room_type( "Undefined", get_color( "bright-red" ) );
}



void i_mapper_module_init_data( )
{
   struct stat map, mapbin, mapper;
   int binary;
   
   DEBUG( "i_mapper_init_data" );
   
   mapper_start( );
   
   /* Check if we have a binary map that is newer than the map and mapper. */
   
   if ( !stat( map_file_bin, &mapbin ) &&
        !stat( map_file, &map ) &&
        ( !stat( "i_mapper.dll", &mapper ) ||
          !stat( "i_mapper.so", &mapper ) ) &&
        mapbin.st_mtime > map.st_mtime &&
        mapbin.st_mtime > mapper.st_mtime )
     binary = 1;
   else
     binary = 0;
   
   get_timer( );
   
   if ( binary ? load_binary_map( map_file_bin ) : load_map( map_file ) )
     {
        destroy_map( );
        mode = NONE;
        return;
     }
   
   debugf( "%sIMap loaded. (%d microseconds)",
           binary ? "Binary " : "", get_timer( ) );
   
   /* Only if one already exists, but is too old. */
   if ( !binary && !stat( map_file_bin, &mapbin ) )
     {
        debugf( "Generating binary map." );
        save_binary_map( map_file_bin );
     }
   
   load_settings( "config.mapper.txt" );
   
   mode = GET_UNLOST;
}



/* Case insensitive versions of strstr and strcmp. */

#define LOW_CASE( a ) ( (a) >= 'A' && (a) <= 'Z' ? (a) - 'A' + 'a' : (a) )

int case_strstr( char *haystack, char *needle )
{
   char *h = haystack, *n = needle;
   
   while ( *h )
     {
        if ( LOW_CASE( *h ) == LOW_CASE( *(needle) ) )
          {
             n = needle;
             
             while( 1 )
               {
                  if ( !(*n) )
                    return 1;
                  
                  if ( LOW_CASE( *h ) != LOW_CASE( *n ) )
                    break;
                  h++, n++;
               }
          }
        
        h++;
     }
   
   return 0;
}


/* I don't trust strcasecmp to be too portable. */
int case_strcmp( const char *s1, const char *s2 )
{
   while ( *s1 && *s2 )
     {
        if ( LOW_CASE( *s1 ) != LOW_CASE( *s2 ) )
          break;
        
        s1++;
        s2++;
     }
   
   return ( *s1 || *s2 ) ? 1 : 0;
}


void remove_players( TIMER *self )
{
   ROOM_DATA *r;
   
   for ( r = world; r; r = r->next_in_world )
     {
        if ( r->person_here )
          {
             free( r->person_here );
             r->person_here = NULL;
          }
     }
}



void mark_player( ROOM_DATA *room, char *name )
{
   ROOM_DATA *r;
   
   if ( room->person_here )
     {
        free( room->person_here );
        room->person_here = NULL;
     }
   
   if ( name )
     {
        /* Make sure it's nowhere else. */
        for ( r = world; r; r = r->next_in_world )
          if ( r->person_here && !strcmp( r->person_here, name ) )
            {
               free( r->person_here );
               r->person_here = NULL;
            }
        
        room->person_here = strdup( name );
     }
   else
     room->person_here = strdup( "Someone" );
   
   add_timer( "remove_players", 5, remove_players, 0, 0, 0 );
   
   /* Force an update of the floating map. */
   last_room = NULL;
}



void locate_room( char *name, int area, char *player )
{
   ROOM_DATA *room, *found = NULL;
   char buf[256];
   int more = 0;
   
   DEBUG( "locate_room" );
   
   strcpy( buf, name );
   
   for ( room = world; room; room = room->next_in_world )
     {
        if ( !strcmp( buf, room->name ) )
          {
             if ( !found )
               found = room;
             else
               {
                  more = 1;
                  break;
               }
          }
     }
   
   if ( found )
     {
        /* Attempt to find out in which side of the arena it is. */
        if ( locate_arena && !more && found->area )
          {
             if ( case_strstr( found->area->name, "north" ) )
               {
                  if ( case_strstr( found->area->name, "east" ) )
                    clientf( C_D " (" C_R "NE" C_D ")" C_0 );
                  else if ( case_strstr( found->area->name, "west" ) )
                    clientf( C_D " (" C_R "NW" C_D ")" C_0 );
                  else
                    clientf( C_D " (" C_R "N" C_D ")" C_0 );
               }
             else if ( case_strstr( found->area->name, "south" ) )
               {
                  if ( case_strstr( found->area->name, "east" ) )
                    clientf( C_D " (" C_R "SE" C_D ")" C_0 );
                  else if ( case_strstr( found->area->name, "west" ) )
                    clientf( C_D " (" C_R "SW" C_D ")" C_0 );
                  else
                    clientf( C_D " (" C_R "S" C_D ")" C_0 );
               }
             else if ( case_strstr( found->area->name, "central" ) ||
                       case_strstr( found->area->name, "palm orchard" ) )
               clientf( C_D " (" C_R "C" C_D ")" C_0 );
             else if ( case_strstr( found->area->name, "east" ) )
               clientf( C_D " (" C_R "E" C_D ")" C_0 );
             else if ( case_strstr( found->area->name, "west" ) )
               clientf( C_D " (" C_R "W" C_D ")" C_0 );
             
             locate_arena = 0;
          }
        
        /* Show the vnum, after the line. */
        clientff( " " C_D "(" C_G "%d" C_D "%s)" C_0,
                 found->vnum, more ? C_D "," C_G "..." C_D : "" );
        
        /* A farsight-like thing. */
        if ( area && found->area )
          {
             clientff( "\r\nFrom your knowledge, that room is in '%s'",
                       found->area->name );
          }
        
        /* Mark the room on the map, for a few moments. */
        if ( !more )
          {
             mark_player( found, player );
          }
     }
}



void parse_msense( char *line )
{
   char *end;
   char buf[256];
   
   DEBUG( "parse_msense" );
   
   if ( strncmp( line, "An image of ", 12 ) )
     return;
   
   line += 12;
   
   if ( !( end = strstr( line, " appears in your mind." ) ) )
     return;
   
   if ( end < line )
     return;
   
   strncpy( buf, line, end - line );
   buf[end-line] = '.';
   buf[end-line+1] = 0;
   
   /* We now have the room name in 'buf'. */
   locate_room( buf, 1, NULL );
}



void parse_window( char *line )
{
   char name[256];
   
   DEBUG( "parse_window" );
   
   if ( strncmp( line, "You see that ", 12 ) )
     return;
   
   get_string( line + 12, name, 256 );
   
   line = strstr( line, " is at " );
   
   if ( !line )
     return;
   
   line += 7;
   
   locate_room( line, 1, name );
}



void parse_scent( char *line )
{
   if ( !strncmp( line, "You detect traces of scent from ", 32 ) )
     {
        locate_room( line + 32, 1, NULL );
     }
}



void parse_scry( char *line )
{
   char buf[256];
   char name[256];
   
   DEBUG( "parse_scry" );
   
   if ( !sense_message )
     {
        if ( !strncmp( line, "You create a pool of water in the air in front of you, and look through it,", 75 ) )
          sense_message = 1;
     }
   
   if ( sense_message == 1 )
     {
        /* Next line: "sensing Whyte at Antioch Runners tent." */
        /* Skip the first three words. */
        line = get_string( line, buf, 256 );
        line = get_string( line, name, 256 );
        line = get_string( line, buf, 256 );
        
        locate_room( line, 1, name );
     }
}



void parse_ka( char *line )
{
   static char buf[1024];
   static int periods;
   int i;
   
   DEBUG( "parse_ka" );
   
   /* A shimmering image of *name* appears before you. She/he is at *room*.*/
   
   if ( !sense_message )
     {
        if ( !strncmp( line, "A shimmering image of ", 22 ) )
          {
             buf[0] = 0;
             sense_message = 2;
             periods = 0;
          }
     }
   
   if ( sense_message == 2 )
     {
        for ( i = 0; line[i]; i++ )
          if ( line[i] == '.' )
            periods++;
        
        strcat( buf, line );
        
        if ( periods == 2 )
          {
             sense_message = 0;
             
             if ( ( line = strstr( buf, " is at " ) ) )
               locate_room( line + 7, 1, NULL );
             else if ( ( line = strstr( buf, " is located at " ) ) )
               locate_room( line + 15, 1, NULL );
          }
     }
}



void parse_seek( char *line )
{
   static char buf[1024];
   static int periods;
   char *end;
   int i;
   
   DEBUG( "parse_seek" );
   
   /* A shimmering image of *name* appears before you. She/he is at *room*.*/
   
   if ( !sense_message )
     {
        if ( !strncmp( line, "You bid your guardian to seek out the lifeforce of ", 51 ) )
          {
             buf[0] = 0;
             sense_message = 3;
             periods = 0;
          }
     }
   
   if ( sense_message == 3 )
     {
        for ( i = 0; line[i]; i++ )
          if ( line[i] == '.' )
            periods++;
        
        strcat( buf, line );
        
        if ( periods == 3 )
          {
             sense_message = 0;
             
             if ( ( line = strstr( buf, " at " ) ) )
               {
                  line += 4;
                  
                  end = strchr( line, ',' );
                  if ( !end )
                    return;
                  
                  strncpy( buf, line, end - line );
                  buf[end-line] = '.';
                  buf[end-line+1] = 0;
                  
                  locate_room( buf, 1, NULL );
               }
          }
     }
}



void parse_shrinesight( char *line )
{
   char *p, *end;
   char buf[256];
   
   DEBUG( "parse_shrinesight" );
   
   if ( !strstr( line, " shrine at '" ) )
     return;
   
   p = strstr( line, "at '" );
   if ( !p )
     return;
   p += 4;
   
   end = strstr( p, "'" );
   if ( !end )
     return;
   
   if ( p >= end )
     return;
   
   strncpy( buf, p, end - p );
   buf[end-p] = 0;
   
   /* We now have the room name in 'buf'. */
   locate_room( buf, 0, NULL );
}



void parse_fullsense( char *line )
{
   char *p;
   char buf[256];
   char name[256];
   char room[256];
   
   DEBUG( "parse_fullsense" );
   
   if ( strncmp( line, "You sense ", 10 ) || strlen( line ) > 128 )
     return;
   
   strcpy( buf, line );
   
   p = strstr( buf, " at " );
   if ( !p )
     return;
   *p = 0;
   p += 4;
   
   strcpy( name, buf + 10 );
   strcpy( room, p );
   
   /* We now have the room name in 'buf'. */
   locate_room( room, 0, name );
}



void parse_scout( char *line )
{
   char buf[512], *p;
   
   DEBUG( "parse_scout" );
   
   if ( !strcmp( line, "You sense the following people in the area:" ) ||
        !strcmp( line, "You sense the following creatures in the area:" ) )
     {
        scout_list = 1;
        return;
     }
   
   if ( !scout_list )
     return;
   
   p = strstr( line, " at " );
   if ( !p )
     return;
   
   strcpy( buf, p + 4 );
   strcat( buf, "." );
   
   locate_room( buf, 0, NULL );
}



void parse_view( char *line )
{
   char name[256], buf[256];
   
   DEBUG( "parse_view" );
   
   /* You see Name at Location. */
   
   if ( cmp( "You see *", line ) )
     return;
   
   line = get_string( line + 8, name, 256 );
   line = get_string( line, buf, 256 );
   
   if ( cmp( "at", buf ) )
     return;
   
   locate_room( line, 0, name );
}



void parse_pursue( char *line )
{
   char buf[512], buf2[512], name[512];
   char *p;
   static int pursue_message;
   
   DEBUG( "parse_pursue" );
   
   if ( !cmp( "You sense that ^ has entered *", line ) )
     {
        pursue_message = 1;
        get_string( line + 15, name, 512 );
        buf[0] = 0;
     }
   
   if ( !pursue_message )
     return;
   
   /* Add a space to the last line, in case there wasn't one. */
   if ( buf[0] && buf[strlen(buf)-1] != ' ' )
     strcat( buf, " " );
   
   strcat( buf, line );
   
   if ( strstr( buf, "." ) )
     {
        pursue_message = 0;
        
        p = strstr( buf, " has entered " );
        
        if ( !p )
          return;
        
        strcpy( buf2, p + 13 );
        
        p = buf2 + strlen( buf2 );
        
        while ( p > buf2 )
          {
             if ( *p == ',' )
               {
                  *(p++) = '.';
                  *p = 0;
                  break;
               }
             p--;
          }
        
        locate_room( buf2, 1, name );
     }
}



void parse_alertness( char *line )
{
   static char buf[512];
   static int alertness_message;
   char buf2[512];
   
   DEBUG( "parse_alertness" );
   
   if ( disable_alertness )
     return;
   
   if ( !cmp( "Your enhanced senses inform you that *", line ) )
     {
        buf[0] = 0;
        alertness_message = 1;
     }
   
   if ( !alertness_message )
     return;
   
   hide_line( );
   
   /* In case something goes wrong. */
   if ( alertness_message++ > 3 )
     {
        alertness_message = 0;
        return;
     }
   
   if ( buf[0] && buf[strlen(buf)-1] != ' ' )
     strcat( buf, " " );
   
   strcat( buf, line );
   
   if ( strstr( buf, "nearby." ) )
     {
        /* We now have the full message in 'buf'. Parse it. */
        char player[256];
        char room_name[256];
        char *p, *p2;
        int found = 0;
        int i;
        
        alertness_message = 0;
        
        /* Your enhanced senses inform you that <player> has entered <room> nearby. */
        
        p = strstr( buf, " has entered" );
        if ( !p )
          return;
        *p = 0;
        
        strcpy( player, buf + 37 );
        
        p2 = strstr( p + 1, " nearby" );
        if ( !p2 )
          return;
        *p2 = 0;
        
        strcpy( room_name, p + 13 );
        strcat( room_name, "." );
        
        alertness_message = 0;
        
        sprintf( buf2, C_R "[" C_W "%s" C_R " - ", player );
   
        /* Find where that room is. */
        
        if ( current_room )
          {     
             if ( !room_cmp( current_room->name, room_name ) )
               {
                  strcat( buf2, "here" );
                  found = 1;
               }
             
             for ( i = 1; dir_name[i]; i++ )
               {
                  if ( !current_room->exits[i] )
                    continue;
                  
                  if ( !room_cmp( current_room->exits[i]->name, room_name ) )
                    {
                       if ( found )
                         strcat( buf2, ", " );
                       
                       strcat( buf2, C_B );
                       strcat( buf2, dir_name[i] );
                       found = 1;
                    }
               }
          }
        
        if ( !found )
          {
             strcat( buf2, C_R );
             strcat( buf2, room_name );
          }
        
        strcat( buf2, C_R "]\r\n" C_0 );
        suffix( buf2 );
     }
}



void parse_eventstatus( char *line )
{
   char buf[512], name[512];
   
   DEBUG( "parse_eventstatus" );
   
   if ( !strncmp( line, "Current event: ", 15 ) )
     {
        evstatus_list = 1;
        return;
     }
   
   if ( !evstatus_list )
     return;
   
   line = get_string( line, buf, 512 );
   
   if ( buf[0] == '-' )
     return;
   
   if ( !strcmp( buf, "Player" ) )
     return;
   
   if ( buf[0] < 'A' || buf[0] > 'Z' )
     {
        evstatus_list = 0;
        return;
     }
   
   strcpy( name, buf );
   
   /* Skip all numbers and other things, until we get to the room name. */
   while ( 1 )
     {
        if ( !line[0] )
          return;
        
        if ( line[0] < 'A' || line[0] > 'Z' )
          {
             line = get_string( line, buf, 512 );
             continue;
          }
        
        break;
     }
   
   strcpy( buf, line );
   strcat( buf, "." );
   
   locate_arena = 1;
   locate_room( buf, 0, name );
}



void parse_petlist( char *line, int line_len )
{
   ROOM_DATA *room = NULL, *r;
   char *roomname, *place = NULL;
   int len, more = 0;
   /*
    * 18742  a mysterious winged, black ten* A well-swept, comfortable ke
    */
   
   if ( !cmp( "Your Pets in the Land", line ) )
     {
        pet_list = 1;
        return;
     }
   
   if ( !pet_list )
     return;
   
   /* Usually begins with the pet's number. */
   if ( line[0] < '0' || line[0] > '9' )
     return;
   
   /* And the room name starts at pos 37. */
   if ( line_len < 39 )
     return;
   roomname = line + 37;
   if ( roomname[0] == '*' )
     roomname += 2;
   len = strlen( roomname );
   
   for ( r = world; r; r = r->next_in_world )
     if ( !strncmp( roomname, r->name, len ) )
       {
          if ( !room )
            room = r;
          else
            {
               more = 1;
               break;
            }
       }
   
   if ( !room )
     return;
   
   clientff( " " C_D "(" C_G "%d" C_D "%s)" C_0,
             room->vnum, more ? C_D "," C_G "..." C_D : "" );
   
   roomname -= 2;
   
   if ( !strcmp( roomname, "* A well-swept, comfortable ke" ) ||
        !strcmp( roomname, "* Within a large, well-kept st" ) )
     place = "Ki";
   
   if ( !strcmp( roomname, "* The Scuderia Animale." ) ||
        !strcmp( roomname, "* The Scuderia Equus." ) )
     place = "S";
   
   if ( !strcmp( roomname, "* A dusty kennel." ) ||
        !strcmp( roomname, "* A clean, brightly lit stable" ) )
     place = "A";
   
   if ( !strcmp( roomname, "* A comfortable, flourishing k" ) ||
        !strcmp( roomname, "* A canopy-covered stable." ) )
     place = "Kh";
   
   if ( !strcmp( roomname, "* Open wilderness behind Phoen" ) ||
        !strcmp( roomname, "* A sheltered overhang." ) )
     place = "C";
   
   if ( !strcmp( roomname, "* An eerie kennel." ) ||
        !strcmp( roomname, "* An unnaturally quiet stable." ) )
     place = "I";
   
   if ( place )
     clientff( " " C_D "(" C_R "%s" C_D ")" C_0, place );
}



void parse_wormholes( char *line )
{
   static int unfinished_message;
   char name1[1024];
   char *name2;
   
   if ( !cmp( "You sense a connection between *", line ) )
     {
        name1[0] = 0;
        line += 31;
        unfinished_message = 1;
     }
   
   if ( !unfinished_message )
     return;
   
   strcat( name1, line );
   if ( strstr( name1, "." ) )
     unfinished_message = 0;
   else
     {
        /* Unfinished message - if it doesn't end in a space, add one. */
        if ( strlen( name1 ) && name1[strlen( name1 ) - 1] != ' ' )
          strcat( name1, " " );
        return;
     }
   
   name2 = strstr( name1, " and " );
   if ( !name2 )
     return;
   *name2 = '.';
   *(name2+1) = 0;
   
   name2 += 5;
   
   locate_room( name1, 0, NULL );
   suffix( " ->" );
   locate_room( name2, 0, NULL );
}



void parse_who( LINES *l, int line )
{
   static int first_time = 1, len1, len2;
   ROOM_DATA *r, *room;
   AREA_DATA *area;
   char name[64];
   char roomname[256];
   char buf2[1024];
   char color[64];
   int more, len, moreareas;
   
   DEBUG( "parse_who" );
   
   if ( disable_wholist ||
        l->line[line][0] != ' ' ||
        l->len[line] < 53 )
     return;
   
   /* Initialize these two, so we won't do them each time. */
   if ( first_time )
     {
        len1 = 9 + strlen( C_D C_G C_D );
        len2 = 9 + strlen( C_D C_R C_D );
        first_time = 0;
     }
   
   /* Has two lines, with color changes at the sides? */
   
   if ( !cmp( " - *", l->line[line] + 14 ) &&
        !cmp( " - *", l->line[line] + 51 ) )
     {
        if ( l->colour[l->line_start[line]] )
          strcpy( color, l->colour[l->line_start[line]] );
        else
          strcpy( color, C_0 );
        
        get_string( l->line[line], name, 64 );
        
        strcpy( roomname, l->line[line] + 54 );
     }
   else
     return;
   
   hide_line( );
   
   /* Search for it. */
   
   room = NULL;
   area = NULL;
   more = 0;
   moreareas = 0;
   len = strlen( roomname );
   
   for ( r = world; r; r = r->next_in_world )
     if ( !strncmp( roomname, r->name, len ) )
       {
          if ( !room )
            room = r, area = r->area;
          else
            {
               more++;
               if ( area != r->area )
                 moreareas++;
            }
       }
   
   clientff( "%s%14s" C_c " - " C_0 "%-24s ", color, name, roomname );
   
   if ( !room )
     {
        clientff( "%9s", "" );
     }
   else
     {
        char buf3[256];
        int len;
        
        if ( !more )
          {
             sprintf( buf3, C_D "(" C_G "%d" C_D ")", room->vnum );
             len = len1;
          }
        else
          {
             sprintf( buf3, C_D "(" C_R "%d rms" C_D ")", more + 1 );
             len = len2;
          }
        
        clientff( "%*s", len, buf3 );
     }
   
   clientf( C_c " - " C_0 );
   
   if ( area )
     {
        if ( !moreareas )
          {
             strcpy( buf2, area->name );
             buf2[24] = 0;
             clientf( buf2 );
          }
        else
          clientf( C_D "(" C_R "multiple areas matched" C_D ")" C_0 );
     }
   else
     clientf( C_D "(" C_R "unknown" C_D ")" C_0 );
   
   clientf( "\r\n" );
}



void parse_underwater( char *line )
{
   DEBUG( "parse_underwater" );
   
   /* 1 - No, 2 - Trying to get up, 3 - Up. */
   
   if ( !strcmp( line, "form an air pocket around you." ) ||
        !strcmp( line, "You are already surrounded by a pocket of air." ) ||
        !strcmp( line, "You are surrounded by a pocket of air and so must move normally through water." ) )
     pear_defence = 2;
   
   if ( !strcmp( line, "The pocket of air around you dissipates into the atmosphere." ) ||
        !strcmp( line, "The bubble of air around you dissipates." ) )
     pear_defence = 0;
}



void parse_special_exits( char *line )
{
   EXIT_DATA *spexit;
   
   DEBUG( "parse_special_exits" );
   
   if ( !current_room )
     return;
   
   if ( mode != FOLLOWING && mode != CREATING )
     return;
   
   /* Since this function is checked after parse_room, this is a good place. */
   if ( mode == CREATING && capture_special_exit )
     {
        strcpy( cse_message, line );
        return;
     }
   
   for ( spexit = current_room->special_exits; spexit; spexit = spexit->next )
     {
        if ( !spexit->message )
          continue;
        
        if ( !cmp( spexit->message, line ) )
          {
             if ( spexit->to )
               {
                  current_room = spexit->to;
                  current_area = current_room->area;
                  add_queue_top( -1 );
               }
             else
               {
                  mode = GET_UNLOST;
               }
             
             return;
          }
     }
   
   for ( spexit = global_special_exits; spexit; spexit = spexit->next )
     {
        if ( !spexit->message )
          continue;
        
        if ( !cmp( spexit->message, line ) )
          {
             if ( spexit->to )
               {
                  current_room = spexit->to;
                  current_area = current_room->area;
                  add_queue_top( -1 );
               }
             else
               {
                  mode = GET_UNLOST;
               }
             
             clientff( C_R " (" C_Y "%s" C_R ")" C_0, spexit->command ? spexit->command : "teleport" );
             return;
          }
     }
}



void parse_sprint( char *line )
{
   static int sprinting;
   char buf[256];
   
   DEBUG( "parse_squint" );
   
   if ( mode != FOLLOWING && mode != CREATING )
     return;
   
   if ( !sprinting && !strncmp( line, "You look off to the ", 20 ) &&
        strstr( line, " and dash speedily away." ) )
     {
        int i;
        
        get_string( line + 20, buf, 256 );
        
        sprinting = 0;
        
        for ( i = 1; dir_name[i]; i++ )
          {
             if ( !strcmp( buf, dir_name[i] ) )
               {
                  sprinting = i;
                  break;
               }
          }
        
        return;
     }
   
   if ( !sprinting )
     return;
   
   /* End of the sprint? */
   if ( strncmp( line, "You dash through ", 17 ) )
     {
        add_queue_top( sprinting );
        sprinting = 0;
        return;
     }
   
   if ( !current_room || !current_room->exits[sprinting] ||
        room_cmp( current_room->exits[sprinting]->name, line + 17 ) )
     {
        sprinting = 0;
        clientf( " " C_D "(" C_G "lost" C_D ")" C_0 );
        return;
     }
   
   current_room = current_room->exits[sprinting];
   clientff( " " C_D "(" C_G "%d" C_D ")" C_0, current_room->vnum );
}



void parse_follow( char *line )
{
   static char msg[256];
   static int in_message;
   char *to;
   
   if ( mode != FOLLOWING || !current_room )
     return;
   
   if ( !cmp( "You follow *", line ) )
     {
        in_message = 1;
        
        strcpy( msg, line );
     }
   else if ( in_message )
     {
        if ( strlen( msg ) > 0 && msg[strlen(msg)-1] != ' ' )
          strcat( msg, " " );
        strcat( msg, line );
     }
   else
     return;
   
   if ( in_message && !strchr( msg, '.' ) )
     return;
   
   in_message = 0;
   
   if ( !( to = strstr( msg, " to " ) ) )
     return;
   
     {
        /* We now have our message, which should look like this... */
        /* You follow <TitledName> <direction> to <roomname> */
        
        ROOM_DATA *destination;
        char room[256], dir[256];
        char *d;
        int changed_area = 0;
        int i, dir_nr = 0;
        
        /* Snatch the room name, then look backwards for the direction. */
        strcpy( room, to + 4 );
        if ( !room[0] )
          {
             debugf( "No room name found." );
             return;
          }
        
        d = to - 1;
        while ( d > msg && *d != ' ' )
          d--;
        get_string( d, dir, 256 );
        
        /* We now have the room, and the direction. */
        
        for ( i = 1; dir_name[i]; i++ )
          if ( !strcmp( dir, dir_name[i] ) )
            {
               dir_nr = i;
            }
        
        if ( !dir_nr )
          return;
        
        destination = current_room->exits[dir_nr];
        
        if ( !destination || room_cmp( destination->name, room ) )
          {
             /* Perhaps it's a special exit, so look for one. */
             EXIT_DATA *spexit;
             
             for ( spexit = current_room->special_exits; spexit; spexit = spexit->next )
               {
                  if ( !spexit->to || room_cmp( spexit->to->name, room ) )
                    continue;
                  
                  destination = spexit->to;
                  clientff( C_R " (" C_Y "sp" C_R ")" C_0 );
                  break;
               }
             
             /* Or perhaps we're just simply lost... */
             if ( !spexit )
               {
                  clientff( C_R " (" C_G "lost" C_R ")" C_0 );
                  mode = GET_UNLOST;
                  return;
               }
          }
        
        if ( current_room->area != destination->area )
          changed_area = 1;
        
        current_room = destination;
        
        clientff( C_R " (" C_g "%d" C_R ")" C_0, current_room->vnum );
        if ( changed_area )
          {
             current_area = current_room->area;
             clientff( "\r\n" C_R "[Entered the %s]" C_0,
                       current_area->name );
          }
     }
}


void parse_autobump( char *line )
{
   /* 1 - Bump all around that room. */
   /* 2 - Wait for bumps to complete. */
   /* 3 - Search a new room. */
   
   if ( !auto_bump )
     return;
   
   if ( auto_bump == 2 )
     {
        if ( !strcmp( "There is no exit in that direction.", line ) )
          bump_exits--;
        
        if ( !bump_exits )
          {
             ROOM_DATA *r;
             int i;
             
             auto_bump = 3;
             
             r = bump_room, i = 0;
             /* Count rooms left. */
             while ( r )
               {
                  r = r->next_in_area;
                  i++;
               }
             
             clientff( C_R "\r\n[Rooms left: %d.]\r\n" C_0, i - 1 );
             
             bump_room = bump_room->next_in_area;
             
             if ( !bump_room )
               {
                  auto_bump = 0;
                  clientfr( "All rooms completed." );
               }
             
             clean_paths( );
             add_to_pathfinder( bump_room, 1 );
             path_finder_2( );
             go_next( );
          }
     }
   
}


void check_autobump( )
{
   if ( !auto_bump || !bump_room )
     return;
   
   /* 1 - Bump all around that room. */
   /* 2 - Wait for bumps to complete. */
   /* 3 - Search a new room. */
   
   if ( auto_bump == 1 )
     {
        bump_exits = 0;
        int i;
        
        send_to_server( "survey\r\n" );
        clientff( "\r\n" );
        
        for ( i = 1; dir_name[i]; i++ )
          {
             if ( bump_room->exits[i] )
               continue;
             
             bump_exits++;
             send_to_server( dir_small_name[i] );
             send_to_server( "\r\n" );
          }
        
        send_to_server( "get gem\n" );
        send_to_server( "get gem\n" );
        send_to_server( "get gem\n" );
        
        auto_bump = 2;
     }
   
   if ( auto_bump == 2 )
     return;
   
   if ( auto_bump == 3 && bump_room == current_room )
     {
        /* We're here! */
        
        auto_bump = 1;
        check_autobump( );
     }
}



void process_line( char *line, int len )
{
   const char *block_messages[] =
     {    "You cannot move that fast, slow down!",
          "You cannot move until you have regained equilibrium.",
          "You are regaining balance and are unable to move.",
          "You'll have to swim to make it through the water in that direction.",
          "rolling around in the dirt a bit.",
          "You are asleep and can do nothing. WAKE will attempt to wake you.",
          "There is a door in the way.",
          "You must first raise yourself and stand up.",
          "You are surrounded by a pocket of air and so must move normally through water.",
          "You cannot fly indoors.",
          "You slip and fall on the ice as you try to leave.",
          "A rite of piety prevents you from leaving.",
          "A wall blocks your way.",
          "The hands of the grave grasp at your ankles and throw you off balance.",
          "You are unable to see anything as you are blind.",
          "The forest seems to close up before you and you are prevented from moving that ",
          "There is no exit in that direction.",
          "The ground seems to tilt and you fall with a thump.",
          "Your legs are tangled in a mass of rope and you cannot move.",
          "You must stand up to do that.",
          "Your legs are crippled, how will you move?",
          "You cannot walk through an enormous stone gate.",
          "You must regain your equilibrium first.",
          "You must regain balance first.",
          "You fumble about drunkenly.",
          "You try to move, but the quicksand holds you back.",
          "You struggle to escape the quicksand, but only manage to partly free yourself.",
          "A loud sucking noise can be heard as you try desperately to escape the mud.",
          "You struggle to escape the mud, but only manage to partly free yourself.",
          "A strange force prevents the movement.",
          
          /* Lusternia. */
          "Now now, don't be so hasty!",
          "You are recovering equilibrium and cannot move yet.",
          "There's water ahead of you. You'll have to swim in that direction to make it ",
          
          /* Aetolia. */
          "You must first raise yourself from the floor and stand up.",
          
          NULL
     };
   int i;
   
   DEBUG( "i_mapper_process_server_line" );
   
   /* Are we sprinting, now? */
   parse_sprint( line );
   
   /* Is this a special exit message? */
   parse_special_exits( line );
   
   /* Is this a follow message, if we're following someone? */
   parse_follow( line );
   
   /* Gag/replace the alertness message, with something easier to read. */
   parse_alertness( line );
   
   /* Is this a sense/seek command? */
   if ( !disable_locating )
     {
        parse_msense( line );
        parse_window( line );
        parse_scent( line );
        parse_scry( line );
        parse_ka( line );
        parse_seek( line );
        parse_scout( line );
        parse_view( line );
        parse_pursue( line );
        parse_eventstatus( line );
        parse_petlist( line, len );
        parse_wormholes( line );
        
        /* Is this a fullsense command? */
        parse_fullsense( line );
        parse_shrinesight( line );
     }
   
   /* Can we get the area name and room type from here? */
   parse_survey( line );
   
   for ( i = 0; block_messages[i]; i++ )  
     if ( !strcmp( line, block_messages[i] ) )
       {
          /* Remove last command from the queue. */
          if ( q_top )
            q_top--;
          
          if ( !q_top )
            del_timer( "queue_reset_timer" );
          
          break;
       }
   
   if ( mode == FOLLOWING || mode == CREATING )
     parse_underwater( line );
   
   parse_autobump( line );
   
   if ( auto_walk )
     {
        if ( !strcmp( line, "You'll have to swim to make it through the water in that direction." ) )
          {
             if ( disable_swimming )
               {
                  clientf( "\r\n" C_R "[Hint: Swimming is disabled. Use 'map config swim' to turn it back on.]" C_0 );
               }
          }
             
        if ( !strcmp( line, "You cannot move that fast, slow down!" ) ||
             !strcmp( line, "Now now, don't be so hasty!" ) )
          {
             auto_walk = 2;
          }
        
        if ( !strcmp( line, "There is a door in the way." ) )
          {
             door_closed = 1;
             door_locked = 0;
             auto_walk = 2;
          }
        
        if ( !strcmp( line, "The door is locked." ) )
          {
             door_closed = 0;
             door_locked = 1;
             auto_walk = 2;
          }
        
        if ( !strncmp( line, "You open the door to the ", 25 ) )
          {
             door_closed = 0;
             door_locked = 0;
             auto_walk = 2;
          }
        
        if ( !strncmp( line, "You unlock the door to the ", 27 ) )
          {
             door_closed = 1;
             door_locked = 0;
             auto_walk = 2;
          }
        
        if ( !strcmp( line, "You are not carrying a key for this door." ) )
          {
             door_closed = 0;
             door_locked = 0;
//           auto_walk = 0;
//           clientfr( "Auto-walk disabled." );
          }
        
        if ( !strncmp( line, "There is no door to the ", 24 ) )
          {
             door_closed = 0;
             door_locked = 0;
             auto_walk = 2;
          }
     }
}


void i_mapper_process_server_line( LINE *l )
{
   process_line( l->line, l->len );
   
   if ( ( auto_walk && ( !cmp( "You cannot move that fast, slow down!", l->line ) ||
                         !cmp( "Now now, don't be so hasty!", l->line ) ) ) ||
        ( auto_bump && ( !cmp( "There is no exit in that direction.", l->line ) ) ) )
     {
        hide_line( );
     }
   
   //parse_who( l );

   /* Is this a room? Parse it. */
   parse_room( l, NULL, l->len );
}



void i_mapper_process_server_prompt( LINE *l )
{
   DEBUG( "i_mapper_process_server_prompt" );
   
   if ( parsing_room )
     parsing_room = 0;
   
   if ( sense_message )
     sense_message = 0;
   
   if ( scout_list )
     scout_list = 0;
   
   if ( evstatus_list )
     evstatus_list = 0;
   
   if ( pet_list )
     pet_list = 0;
   
   if ( capture_special_exit )
     cse_message[0] = 0;
   
   if ( get_unlost_exits )
     {
        ROOM_DATA *r, *found = NULL;
        int count = 0, i;
        
        get_unlost_exits = 0;
        
        if ( !current_room )
          return;
        
        for ( r = world; r; r = r->next_in_world )
          {
             if ( !strcmp( r->name, current_room->name ) )
               {
                  int good = 1;
                  
                  for ( i = 1; dir_name[i]; i++ )
                    {
                       if ( ( ( r->exits[i] || r->detected_exits[i] )
                              && !get_unlost_detected_exits[i] ) ||
                            ( !( r->exits[i] || r->detected_exits ) &&
                              get_unlost_detected_exits[i] ) )
                         {
                            good = 0;
                            break;
                         }
                    }
                  
                  if ( good )
                    {
                       if ( !found )
                         found = r;
                       
                       count++;
                    }
               }
          }
        
        if ( !found )
          {
             prefix( C_R "[No perfect matches found.]\r\n" C_0 );
             mode = GET_UNLOST;
          }
        else
          {
             current_room = found;
             current_area = current_room->area;
             mode = FOLLOWING;
             
             if ( !count )
               {
                  prefix( C_W "IMapper: Impossible error.\r\n" );
                  return;
               }
             
             prefixf( C_R "[Match probability: %d%%]\r\n" C_0, 100 / count );
          }
     }
   
   if ( check_for_duplicates )
     {
        check_for_duplicates = 0;
        
        if ( current_room && mode == CREATING )
          {
             ROOM_DATA *r;
             int count = 0, far_count = 0, i;
             
             for ( r = world; r; r = r->next_in_world )
               if ( !strcmp( current_room->name, r->name ) && r != current_room )
                 {
                    int good = 1;
                    
                    for ( i = 1; dir_name[i]; i++ )
                      {
                         int e1 = r->exits[i] ? 1 : r->detected_exits[i];
                         int e2 = current_room->exits[i] ? 1 : current_room->detected_exits[i];
                         
                         if ( e1 != e2 )
                           {
                              good = 0;
                              break;
                           }
                      }
                    
                    if ( good )
                      {
                         if ( r->area == current_room->area )
                           count++;
                         else
                           far_count++;
                      }
                 }
             
             if ( count || far_count )
               {
                  prefixf( C_R "[Warning: Identical rooms found... %d in this area, %d in other areas.]\r\n" C_0,
                           count, far_count );
               }
          }
     }
   
   if ( current_room && !pear_defence &&
        ( current_room->underwater ||
          current_room->room_type->underwater ) )
     {
        pear_defence = 1;
        send_to_server( "outr pear\r\neat pear\r\n" );
        clientf( C_W "(" C_G "outr/eat pear" C_W ") " C_0 );
     }
   
   if ( mode == FOLLOWING && auto_walk == 2 )
     {
        go_next( );
     }
   
   check_autobump( );
   
   if ( last_room != current_room && current_room )
     {
        show_floating_map( current_room );
        last_room = current_room;
     }
   
   if ( gag_next_prompt )
     {
        hide_line( );
     }
}



void i_mapper_process_server_paragraph( LINES *l )
{
   int line;
   
   for ( line = 1; line <= l->nr_of_lines; line++ )
     {
        set_line( line );
        
        process_line( l->line[line], l->len[line] );
        
        parse_who( l, line );
        
        if ( ( auto_walk && ( !cmp( "You cannot move that fast, slow down!", l->line[line] ) ||
                              !cmp( "Now now, don't be so hasty!", l->line[line] ) ) ) ||
             ( auto_bump && ( !cmp( "There is no exit in that direction.", l->line[line] ) ) ) )
          {
             hide_line( );
          }
     }
   
   /* Is this a room? Parse it. */
   parse_room2( l );
   
   set_line( -1 );
   i_mapper_process_server_prompt( NULL );
}



AREA_DATA *get_area_by_name( char *string )
{
   AREA_DATA *area = NULL, *a;
   
   if ( !string[0] )
     {
        if ( !current_area )
          {
             clientfr( "No current area set." );
             return NULL;
          }
        else
          return current_area;
     }
   
   for ( a = areas; a; a = a->next )
     if ( case_strstr( a->name, string ) )
       {
          if ( !area )
            area = a;
          else
            {
               /* More than one area of that name. */
               /* List them all, and return. */
               
               clientfr( "Multiple matches found:" );
               
               clientff( " - %s\r\n", area->name );
               clientff( " - %s\r\n", a->name );
               for ( area = a->next; area; area = area->next )
                 if ( case_strstr( area->name, string ) )
                   {
                      clientff( " - %s\r\n", area->name );
                   }
               
               return NULL;
            }
       }
   
   if ( !area )
     {
        clientfr( "No area names match that." );
        return NULL;
     }
   
   return area;
}



/* Map commands. */

void do_map( char *arg )
{
   ROOM_DATA *room;
   
   if ( arg[0] && isdigit( arg[0] ) )
     {
        if ( !( room = get_room( atoi( arg ) ) ) )
          {
             clientfr( "No room with that vnum found." );
             return;
          }
     }
   else
     {
        if ( current_room )
          room = current_room;
        else
          {
             clientfr( "No current room set." );
             return;
          }
     }
   
   show_map_new( room );
}



void do_map_old( char *arg )
{
   ROOM_DATA *room;
   
   if ( arg[0] && isdigit( arg[0] ) )
     {
        if ( !( room = get_room( atoi( arg ) ) ) )
          {
             clientfr( "No room with that vnum found." );
             return;
          }
     }
   else
     {
        if ( current_room )
          room = current_room;
        else
          {
             clientfr( "No current room set." );
             return;
          }
     }
   
   show_map( room );
}



void do_map_help( char *arg )
{
   clientfr( "Module: IMapper. Commands:" );
   clientf( " map help     - This help.\r\n"
            " area help    - Area commands help.\r\n"
            " room help    - Room commands help.\r\n"
            " exit help    - Room exit commands help.\r\n"
            " map load     - Load map.\r\n"
            " map save     - Save map.\r\n"
            " map none     - Disable following or mapping.\r\n"
            " map follow   - Enable following.\r\n"
            " map create   - Enable mapping.\r\n"
            " map path #   - Build directions to vnum #.\r\n"
            " map status   - Show general information about the map.\r\n"
            " map color    - Change the color of the room title.\r\n"
            " map file     - Set the file for map load and map save.\r\n"
            " map teleport - Manage global special exits.\r\n"
            " map queue    - Show the command queue.\r\n"
            " map queue cl - Clear the command queue.\r\n"
            " map config   - Configure the mapper.\r\n"
            " map window   - Open a floating MXP-based window.\r\n"
            " map          - Generate a map, from the current room.\r\n"
            " map #        - Generate a map centered on vnum #.\r\n"
            " landmarks    - Show all the landmarks, in the world.\r\n"
            " go/stop      - Begins, or stops speedwalking.\r\n" );
}



void do_map_create( char *arg )
{
   if ( !strcmp( arg, "newroom" ) )
     {
        clientfr( "Mapping on. Please 'look', to update this room." );
        
        if ( world == NULL )
          {
             create_area( );
             current_area = areas;
             current_area->name = strdup( "New area" );
          }
        
        current_room = create_room( -1 );
        current_room->name = strdup( "New room." );
        q_top = 0;
     }
   else if ( !current_room )
     {
        clientfr( "No current room set, from which to begin mapping." );
        clientfr( "In case it's really what you need, use 'map create newroom'." );
        return;
     }
   else
     clientfr( "Mapping on." );
   
   mode = CREATING;
}



void do_map_follow( char *arg )
{
   if ( !current_room )
     {
        mode = GET_UNLOST;
        clientfr( "Will try to find ourselves on the map." );
     }
   else
     {
        mode = FOLLOWING;
        clientfr( "Following on." );
     }
}



void do_map_none( char *arg )
{
   if ( mode != NONE )
     {
        mode = NONE;
        clientfr( "Mapping and following off." );
     }
   else
     clientfr( "Mapping or following were not enabled, anyway." );
}



void do_map_save( char *arg )
{
   if ( !strcmp( arg, "binary" ) )
     {
        save_binary_map( map_file_bin );
        return;
     }
   
   if ( areas )
     save_map( map_file );
   else
     clientfr( "No areas to save." );
}



void do_map_load( char *arg )
{
   char buf[256];
   
   destroy_map( );
   get_timer( );
   if ( !strcmp( arg, "binary" ) ? load_binary_map( map_file_bin ) : load_map( map_file ) )
     {
        destroy_map( );
        clientfr( "Couldn't load map." );
     }
   else
     {
        sprintf( buf, "Map loaded. (%d microseconds)", get_timer( ) );
        clientfr( buf );
        load_settings( "config.mapper.txt" );
        
        mode = GET_UNLOST;
     }
}



void do_map_path( char *arg )
{
   ROOM_DATA *room;
   ELEMENT *tag;
   char buf[256];
   
   /* Force an update of the floating map. */
   last_room = NULL;
   
   clean_paths( );
   
   if ( !arg[0] )
     {
        clientfr( "Map directions cleared." );
        clientfr( "Usage: map path [near] <vnum/tag> [[near] <vnum2/tag2>]..." );
        
        return;
     }
   
   while ( arg[0] )
     {
        int neari = 0;
        
        arg = get_string( arg, buf, 256 );
        
        if ( !strcmp( buf, "near" ) )
          {
             arg = get_string( arg, buf, 256 );
             neari = 1;
          }
        
        /* Vnum. */
        if ( isdigit( buf[0] ) )
          {
             if ( !( room = get_room( atoi( buf ) ) ) )
               {
                  clientff( C_R "[No room with the vnum '%s' was found.]\r\n" C_0, buf );
                  clean_paths( );
                  return;
               }
             
             if ( !neari )
               {
                  add_to_pathfinder( room, 1 );
               }
             else
               {
                  for ( neari = 1; dir_name[neari]; neari++ )
                    if ( room->exits[neari] )
                      {
                         add_to_pathfinder( room->exits[neari], 1 );
                      }
               }
             
          }
        /* Tag. */
        else
          {
             int found = 0;
             
             for ( room = world; room; room = room->next_in_world )
               for ( tag = room->tags; tag; tag = tag->next )
                 if ( !case_strcmp( buf, (char *) tag->p ) )
                   {
                      if ( !neari )
                        {
                           add_to_pathfinder( room, 1 );
                        }
                      else
                        {
                           for ( neari = 1; dir_name[neari]; neari++ )
                             if ( room->exits[neari] )
                               {
                                  add_to_pathfinder( room->exits[neari], 1 );
                               }
                        }
                      found = 1;
                   }
             
             if ( !found )
               {
                  clientff( C_R "[No room with the tag '%s' was found.]\r\n" C_0, buf );
                  clean_paths( );
                  return;
               }
          }
     }
   
   get_timer( );
   path_finder_2( );
   
   sprintf( buf, "Path calculated in: %d microseconds.", get_timer( ) );
   clientfr( buf );
   
   if ( current_room )
     show_path( current_room );
   else
     clientfr( "Can't show the path from here though, as no current room is set." );
}



void do_map_status( char *arg )
{
   AREA_DATA *a;
   ROOM_DATA *r;
   char buf[256];
   int count = 0, count2 = 0;
   int i;
   
   DEBUG( "do_map_status" );
   
   clientfr( "General information:" );
   
   for ( r = world; r; r = r->next_in_world )
     count++, count2 += sizeof( ROOM_DATA );
   
   sprintf( buf, "Rooms: " C_G "%d" C_0 ", using " C_G "%d" C_0 " bytes of memory.\r\n", count, count2 );
   clientf( buf );
   
   count = 0, count2 = 0;
   
   for ( a = areas; a; a = a->next )
     {
        int notype = 0, unlinked = 0;
        count++;
        
        for ( r = a->rooms; r; r = r->next_in_area )
          {
             if ( !notype && r->room_type == null_room_type )
               notype = 1;
             if ( !unlinked )
               for ( i = 1; dir_name[i]; i++ )
                 if ( !r->exits[i] && r->detected_exits[i] &&
                      !r->locked_exits[i] )
                   unlinked = 1;
          }
        
        if ( !( notype || unlinked ) )
          count2++;
     }
   
   sprintf( buf, "Areas: " C_G "%d" C_0 ", of which fully mapped: " C_G "%d" C_0 ".\r\n", count, count2 );
   clientf( buf );
   
   sprintf( buf, "Room structure size: " C_G "%d" C_0 " bytes.\r\n", (int) sizeof( ROOM_DATA ) );
   clientf( buf );
   
   sprintf( buf, "Hash table size: " C_G "%d" C_0 " chains.\r\n", MAX_HASH );
   clientf( buf );
   
   sprintf( buf, "Map size, x: " C_G "%d" C_0 " y: " C_G "%d" C_0 ".\r\n", MAP_X, MAP_Y );
   clientf( buf );
   
   for ( i = 0; color_names[i].name; i++ )
     {
        if ( !strcmp( color_names[i].code, room_color ) )
          break;
     }
   
   sprintf( buf, "Room title color: %s%s" C_0 ", code length " C_G "%d" C_0 ".\r\n", room_color,
            color_names[i].name ? color_names[i].name : "unknown", room_color_len );
   clientf( buf );
}



void do_map_color( char *arg )
{
   char buf[256];
   int i;
   
   arg = get_string( arg, buf, 256 );
   
   for ( i = 0; color_names[i].name; i++ )
     {
        if ( !strcmp( color_names[i].name, buf ) )
          {
             room_color = color_names[i].code;
             room_color_len = color_names[i].length;
             
             sprintf( buf, "Room title color changed to: " C_0 "%s%s" C_R ".",
                      room_color, color_names[i].name );
             clientfr( buf );
             clientfr( "Use 'map config save' to make it permanent." );
             
             return;
          }
     }
   
   clientfr( "Possible room-title colors:" );
   
   for ( i = 0; color_names[i].name; i++ )
     {
        sprintf( buf, "%s - " C_0 "%s%s" C_0 ".\r\n",
                 !strcmp( color_names[i].code, room_color ) ? C_R : "",
                 color_names[i].code, color_names[i].name );
        clientf( buf );
     }
   
   clientfr( "Use 'map color <name>' to change it." );
}



void do_map_bump( char *arg )
{
   if ( !current_room )
     {
        clientfr( "No current room set." );
        return;
     }
   
   if ( !strcmp( arg, "skip" ) )
     {
        if ( bump_room && bump_room->next_in_area )
          {
             bump_room = bump_room->next_in_area;
             auto_bump = 3;
             clientfr( "Skipped one room." );
          }
        else
          {
             clientfr( "Skipping a room is not possible." );
             return;
          }
     }
   
   else if ( !strcmp( arg, "continue" ) )
     {
        if ( bump_room )
          {
             clientfr( "Bumping continues." );
             
             auto_bump = 3;
          }
        else
          {
             clientfr( "Unable to continue bumping." );
             return;
          }
     }
   
   else if ( auto_bump )
     {
        auto_bump = 0;
        clientfr( "Bumping ended." );
        return;
     }
   
   else
     {
        clientfr( "Bumping begins." );
        
        auto_bump = 3;
        bump_room = current_room->area->rooms;
     }
   
   clean_paths( );
   add_to_pathfinder( bump_room, 1 );
   path_finder_2( );
   go_next( );
}



void do_map_queue( char *arg )
{
   char buf[256];
   int i;
   
   if ( arg[0] == 'c' )
     {
        q_top = 0;
        clientfr( "Queue cleared." );
        del_timer( "queue_reset_timer" );
        return;
     }
   
   if ( q_top )
     {
        clientfr( "Command queue:" );
        
        for ( i = 0; i < q_top; i++ )
          {
             sprintf( buf, C_R " %d: %s.\r\n" C_0, i,
                      queue[i] < 0 ? "look" : dir_name[queue[i]] );
             clientf( buf );
          }
     }
   else
     clientfr( "Queue empty." );
}



void do_map_config( char *arg )
{
   char option[256];
   int i;
   struct config_option
     {
        char *option;
        int *value;
        char *true_message;
        char *false_message;
     } options[ ] =
     {
          { "swim", &disable_swimming,
               "Swimming disabled - will now walk over water.",
               "Swimming enabled." },
          { "wholist", &disable_wholist,
               "Parsing disabled.",
               "Parsing enabled" },
          { "alertness", &disable_alertness,
               "Parsing disabled.",
               "Parsing enabled" },
          { "locate", &disable_locating,
               "Locating disabled.",
               "Vnums will now be displayed on locating abilities." },
          { "showarea", &disable_areaname,
               "The area name will no longer be shown.",
               "The area name will be shown after room titles." },
          { "title_mxp", &disable_mxp_title,
               "MXP tags will no longer be used to mark room titles.",
               "MXP tags will be used around room title." },
          { "exits_mxp", &disable_mxp_exits,
               "MXP tags will no longer be used to mark room exits.",
               "MXP tags will be used around room exits." },
          { "map_mxp", &disable_mxp_map,
               "MXP tags will no longer be used on a map.",
               "MXP tags will now be used on a map." },
          { "autolink", &disable_autolink, 
               "Auto-linking disabled.",
               "Auto-linking enabled." },
          { "sewer_grates", &disable_sewer_grates, 
               "Pathfinding through sewer grates disabled.",
               "Pathfinding through sewer grates enabled." },
          { "god_rooms", &enable_godrooms, 
               "Parsing titles with GodRooms enabled.",
               "Parsing titles with GodRooms disabled." },
        
          { NULL, NULL, NULL, NULL }
     };
   
   arg = get_string( arg, option, 256 );
   
   if ( !strcmp( option, "save" ) )
     {
        if ( save_settings( "config.mapper.txt" ) )
          clientfr( "Unable to open the file for writing." );
        else
          clientfr( "All settings saved." );
        return;
     }
   else if ( !strcmp( option, "load" ) )
     {
        if ( load_settings( "config.mapper.txt" ) )
          clientfr( "Unable to open the file for reading." );
        else
          clientfr( "All settings reloaded." );
        return;
     }
   
   if ( option[0] )
     {
        for ( i = 0; options[i].option; i++ )
          {
             if ( strcmp( options[i].option, option ) )
               continue;
             
             *options[i].value = *options[i].value ? 0 : 1;
             if ( *options[i].value )
               clientfr( options[i].true_message );
             else
               clientfr( options[i].false_message );
             
             clientfr( "Use 'map config save' to make it permanent." );
             
             return;
          }
     }
   
   clientfr( "Commands:" );
   clientf( " map config save         - Save all settings.\r\n"
            " map config load         - Reload the previously saved settings.\r\n"
            " map config swim         - Toggle swimming.\r\n"
            " map config wholist      - Parse and replace the 'who' list.\r\n"
            " map config alertness    - Parse and replace alertness messages.\r\n"
            " map config locate       - Append vnums to various locating abilities.\r\n"
            " map config showarea     - Show the current area after a room title.\r\n"
            " map config title_mxp    - Mark the room title with MXP tags.\r\n"
            " map config exits_mxp    - Mark the room exits with MXP tags.\r\n"
            " map config map_mxp      - Use MXP tags on map generation.\r\n"
            " map config autolink     - Link rooms automatically when mapping.\r\n"
            " map config sewer_grates - Toggle pathfinding through sewer grates.\r\n" );
}



void do_map_orig( char *arg )
{
   ROOM_DATA *room, *r;
   int rooms = 0, original_rooms = 0;
   
   for ( room = world; room; room = room->next_in_world )
     {
        /* Add one to the room count. */
        rooms++;
        
        /* And check if there is any other room with this same name. */
        original_rooms++;
        for ( r = world; r; r = r->next_in_world )
          if ( !strcmp( r->name, room->name ) && r != room )
            {
               original_rooms--;
               break;
            }
     }
   
   if ( !rooms )
     {
        clientfr( "The area is empty..." );
        return;
     }
   
   clientff( C_R "[Rooms: " C_G "%d" C_R "  Original rooms: " C_G "%d" C_R
             "  Originality: " C_G "%d%%" C_R "]\r\n" C_0,
             rooms, original_rooms,
             original_rooms ? original_rooms * 100 / rooms : 0 );
}



void do_map_file( char *arg )
{
   if ( !arg[0] )
     {
        strcpy( map_file, "IMap" );
        strcpy( map_file, "IMap.bin" );
     }
   else
     {
        strcpy( map_file, arg );
        strcpy( map_file_bin, arg );
        strcat( map_file_bin, ".bin" );
     }
   
   clientff( C_R "[File for map load/save set to '%s'.]\r\n" C_0, map_file );
}



/* Snatched from do_exit_special. */
void do_map_teleport( char *arg )
{
   EXIT_DATA *spexit;
   char cmd[256];
   char buf[256];
   int i, nr = 0;
   
   DEBUG( "do_map_teleport" );
   
   if ( mode != CREATING && strcmp( arg, "list" ) )
     {
        clientfr( "Turn mapping on, first." );
        return;
     }
   
   if ( !strncmp( arg, "help", strlen( arg ) ) )
     {
        clientfr( "Syntax: map teleport <command> [exit] [args]" );
        clientfr( "Commands: list, add, create, destroy, name, message, link" );
        return;
     }
   
   arg = get_string( arg, cmd, 256 );
   
   if ( !strcmp( cmd, "list" ) )
     {
        clientfr( "Global special exits:" );
        for ( spexit = global_special_exits; spexit; spexit = spexit->next )
          {
             sprintf( buf, C_B "%3d" C_0 " - L: " C_G "%4d" C_0 " N: '" C_g "%s" C_0 "' M: '" C_g "%s" C_0 "'\r\n",
                      nr++, spexit->vnum, spexit->command, spexit->message );
             clientf( buf );
          }
        
        if ( !nr )
          clientf( " - None.\r\n" );
     }
   
   else if ( !strcmp( cmd, "add" ) )
     {
        char name[256];
        
        arg = get_string( arg, name, 256 );
        
        if ( !arg[0] )
          {
             clientfr( "Syntax: map teleport add <name> <message>" );
             return;
          }
        
        spexit = create_exit( NULL );
        spexit->command = strdup( name );
        spexit->message = strdup( arg );
        
        clientfr( "Global special exit created." );
     }
   
   else if ( !strcmp( cmd, "create" ) )
     {
        create_exit( NULL );
        clientfr( "Global special exit created." );
     }
   
   else if ( !strcmp( cmd, "destroy" ) )
     {
        int done = 0;
        
        if ( !arg[0] || !isdigit( arg[0] ) )
          {
             clientfr( "What global special exit do you wish to destroy?" );
             return;
          }
        
        nr = atoi( arg );
        
        for ( i = 0, spexit = global_special_exits; spexit; spexit = spexit->next )
          {
             if ( i++ == nr )
               {
                  destroy_exit( spexit );
                  done = 1;
                  break;
               }
          }
        
        if ( done )
          sprintf( buf, "Global special exit %d destroyed.", nr );
        else
          sprintf( buf, "Global special exit %d was not found.", nr );
        clientfr( buf );
     }
   
   else if ( !strcmp( cmd, "message" ) )
     {
        int done = 0;
        
        /* We'll store the number in cmd. */
        arg = get_string( arg, cmd, 256 );
        
        if ( !isdigit( cmd[0] ) )
          {
             clientfr( "Set message on what global special exit?" );
             return;
          }
        
        nr = atoi( cmd );
        
        for ( i = 0, spexit = global_special_exits; spexit; spexit = spexit->next )
          {
             if ( i++ == nr )
               {
                  if ( spexit->message )
                    free( spexit->message );
                  
                  spexit->message = strdup( arg );
                  done = 1;
                  break;
               }
          }
        
        if ( done )
          sprintf( buf, "Message on global exit %d changed to '%s'", nr, arg );
        else
          sprintf( buf, "Can't find global special exit %d.", nr );
        
        clientfr( buf );
     }
   
   else if ( !strcmp( cmd, "name" ) )
     {
        int done = 0;
        
        /* We'll store the number in cmd. */
        arg = get_string( arg, cmd, 256 );
        
        if ( !isdigit( cmd[0] ) )
          {
             clientfr( "Set name on what global special exit?" );
             return;
          }
        
        nr = atoi( cmd );
        
        for ( i = 0, spexit = global_special_exits; spexit; spexit = spexit->next )
          {
             if ( i++ == nr )
               {
                  if ( spexit->command )
                    free( spexit->command );
                  
                  if ( arg[0] )
                    spexit->command = strdup( arg );
                  done = 1;
                  break;
               }
          }
        
        if ( done )
          {
             if ( arg[0] )
               sprintf( buf, "Name on global exit %d changed to '%s'", nr, arg );
             else
               sprintf( buf, "Name on global exit %d cleared.", nr );
          }
        else
          sprintf( buf, "Can't find global special exit %d.", nr );
        
        clientfr( buf );
     }
   
   else if ( !strcmp( cmd, "link" ) )
     {
        /* E special link 0 -1 */
        char vnum[256];
        
        /* We'll store the number in cmd. */
        arg = get_string( arg, cmd, 256 );
        
        if ( !isdigit( cmd[0] ) )
          {
             clientfr( "Link which global special exit?" );
             return;
          }
        
        nr = atoi( cmd );
        
        for ( i = 0, spexit = global_special_exits; spexit; spexit = spexit->next )
          {
             if ( i++ == nr )
               {
                  get_string( arg, vnum, 256 );
                  
                  if ( vnum[0] == '-' )
                    {
                       ROOM_DATA *to;
                       
                       to = spexit->to;
                       
                       spexit->vnum = -1;
                       spexit->to = NULL;
                       
                       sprintf( buf, "Link cleared on exit %d.", nr );
                       clientfr( buf );
                       return;
                    }
                  else if ( isdigit( vnum[0] ) )
                    {
                       spexit->vnum = atoi( vnum );
                       spexit->to = get_room( spexit->vnum );
                       if ( !spexit->to )
                         {
                            clientfr( "A room whith that vnum was not found." );
                            spexit->vnum = -1;
                            return;
                         }
                       sprintf( buf, "Global special exit %d linked to '%s'.",
                                nr, spexit->to->name );
                       clientfr( buf );
                       return;
                    }
                  else
                    {
                       clientfr( "Link to which vnum?" );
                       return;
                    }
               }
          }
        
        sprintf( buf, "Can't find global special exit %d.", nr );
        clientfr( buf );
     }
   
   else
     {
        clientfr( "Unknown command... Try 'map teleport help'." );
     }
}




void do_map_window( char *arg )
{
   if ( floating_map_enabled )
     {
        floating_map_enabled = 0;
        clientfr( "Floating MXP map disabled. Don't forget to close the window!" );
        return;
     }
   
   if ( !mxp_tag( TAG_LOCK_SECURE ) )
     {
        floating_map_enabled = 0;
        clientfr( "Unable to get a SECURE tag. Maybe your client does not support MXP?" );
        return;
     }
   
   floating_map_enabled = 1;
   mxp( "<FRAME IMapper Left=\"-57c\" Top=\"2c\" Height=\"21c\""
        " Width=\"55c\" FLOATING>" );
   mxp( "<DEST IMapper>" );
   mxp_tag( TAG_LOCK_SECURE );
   mxp( "<!element mpelm '<send \"map path &v;|room look &v;\" "
        "hint=\"&r;|Vnum: &v;|Type: &t;\">' ATT='v r t'>" );
   mxp( "<!element mppers '<send \"map path &v;|room look &v;|who &p;\" "
        "hint=\"&p; (&r;)|Vnum: &v;|Type: &t;|Player: &p;\">' ATT='v r t p'>" );
   mxp( "</DEST>" );
   mxp_tag( TAG_DEFAULT );
   clientfr( "Enabled. Warning, this may slow you down." );
}




/* Area commands. */

void do_area_help( char *arg )
{
   clientfr( "Module: IMapper. Area commands:" );
   clientf( " area list   - List all areas.\r\n"
            " area check  - Check for problems in the current areas.\r\n"
            " area switch - Switch the area of the current room.\r\n"
            " area update - Update current area's name.\r\n"
            " area off    - Disable pathfinding in the current area.\r\n"
            " area orig   - Check the originality of the area.\r\n" );
}



// This could be cleaned up and beautified.
void do_area_check( char *arg )
{
   ROOM_DATA *room;
   char buf[256];  
   int detected_only = 0, unknown_type = 0;
   int rooms = 0, i;
   
   DEBUG( "do_area_check" );
   
   if ( !current_area )
     {
        clientfr( "No current area." );
        return;
     }
   
   clientfr( "Unlinked or no-environment rooms:" );
   
   for ( room = current_area->rooms; room; room = room->next_in_area )
     {
        int unlinked = 0, notype = 0, locked = 0;
        
        if ( room->room_type == null_room_type )
          notype = 1;
        for ( i = 1; dir_name[i]; i++ )
          if ( !room->exits[i] && room->detected_exits[i] )
            {
               if ( room->locked_exits[i] )
                 locked = 1;
               else
                 unlinked = 1;
            }
        
        if ( unlinked || notype )
          {
             sprintf( buf, " - %s (" C_G "%d" C_0 ")", room->name, room->vnum );
             if ( unlinked )
               strcat( buf, C_B " (" C_G "unlinked" C_B ")" C_0 );
             else if ( locked )
               strcat( buf, C_B " (" C_G "locked" C_B ")" C_0 );
             if ( notype )
               strcat( buf, C_B " (" C_G "no type" C_B ")" C_0 );
             strcat( buf, "\r\n" );
             clientf( buf );
             rooms++;
          }
     }
   if ( !rooms )
     clientff( " - None.\r\n" );
   
   rooms = 0;
   
   sprintf( buf, "Area: %s", current_area->name );
   clientfr( buf );
   
   for ( room = current_area->rooms; room; room = room->next_in_area )
     {
        rooms++;
        
        if ( room->room_type == null_room_type )
          unknown_type++;
        
        for ( i = 1; dir_name[i]; i++ )
          if ( room->detected_exits[i] && !room->exits[i] &&
               !room->locked_exits[i] )
            detected_only++;
     }
   
   sprintf( buf, C_R "Rooms: " C_G "%d" C_R "  Unlinked exits: " C_G
            "%d" C_R "  Unknown type rooms: " C_G "%d" C_R ".",
            rooms, detected_only, unknown_type );
   clientfr( buf );
}



void do_area_orig( char *arg )
{
   ROOM_DATA *room, *r;
   int rooms = 0, original_rooms = 0;
   
   if ( !current_area )
     {
        clientfr( "No current area." );
        return;
     }
   
   for ( room = current_area->rooms; room; room = room->next_in_area )
     {
        /* Add one to the room count. */
        rooms++;
        
        /* And check if there is any other room with this same name. */
        original_rooms++;
        for ( r = current_area->rooms; r; r = r->next_in_area )
          if ( !strcmp( r->name, room->name ) && r != room )
            {
               original_rooms--;
               break;
            }
     }
   
   if ( !rooms )
     {
        clientfr( "The area is empty..." );
        return;
     }
   
   clientff( C_R "[Rooms: " C_G "%d" C_R "  Original rooms: " C_G "%d" C_R
             "  Originality: " C_G "%d%%" C_R "]\r\n" C_0,
             rooms, original_rooms,
             original_rooms ? original_rooms * 100 / rooms : 0 );
}



void do_area_list( char *arg )
{
   AREA_DATA *area;
   ROOM_DATA *room;
   const int align = 37;
   char spcs[256];
   char buf[1024];
   int space = 0;
   int right = 1;
   int i;
   
   DEBUG( "do_area_list" );
   
   clientfr( "Areas:" );
   for ( area = areas; area; area = area->next )
     {
        int unlinked = 0, notype = 0;
        
        for ( room = area->rooms; room; room = room->next_in_area )
          {
             if ( !notype && room->room_type == null_room_type )
               notype = 1;
             if ( !unlinked )
               for ( i = 1; dir_name[i]; i++ )
                 if ( !room->exits[i] && room->detected_exits[i] &&
                      !room->locked_exits[i] )
                   unlinked = 1;
          }
        
        if ( !right )
          {
             space = align - strlen( area->name );
             spcs[0] = '\r', spcs[1] = '\n', spcs[2] = 0;
             right = 1;
          }
        else
          {
             for ( i = 0; i < space; i++ )
               spcs[i] = ' ';
             spcs[i] = 0;
             right = 0;
          }
        clientf( spcs );
        
        sprintf( buf, " (%s%c" C_0 ") %s%s%s",
                 ( notype || unlinked ) ? C_R : C_B,
                 ( notype || unlinked ) ? 'x' : '*',
                 area == current_area ? C_W : ( area->disabled ? C_D : "" ),
                 area->name, area == current_area ? C_0 : "" );
        clientf( buf );
     }
   clientf( "\r\n\r\n" );
   clientf( "  " C_B "*" C_0 " - Mapped entirely.                      "
            C_R "x" C_0 " - Mapped partially.\r\n" );
}



void do_area_switch( char *arg )
{
   AREA_DATA *area;
   char buf[256];
   
   if ( mode != CREATING )
     {
        clientfr( "Turn mapping on, first." );
        return;
     }
   
   if ( !current_room )
     {
        clientfr( "No current room, to change." );
        return;
     }
   
   /* Move in a circular motion. */
   area = current_room->area->next;
   if ( !area )
     area = areas;
   if ( current_room->area )
     unlink_from_area( current_room );
   link_to_area( current_room, area );
   current_area = current_room->area;
   
   sprintf( buf, "Area switched to '%s'.",
            current_area->name );
   clientfr( buf );
}



void do_area_update( char *arg )
{
   if ( mode != CREATING )
     {
        clientfr( "Turn mapping on, first." );
        return;
     }
   
   if ( update_area_from_survey )
     {
        update_area_from_survey = 0;
        clientfr( "Disabled." );
     }
   else
     {
        update_area_from_survey = 1;
        clientfr( "Type survey. The current area's name will be updated." );
     }
}



void do_area_off( char *arg )
{
   AREA_DATA *area;
   char buf[256];
   
   if ( mode != CREATING )
     {
        clientfr( "Turn mapping on, first." );
        return;
     }
   
   area = get_area_by_name( arg );
   
   if ( !area )
     return;
   
   area->disabled = area->disabled ? 0 : 1;
   
   if ( area->disabled )
     sprintf( buf, "Pathfinding in '%s' disabled.", area->name );
   else
     sprintf( buf, "Pathfinding in '%s' re-enabled.", area->name );
   
   clientfr( buf );
}




/* Room commands. */

void do_room_help( char *arg )
{
   clientfr( "Module: IMapper. Room commands:" );
   clientf( " room switch  - Switch current room to another vnum.\r\n"
            " room look    - Show info on the current room.\r\n"
            " room find    - Find all rooms that contain something in their name.\r\n"
            " room destroy - Unlink and destroy a room.\r\n"
            " room list    - List all rooms in the current area.\r\n"
            " room underw  - Set the room as Underwater.\r\n"
            " room mark    - Set or clear a landmark on a vnum, or current room.\r\n"
            " room types   - List all known room types (environments).\r\n"
            " room merge   - Combine two identical rooms into one.\r\n" );
}



void do_room_switch( char *arg )
{
   ROOM_DATA *room;
   char buf[256];
   
   if ( !isdigit( *(arg) ) )
     clientfr( "Specify a vnum to switch to." );
   else
     {
        room = get_room( atoi( arg ) );
        if ( !room )
          clientfr( "No room with that vnum was found." );
        else
          {
             sprintf( buf, "Switched to '%s' (%s).", room->name,
                      room->area->name );
             clientfr( buf );
             current_room = room;
             current_area = room->area;
             if ( mode == GET_UNLOST )
               mode = FOLLOWING;
          }
     }
}



void do_room_find( char *arg )
{
   ROOM_DATA *room;
   char buf[256];
   
   if ( *(arg) == 0 )
     {
        clientfr( "Find what?" );
        return;
     }
   
   /* Looking for a room type? */
   if ( !strncmp( arg, "type ", 5 ) )
     {
        ROOM_TYPE *type;
        
        for ( type = room_types; type; type = type->next )
          if ( !strcmp( type->name, arg+5 ) )
            break;
        
        if ( !type )
          clientfr( "What room type? Use 'room types' for a list." );
        else
          {
             clientfr( "Rooms that match:" );
             for ( room = world; room; room = room->next_in_world )
               if ( room->room_type == type )
                 clientff( " " C_D "(" C_G "%d" C_D ")" C_0 " %s%s%s%s\r\n",
                           room->vnum, room->name,
                           room->area != current_area ? " (" : "",
                           room->area != current_area ? room->area->name : "",
                           room->area != current_area ? ")" : "" );
          }
        
        return;
     }
   
   clientfr( "Rooms that match:" );
   for ( room = world; room; room = room->next_in_world )
     {
        if ( case_strstr( room->name, arg ) )
          {
             sprintf( buf, " " C_D "(" C_G "%d" C_D ")" C_0 " %s%s%s%s",
                      room->vnum, room->name,
                      room->area != current_area ? " (" : "",
                      room->area != current_area ? room->area->name : "",
                      room->area != current_area ? ")" : "" );
             if ( mode == CREATING )
               {
                  int i, first = 1;
                  for ( i = 1; dir_name[i]; i++ )
                    {
                       if ( room->exits[i] || room->detected_exits[i] )
                         {
                            if ( first )
                              {
                                 strcat( buf, C_D " (" );
                                 first = 0;
                              }
                            else
                              strcat( buf, C_D "," );
                            
                            if ( room->exits[i] )
                              strcat( buf, C_B );
                            else
                              strcat( buf, C_R );
                            
                            strcat( buf, dir_small_name[i] );
                         }
                    }
                  if ( !first )
                    strcat( buf, C_D ")" C_0 );
               }
             strcat( buf, "\r\n" );
             clientf( buf );
          }
     }
}



void do_room_look( char *arg )
{
   ROOM_DATA *room;
   EXIT_DATA *spexit;
   char buf[256];
   int i, nr;
   
   if ( arg[0] && isdigit( arg[0] ) )
     {
        if ( !( room = get_room( atoi( arg ) ) ) )
          {
             clientfr( "No room with that vnum found." );
             return;
          }
     }
   else
     {
        if ( current_room )
          room = current_room;
        else
          {
             clientfr( "No current room set." );
             return;
          }
     }
   
   sprintf( buf, "Room: %s  Vnum: %d.  Area: %s", room->name ?
            room->name : "-null-", room->vnum, room->area->name );
   clientfr( buf );
   sprintf( buf, "Type: %s.  Underwater: %s.",
            room->room_type->name,
            room->underwater ? "Yes" : "No" );
   clientfr( buf );
   if ( room->tags )
     {
        ELEMENT *tag;
        
        sprintf( buf, "Tags:" );
        for ( tag = room->tags; tag; tag = tag->next )
          {
             strcat( buf, " " );
             strcat( buf, (char *) tag->p );
          }
        strcat( buf, "." );
        clientfr( buf );
     }
   
   for ( i = 1; dir_name[i]; i++ )
     {
        if ( room->exits[i] )
          {
             char lngth[128];
             if ( room->exit_length[i] )
               sprintf( lngth, " (%d)", room->exit_length[i] );
             else
               lngth[0] = 0;
             
             clientff( "  " C_B "%s" C_0 ": (" C_G "%d" C_0 ") %s%s\r\n", dir_name[i],
                      room->exits[i]->vnum,
                      room->exits[i]->name, lngth );
          }
        else if ( room->detected_exits[i] )
          {
             if ( room->locked_exits[i] )
               clientff( "  %s: locked exit.\r\n", dir_name[i] );
             else
               clientff( "  %s: unlinked exit.\r\n", dir_name[i] );
          }
     }
   
   nr = 0;
   for ( spexit = room->special_exits; spexit; spexit = spexit->next )
     {
        if ( !nr )
          clientfr( "Special exits:" );
        nr++;
        if ( spexit->alias )
          sprintf( buf, "(alias: %s) ", dir_name[spexit->alias] );
        else
          buf[0] = 0;
        
        clientff( C_B "%3d" C_0 ": '" C_g "%s" C_0 "' %s-> \"" C_g "%s" C_0 "\""
                  " -> " C_G "%d" C_0 ".\r\n",
                  nr, spexit->command ? spexit->command : "n/a", buf,
                  spexit->message, spexit->vnum );
     }
   
   /* Debugging v2.0 */
     {
        REVERSE_EXIT *rexit;
        
        if ( room->rev_exits )
          clientfr( "Reverse exits:" );
        
        for ( rexit = room->rev_exits; rexit; rexit = rexit->next )
          {
             clientff( C_B "  %s" C_0 ": (" C_G "%d" C_0 ") From %s\r\n",
                       rexit->direction ? dir_name[rexit->direction] : ( rexit->spexit->command ? rexit->spexit->command : "??" ),
                       rexit->from->vnum, rexit->from->name );
          }
     }
   
   
   /* Debugging.
   sprintf( buf, "PF-Cost: %d. PF-Direction: %s.", room->pf_cost,
            room->pf_direction ? dir_name[room->pf_direction] : "none" );
   clientfr( buf );
   sprintf( buf, "PF-Parent: %s", room->pf_parent ? room->pf_parent->name : "none" );
   clientfr( buf );*/
}



void do_room_destroy( char *arg )
{
   int i;
   
   if ( mode != CREATING )
     {
        clientfr( "Turn mapping on, first." );
        return;
     }
   
   if ( !isdigit( *(arg) ) )
     clientfr( "Specify a room's vnum to destroy." );
   else
     {
        ROOM_DATA *room;
        
        room = get_room( atoi( arg ) );
        if ( !room )
          {
             clientfr( "No room with such a vnum was found." );
             return;
          }
        
        if ( room == current_room )
          clientfr( "Can't destroy the room you're currently in." );
        else
          {
             for ( i = 1; dir_name[i]; i++ )
               if ( room->exits[i] )
                 unlink_rooms( room, i, room->exits[i] );
             
             /* We don't want pointers to point in unknown locations, do we? */
             while ( room->rev_exits )
               {
                  /* Normal exits. */
                  if ( room->rev_exits->direction )
                    unlink_rooms( room->rev_exits->from, room->rev_exits->direction, room );
                  /* Special exits. */
                  else
                    destroy_exit( room->rev_exits->spexit );
               }
             
             /* Replaced?
             for ( i = 1; dir_name[i]; i++ )
               if ( room->exits[i] )
                 set_reverse( NULL, i, room->exits[i] );
             
             for ( r = world; r; r = r->next_in_world )
               {
                  for ( i = 1; dir_name[i]; i++ )
                    if ( r->exits[i] == room )
                      r->exits[i] = NULL;
                  for ( e = r->special_exits; e; e = e_next )
                    {
                       e_next = e->next;
                       if ( e->to == room )
                         destroy_exit( e );
                    }
               }
              */
             
             destroy_room( room );
             clientfr( "Room unlinked and destroyed." );
          }
     }
   
   refind_last_vnum( );
}
             


void do_room_list( char *arg )
{
   AREA_DATA *area;
   ROOM_DATA *room;
   char buf[256];
   
   area = get_area_by_name( arg );
   
   if ( !area )
     return;
   
   room = area->rooms;
   
   clientff( C_R "[Rooms in " C_B "%s" C_R "]\r\n" C_0,
             room->area->name );
   while( room )
     {
        sprintf( buf, " " C_D "(" C_G "%d" C_D ")" C_0 " %s\r\n", room->vnum,
                 room->name ? room->name : "no name" );
        clientf( buf );
        room = room->next_in_area;
     }
}



void do_room_underw( char *arg )
{
   if ( !current_room )
     {
        clientfr( "No current room set." );
        return;
     }
   
   if ( mode != CREATING )
     {
        clientfr( "Turn mapping on, first." );
        return;
     }
   
   current_room->underwater = current_room->underwater ? 0 : 1;
   
   if ( current_room->underwater )
     clientfr( "Current room set as an underwater place." );
   else
     clientfr( "Current room set as normal, with a nice sky (or roof) above." );
}



void do_room_types( char *arg )
{
   ROOM_TYPE *type, *t;
   char cmd[256], *p = arg, *color;
   
   if ( !current_room )
     {
        clientfr( "No current room set." );
        return;
     }
   
   type = current_room->room_type;
   
   if ( !arg[0] )
     {
        clientff( C_R "[This room's type is set as '%s%s" C_R "'.]\r\n"
                  "[Must swim: %s. Underwater: %s. Avoid: %s.]\r\n"
                  " Syntax: room type [colour] [swim] [underwater] [avoid]\r\n"
                  "     Or: room type list\r\n"
                  " Example: room type bright-cyan swim (for a water room)\r\n" C_0,
                  type->color ? type->color : C_R, type->name,
                  type->must_swim ? "yes" : "no",
                  type->underwater ? "yes" : "no",
                  type->avoid ? "yes" : "no" );
        return;
     }
   
   p = get_string( arg, cmd, 256 );
   while ( cmd[0] )
     {
        color = get_color( cmd );
        
        if ( !strcmp( cmd, "list" ) )
          {
             clientfr( "Room types known:" );
             
             clientff( "   %-20s Underwater  Swim Avoid\r\n", "Name" );
             for ( t = room_types; t; t = t->next )
               {
                  if ( t->color )
                    clientff( " - %s%-25s" C_0 "   %s   %s   %s\r\n",
                              t->color, t->name,
                              t->underwater ? C_B "yes" C_0 : C_R " no" C_0,
                              t->must_swim ? C_B "yes" C_0 : C_R " no" C_0,
                              t->avoid ? C_B "yes" C_0 : C_R " no" C_0 );
                  else
                    clientff( " - " C_R "%-34s" C_D "[[unset]]" C_0 "\r\n", t->name );
               }
          }
        else if ( !strcmp( cmd, "swim" ) || !strcmp( cmd, "water" ) )
          {
             type->must_swim = type->must_swim ? 0 : 1;
             clientff( C_R "[Environment '%s' will %s require swimming.]\r\n" C_0,
                       type->name, type->must_swim ? "now" : C_r "no longer" C_R );
          }
        else if ( !strcmp( cmd, "underwater" ) )
          {
             type->underwater = type->underwater ? 0 : 1;
             clientff( C_R "[Environment '%s' is %s set as underwater.]\r\n" C_0,
                       type->name, type->underwater ? "now" : C_r "no longer" C_R );
          }
        else if ( !strcmp( cmd, "avoid" ) )
          {
             type->avoid = type->avoid ? 0 : 1;
             clientff( C_R "[Environment '%s' will %s be avoided when possible.]\r\n" C_0,
                       type->name, type->avoid ? "now" : C_r "no longer" C_R );
          }
        else if ( color )
          {
             type->color = color;
             clientff( C_R "[Environment '%s' will be shown as %s%s" C_R ".]\r\n" C_0,
                       type->name, color, cmd );
          }
        else
          {
             clientff( C_R "[I don't know what %s means.]\r\n" C_0, cmd );
             clientfr( "If you tried to set a colour, check 'map colour' for a list!" );
          }
        
        p = get_string( p, cmd, 256 );
     }
}



void do_room_merge( char *arg )
{
   ROOM_DATA *r, *old_room, *new_room;
   EXIT_DATA *spexit;
   REVERSE_EXIT *rexit;
   char buf[4096];
   int found;
   int i;
   
   if ( !current_room )
     {
        clientfr( "No current room set." );
        return;
     }
   
   if ( !arg[0] )
     {
        /* Show this room... Just for comparison purposes. */
        int first = 1;
        
        clientfr( "This room:" );
        
        buf[0] = 0;
        for ( i = 1; dir_name[i]; i++ )
          {
             if ( current_room->exits[i] || current_room->detected_exits[i] )
               {
                  if ( first )
                    {
                       strcat( buf, C_D "(" );
                       first = 0;
                    }
                  else
                    strcat( buf, C_D "," );
                  
                  if ( current_room->exits[i] )
                    strcat( buf, C_B );
                  else
                    strcat( buf, C_R );
                  
                  strcat( buf, dir_small_name[i] );
               }
          }
        if ( !first )
          strcat( buf, C_D ")" C_0 );
        else
          strcat( buf, C_D "(" C_r "no exits" C_D ")" C_0 );
        
        clientff( "  %s -- %s " C_D "(" C_G "%d" C_D ")\r\n" C_0,
                             buf, current_room->name, current_room->vnum );
        
        
        /* List all rooms that can be merged into this one. */
        
        clientfr( "Other rooms:" );
        found = 0;
        
        for ( r = world; r; r = r->next_in_world )
          if ( !strcmp( current_room->name, r->name ) && r != current_room )
            {
               int good = 1;
               
               for ( i = 1; dir_name[i]; i++ )
                 {
                    int e1 = r->exits[i] ? 1 : r->detected_exits[i];
                    int e2 = current_room->exits[i] ? 1 : current_room->detected_exits[i];
                    
                    if ( e1 != e2 )
                      {
                         good = 0;
                         break;
                      }
                 }
               
               /* Print it away. */
               if ( good )
                 {
                    int first = 1;
                    
                    found = 1;
                    
                    buf[0] = 0;
                    for ( i = 1; dir_name[i]; i++ )
                      {
                         if ( r->exits[i] || r->detected_exits[i] )
                           {
                              if ( first )
                                {
                                   strcat( buf, C_D "(" );
                                   first = 0;
                                }
                              else
                                strcat( buf, C_D "," );
                              
                              if ( r->exits[i] )
                                strcat( buf, C_B );
                              else
                                strcat( buf, C_R );
                              
                              strcat( buf, dir_small_name[i] );
                           }
                      }
                    if ( !first )
                      strcat( buf, C_D ")" C_0 );
                    else
                      strcat( buf, C_D "(" C_r "no exits" C_D ")" C_0 );
                    
                    clientff( "  %s -- %s " C_D "(" C_G "%d" C_D ")\r\n" C_0,
                             buf, r->name, r->vnum );
                 }
            }
        
        if ( !found )
          clientf( "  No identical matches found.\r\n" );
     }
   else
     {
        int vnum;
        
        old_room = current_room;
        
        if ( mode != CREATING )
          {
             clientfr( "Turn mapping on, first." );
             return;
          }
        
        vnum = atoi( arg );
        if ( !vnum || !( new_room = get_room( vnum ) ) )
          {
             clientfr( "Merge with which vnum?" );
             return;
          }
        
        if ( new_room == old_room )
          {
             clientfr( "You silly thing... What's the point in merging with the same room?" );
             return;
          }
        
        if ( strcmp( new_room->name, old_room->name ) )
          {
             clientfr( "That room doesn't even have the same name!" );
             return;
          }
        
        /* Check for exits leading to other places. */
        for ( i = 1; dir_name[i]; i++ )
          {
             if ( new_room->exits[i] && old_room->exits[i] &&
                  new_room->exits[i] != old_room->exits[i] )
               {
                  clientff( C_R "[Problem with the %s exits; they lead to different places.]\r\n" C_0, dir_name[i] );
                  return;
               }
             
             if ( ( new_room->exits[i] || new_room->detected_exits[i] ) !=
                  ( old_room->exits[i] || old_room->detected_exits[i] ) )
               {
                  clientff( C_R "[Problem with the '%s' exit; one room has it, the other doesn't.]\r\n" C_0, dir_name[i] );
                  return;
               }
          }
        
        /* Start merging old_room into new_room. */
        
        /* Exits from old_room, will now be from new_room. */
        for ( i = 1; dir_name[i]; i++ )
          {
             if ( old_room->exits[i] )
               {
                  if ( new_room->exits[i] )
                    unlink_rooms( new_room, i, new_room->exits[i] );
                  
                  link_rooms( new_room, i, old_room->exits[i] );
                  unlink_rooms( old_room, i, old_room->exits[i] );
                  
                  new_room->detected_exits[i] |= old_room->detected_exits[i];
                  new_room->locked_exits[i] |= old_room->locked_exits[i];
                  new_room->exit_joins_areas[i] |= old_room->exit_joins_areas[i];
                  
                  if ( !new_room->exit_length[i] )
                    new_room->exit_length[i] = old_room->exit_length[i];
                  if ( !new_room->use_exit_instead[i] )
                    new_room->use_exit_instead[i] = old_room->use_exit_instead[i];
                  new_room->exit_stops_mapping[i] = old_room->exit_stops_mapping[i];
               }
          }
        /* Move special exits too. */
        while ( old_room->special_exits )
          {
             spexit = create_exit( new_room );
             spexit->to         = old_room->special_exits->to;
             spexit->vnum       = old_room->special_exits->vnum;
             spexit->alias      = old_room->special_exits->alias;
             if ( spexit->command )
               spexit->command  = strdup( old_room->special_exits->command );
             if ( spexit->message )
               spexit->message  = strdup( old_room->special_exits->message );
             if ( spexit->to )
               link_special_exit( new_room, spexit, spexit->to );
             
             destroy_exit( old_room->special_exits );
          }
        
        /* Exits going INTO old_room, should go into new_room. */
        while ( old_room->rev_exits )
          {
             rexit = old_room->rev_exits;
             
             if ( rexit->direction )
               {
                  r = rexit->from, i = rexit->direction;
                  unlink_rooms( r, i, old_room );
                  link_rooms( r, i, new_room );
               }
             else if ( rexit->spexit )
               {
                  r = rexit->from, spexit = rexit->spexit;
                  unlink_special_exit( r, spexit, old_room );
                  spexit->to = new_room;
                  spexit->vnum = new_room->vnum;
                  link_special_exit( r, spexit, new_room );
               }
          }
        
        new_room->underwater |= old_room->underwater;
        
        /* Add all missing tags. */
        while ( old_room->tags )
          {
             ELEMENT *tag;
             
             /* Look if it already exists. */
             for ( tag = new_room->tags; tag; tag = tag->next )
               if ( !strcmp( (char *) old_room->tags->p, (char *) tag->p ) )
                 break;
             if ( !tag )
               {
                  tag = calloc( 1, sizeof( ELEMENT ) );
                  tag->p = old_room->tags->p;
                  link_element( tag, &new_room->tags );
               }
             else
               free( old_room->tags->p );
             
             unlink_element( old_room->tags );
          }
        
        clientff( C_R "[Rooms " C_G "%d" C_R " and " C_G "%d" C_R " merged into " C_G "%d" C_R ".]\r\n" C_0,
                  new_room->vnum, old_room->vnum, new_room->vnum );
        destroy_room( old_room );
        
        current_room = new_room;
        current_area = new_room->area;
        
        refind_last_vnum( );
     }
}




void do_room_tag( char *arg )
{
   ROOM_DATA *room;
   ELEMENT *tag;
   char buf[256];
   int vnum;
   
   arg = get_string( arg, buf, 256 );
   
   if ( ( vnum = atoi( buf ) ) )
     {
        room = get_room( vnum );
        if ( !room )
          {
             clientfr( "No room with that vnum found." );
             return;
          }
        
        arg = get_string( arg, buf, 256 );
     }
   else if ( !( room = current_room ) )
     {
        clientfr( "No current room set." );
        return;
     }
   
   if ( !buf[0] )
     {
        clientfr( "Usage: room tag [vnum] <tagname>" );
        return;
     }
   
   /* Look if it already exists. */
   for ( tag = room->tags; tag; tag = tag->next )
     if ( !strcmp( buf, (char *) tag->p ) )
       break;
   
   if ( tag )
     {
        /* Unlink it. */
        free( tag->p );
        unlink_element( tag );
     }
   else
     {
        /* Link it. */
        tag = calloc( 1, sizeof( ELEMENT ) );
        tag->p = strdup( buf );
        link_element( tag, &room->tags );
     }
   
   clientfr( room->name );
   clientff( C_R "[Tags in v" C_G "%d" C_R ": ", room->vnum );
   if ( room->tags )
     {
        clientf( (char *) room->tags->p );
        for ( tag = room->tags->next; tag; tag = tag->next )
          clientff( ", %s", (char *) tag->p );
     }
   else
     clientf( C_D "(none)" C_R );
   clientf( ".]\r\n" C_0 );
   
   save_settings( "config.mapper.txt" );
}



void do_room_mark( char *arg )
{
   char buf[256];
   char buf2[256];
   
   get_string( arg, buf, 256 );
   
   if ( isdigit( buf[0] ) )
     sprintf( buf2, "%s mark", buf );
   else
     sprintf( buf2, "mark" );
   
   do_room_tag( buf2 );
   
   clientfr( "Using 'map config save' will make it permanent." );
}




/* Exit commands. */

void do_exit_help( char *arg )
{
   clientfr( "Module: IMapper. Room exit commands:" );
   clientf( " exit link      - Link room # to this one. Both ways.\r\n"
            " exit stop      - Hide rooms beyond this exit.\r\n"
            " exit length    - Increase the exit length by #.\r\n"
            " exit map       - Map the next exit elsewhere.\r\n"
            " exit lock      - Set all unlinked exits in room as locked.\r\n"
            " exit unilink   - One-way exit. Does not create a reverse link.\r\n"
            " exit destroy   - Destroy an exit, and its reverse.\r\n"
            " exit joinareas - Show two areas on a single 'map'.\r\n"
            " exit special   - Create, destroy, modify or list special exits.\r\n" );
}



void do_exit_length( char *arg )
{
   if ( mode != CREATING )
     {
        clientfr( "Turn mapping on, first." );
        return;
     }
   
   if ( !isdigit( *(arg) ) )
     clientfr( "Specify a length. 0 is normal." );
   else
     {
        set_length_to = atoi( arg );
        
        if ( set_length_to == 0 )
          set_length_to = -1;
        
        clientfr( "Move in the direction you wish to have this length." );
     }
}



void do_exit_stop( char *arg )
{
   if ( mode != CREATING )
     {
        clientfr( "Turn mapping on, first." );
        return;
     }
   
   clientfr( "Move in the direction you wish to stop (or start again) mapping from." );
   switch_exit_stops_mapping = 1;
}



void do_exit_joinareas( char *arg )
{
   if ( mode != CREATING )
     {
        clientfr( "Turn mapping on, first." );
        return;
     }
   
   clientfr( "Move into the other area." );
   switch_exit_joins_areas = 1;
}



void do_exit_map( char *arg )
{
   char buf[256];
   int i;
   
   if ( mode != CREATING )
     {
        clientfr( "Turn mapping on, first." );
        return;
     }
   
   use_direction_instead = 0;
   
   for ( i = 1; dir_name[i]; i++ )
     {
        if ( !strcmp( arg, dir_small_name[i] ) )
          use_direction_instead = i;
     }
   
   if ( !use_direction_instead )
     {
        use_direction_instead = -1;
        clientfr( "Will use default exit, as map position." );
     }
   else
     {
        sprintf( buf, "The room will be mapped '%s' from here, instead.",
                 dir_name[use_direction_instead] );
        clientfr( buf );
     }
}



void do_exit_link( char *arg )
{
   if ( mode != CREATING )
     {
        clientfr( "Turn mapping on, first." );
        return;
     }
   
   if ( !isdigit( *(arg) ) )
     {
        clientfr( "Specify a vnum to link to." );
        return;
     }
   
   link_next_to = get_room( atoi( arg ) );
   
   unidirectional_exit = 0;
   
   if ( !link_next_to )
     clientfr( "Disabled." );
   else
     clientfr( "Move in the direction the room is." );
}



void do_exit_unilink( char *arg )
{
   if ( mode != CREATING )
     {
        clientfr( "Turn mapping on, first." );
        return;
     }
   
   if ( arg[0] )
     {
        if ( !isdigit( *arg ) )
          {
             clientfr( "Specify a vnum to link to." );
             return;
          }
        
        link_next_to = get_room( atoi( arg ) );
        
        if ( !link_next_to )
          {
             clientfr( "That room does not exist." );
             return;
          }
     }
   
   unidirectional_exit = 1;
   
   clientfr( "Move in the direction you want to create an unidirectional exit towards." );
}



void do_exit_lock( char *arg )
{
   int i, set, nr;
   
   if ( mode != CREATING )
     {
        clientfr( "Turn mapping on, first." );
        return;
     }
   
   if ( !current_room )
     {
        clientfr( "No current room set." );
        return;
     }
   
   if ( arg[0] == 'c' )
     set = 0;
   else
     set = 1;
   
   for ( i = 1, nr = 0; dir_name[i]; i++ )
     {
        if ( current_room->detected_exits[i] &&
             !current_room->exits[i] )
          {
             if ( !nr++ )
               clientff( C_R "[Marked as %slocked: ", set ? "" : "un" );
             else
               clientff( ", " );
             
             clientff( C_r "%s" C_R, dir_name[i] );
             current_room->locked_exits[i] = set;
          }
     }
   
   if ( nr )
     clientff( ".]\r\n" C_0 );
   else
     clientfr( "There are no unlinked exits to mark here." );
}



void do_exit_destroy( char *arg )
{
   EXIT_DATA *spexit;
   char buf[256];
   int dir = 0;
   int i;
   
   if ( mode != CREATING )
     {
        clientfr( "Turn mapping on, first." );
        return;
     }
   
   if ( !current_room )
     {
        clientfr( "No current room set." );
        return;
     }
   
   /* Destroy a normal exit? */
   for ( i = 1; dir_name[i]; i++ )
     {
        if ( !strcmp( arg, dir_name[i] ) )
          dir = i;
     }
   
   if ( dir )
     {
        if ( !current_room->exits[dir] )
          {
             clientfr( "No link in that direction." );
             return;
          }
        
        if ( current_room->exits[dir]->exits[reverse_exit[dir]] != current_room )
          {
             clientfr( "Warning: Reverse link was not destroyed." );
          }
        else
          {
             unlink_rooms( current_room->exits[dir], reverse_exit[dir], current_room );
          }
        
        i = current_room->exits[dir]->vnum;
        
        unlink_rooms( current_room, dir, current_room->exits[dir] );
        
        sprintf( buf, "Link to vnum %d destroyed.", i );
        clientfr( buf );
        return;
     }
   
   /* Destroy a special exit? */
   if ( arg[0] && isdigit( arg[0] ) )
     {
        int nr;
        
        nr = atoi( arg );
        
        for ( i = 1, spexit = current_room->special_exits; spexit; spexit = spexit->next, i++ )
          {
             if ( i == nr )
               {
                  destroy_exit( spexit );
                  sprintf( buf, "Special exit %d destroyed.", nr );
                  clientfr( buf );
                  return;
               }
          }
        
        sprintf( buf, "Special exit %d was not found.", nr );
        clientfr( buf );
        
        return;
     }
   
   clientfr( "Which link do you wish to destroy?" );
   clientfr( "Use 'room look' for a list." );
   return;
}



// Add "alias", <create/link <vnum>>, "nocommand"
// Real: exit special <create/nolink/link <vnum>> [nocommand] [alias <dir>]
// FIXME: Convert [1,n]!
void do_exit_special( char *arg )
{
   ROOM_DATA *room;
   char cmd[256];
   
   DEBUG( "do_exit_special" );
   
   if ( mode != CREATING )
     {
        clientfr( "Turn mapping on, first." );
        return;
     }
   
   if ( !arg[0] )
     {
        clientfr( "Syntax: exit special <command> [arguments]" );
        clientfr( "Commands:" );
        clientff( C_R " create      - On the other side, create a new room.\r\n" C_0 );
        clientff( C_R " link <vnum> - On the other side, link with <vnum>.\r\n" C_0 );
        clientff( C_R " nolink      - On the other side, is always something random.\r\n" C_0 );
        clientfr( "Optional arguments:" );
        clientff( C_R " nocommand   - The exit is not triggered by a command.\r\n" C_0 );
        clientff( C_R " alias <dir> - Create an alias for this exit's command.\r\n" C_0 );
        
        return;
     }
   
   if ( !current_room )
     {
        clientfr( "No current room set." );
        return;
     }
   
   if ( capture_special_exit )
     {
        clientfr( "Already capturing, use 'stop' first." );
        return;
     }
   
   arg = get_string( arg, cmd, 256 );
   
   if ( !strcmp( cmd, "create" ) )
     {
        special_exit_vnum = 0;
     }
   else if ( !strcmp( cmd, "nolink" ) )
     {
        special_exit_vnum = -1;
     }
   else if ( !strcmp( cmd, "link" ) )
     {
        int vnum;
        
        arg = get_string( arg, cmd, 256 );
        vnum = atoi( cmd );
        
        if ( vnum < 1 )
          {
             clientfr( "Specify a valid vnum to link the exit to." );
             return;
          }
        if ( !( room = get_room( vnum ) ) )
          {
             clientfr( "No room with that vnum exists!" );
             return;
          }
        
        special_exit_vnum = vnum;
     }
   
   special_exit_nocommand = 0;
   special_exit_alias = 0;
   
   while ( arg[0] )
     {
        arg = get_string( arg, cmd, 256 );
        
        if ( !strcmp( cmd, "nocommand" ) )
          special_exit_nocommand = 1;
        else if ( !strcmp( cmd, "alias" ) )
          {
             int i;
             
             arg = get_string( arg, cmd, 256 );
             
             for ( i = 0; dir_name[i]; i++ )
               if ( !strcmp( cmd, dir_name[i] ) )
                 break;
             
             if ( !cmd[0] || !dir_name[i] )
               {
                  clientfr( "Aliases can only be north, up, in, etc." );
                  return;
               }
             
             special_exit_alias = i;
          }
        else if ( !strcmp( cmd, "tag" ) )
          {
             arg = get_string( arg, cmd, 256 );
             
             if ( special_exit_tag )
               free( special_exit_tag );
             special_exit_tag = strdup( cmd );
          }
        else
          {
             clientff( C_R "[Unknown option '%s'.]\r\n" C_0, cmd );
             return;
          }
     }
   
   capture_special_exit = 1;
   cse_message[0] = 0;
   cse_command[0] = 0;
   
   clientfr( "Capturing. Use 'stop' to disable." );
   if ( special_exit_vnum > 0 )
     clientff( C_R "[The link will be made with '%s'.]\r\n" C_0, room->name );
   
   return;
   
#if 0
   // ---================ *** =================---
   if ( !strcmp( cmd, "capture" ) )
     {
        arg = get_string( arg, cmd, 256 );
        
        if ( !cmd[0] )
          {
             clientfr( "Syntax: Esp capture create" );
             clientfr( "        Esp capture link <vnum>" );
             return;
          }
        
        if ( !strcmp( cmd, "create" ) )
          {
             capture_special_exit = -1;
             
             clientfr( "Capturing. A room will be created on the other end." );
             clientfr( "Use 'stop' to disable capturing." );
          }
        else if ( !strcmp( cmd, "link" ) )
          {
             ROOM_DATA *room;
             int vnum;
             
             get_string( arg, cmd, 256 );
             vnum = atoi( cmd );
             
             if ( vnum < 1 )
               {
                  clientfr( "Specify a valid vnum to link the exit to." );
                  return;
               }
             if ( !( room = get_room( vnum ) ) )
               {
                  clientfr( "No room with that vnum exists!" );
                  return;
               }
             
             capture_special_exit = vnum;
             
             clientff( C_R "[Capturing. The exit will be linked to '%s']\r\n" C_0, room->name );
             clientfr( "Use 'stop' to disable capturing." );
          }
        
        cse_command[0] = 0;
        cse_message[0] = 0;
        
        return;
     }
   
   // FIXME: How can we create -1 vnum exits?
   // Doesn't conform to new reverse exits.
   else if ( !strcmp( cmd, "link" ) )
     {
        /* E special link 0 -1 */
        char vnum[256];
        
        /* We'll store the number in cmd. */
        arg = get_string( arg, cmd, 256 );
        
        if ( !isdigit( cmd[0] ) )
          {
             clientfr( "Link which special exit?" );
             return;
          }
        
        nr = atoi( cmd );
        
        for ( i = 0, spexit = current_room->special_exits; spexit; spexit = spexit->next )
          {
             if ( i++ == nr )
               {
                  get_string( arg, vnum, 256 );
                  
                  if ( vnum[0] == '-' )
                    {
                       ROOM_DATA *to;
                       
                       to = spexit->to;
                       
                       spexit->vnum = -1;
                       spexit->to = NULL;
                       
                       sprintf( buf, "Link cleared on exit %d.", nr );
                       clientfr( buf );
                       return;
                    }
                  else if ( isdigit( vnum[0] ) )
                    {
                       spexit->vnum = atoi( vnum );
                       spexit->to = get_room( spexit->vnum );
                       if ( !spexit->to )
                         {
                            clientfr( "A room whith that vnum was not found." );
                            spexit->vnum = -1;
                            return;
                         }
                       sprintf( buf, "Special exit %d linked to '%s'.",
                                nr, spexit->to->name );
                       clientfr( buf );
                       return;
                    }
                  else
                    {
                       clientfr( "Link to which vnum?" );
                       return;
                    }
               }
          }
        
        sprintf( buf, "Can't find special exit %d.", nr );
        clientfr( buf );
     }
   
   else if ( !strcmp( cmd, "alias" ) )
     {
        /* E special alias 0 down */
        
        if ( !arg[0] || !isdigit( arg[0] ) )
          {
             clientfr( "What special exit do you wish to destroy?" );
             return;
          }
        
        nr = atoi( arg );
        
        for ( i = 0, spexit = current_room->special_exits; spexit; spexit = spexit->next )
          {
             if ( i++ == nr )
               {
                  arg = get_string( arg, cmd, 256 );
                  
                  if ( !strcmp( arg, "none" ) )
                    {
                       sprintf( buf, "Alias on exit %d cleared.", nr );
                       clientfr( buf );
                       return;
                    }
                  
                  for ( i = 1; dir_name[i]; i++ )
                    {
                       if ( !strcmp( arg, dir_name[i] ) )
                         {
                            sprintf( buf, "Going %s will now trigger exit %d.",
                                     dir_name[i], nr );
                            clientfr( buf );
                            spexit->alias = i;
                            return;
                         }
                    }
                  
                  clientfr( "Use an exit, such as 'north', 'up', 'none', etc." );
                  return;
               }
          }
        
        sprintf( buf, "Special exit %d was not found.", nr );
        clientfr( buf );
     }
   
   else
     {
        clientfr( "Unknown command... Try 'exit special help'." );
     }
#endif
}




/* Normal commands. */

void do_landmarks( char *arg )
{
   AREA_DATA *area;
   ROOM_DATA *room;
   ELEMENT *tag;
   char buf[256];
   char name[256];
   int first;
   int found = 0;
   
   get_string( arg, name, 256 );
   
   if ( !name[0] )
     clientfr( "Landmarks throughout the world:" );
   else
     clientfr( "Tagged rooms:" );
   
   for ( area = areas; area; area = area->next )
     {
        first = 1;
        
        for ( room = area->rooms; room; room = room->next_in_area )
          {
             for ( tag = room->tags; tag; tag = tag->next )
               if ( !case_strcmp( (char *) tag->p, name[0] ? name : "mark" ) )
                 break;
             
             if ( !tag )
               continue;
             
             if ( !found )
               found = 1;
             
             /* And now... */
             
             /* Area name, if first room in the area's list. */
             if ( first )
               {
                  sprintf( buf, "\r\n" C_B "%s" C_0 "\r\n", area->name );
                  clientf( buf );
                  first = 0;
               }
             
             sprintf( buf, C_D " (" C_G "%d" C_D ") " C_0 "%s\r\n",
                      room->vnum, room->name );
             clientf( buf );
          }
     }
   
   if ( !found )
     {
        if ( !name[0] )
          clientf( "None defined, use the 'room mark' command to add some." );
        else
          clientf( "None found, check your spelling or add some with 'room tag'." );
     }
   
   clientf( "\r\n" );
}



void print_mhelp_line( char *line )
{
   char buf[8096];
   char *p, *b, *c;
   
   p = line;
   b = buf;
   
   while ( *p )
     {
        if ( *p != '^' )
          {
             *(b++) = *(p++);
             continue;
          }
        
        p++;
        switch ( *p )
          {
           case 'r':
             c = "\33[0;31m"; break;
           case 'g':
             c = "\33[0;32m"; break;
           case 'y':
             c = "\33[0;33m"; break;
           case 'b':
             c = "\33[0;34m"; break;
           case 'm':
             c = "\33[0;35m"; break;
           case 'c':
             c = "\33[0;36m"; break;
           case 'w':
             c = "\33[0;37m"; break;
           case 'D':
             c = "\33[1;30m"; break;
           case 'R':
             c = "\33[1;31m"; break;
           case 'G':
             c = "\33[1;32m"; break;
           case 'Y':
             c = "\33[1;33m"; break;
           case 'B':
             c = "\33[1;34m"; break;
           case 'M':
             c = "\33[1;35m"; break;
           case 'C':
             c = "\33[1;36m"; break;
           case 'W':
             c = "\33[1;37m"; break;
           case 'x':
             c = "\33[0;37m"; break;
           case '^':
             c = "^"; break;
           case 0:
             c = NULL;
             break;
           default:
             c = "-?-";
          }
        
        if ( !c )
          break;
        
        while ( *c )
          *(b++) = *(c++);
        p++;
     }
   
   *b = 0;
   
   clientf( buf );
}



void do_mhelp( char *arg )
{
   FILE *fl;
   char buf[4096];
   char name[256];
   char *p;
   int found = 0;
   int empty_lines = 0;
   
   if ( !arg[0] )
     {
        clientf( "Use 'mhelp index' for a list of mhelp files.\r\n" );
        return;
     }
   
   fl = fopen( "mhelp", "r" );
   if ( !fl )
     fl = fopen( "mhelp.txt", "r" );
   
   if ( !fl )
     {
        clientf( C_R "[" );
        clientff( "Unable to open mhelp: %s.", strerror( errno ) );
        clientf( "]\r\n" C_0 );
        return;
     }
   
   while ( 1 )
     {
        if ( !fgets( buf, 4096, fl ) )
          break;
        
        /* Comments... Ignored. */
        if ( buf[0] == '#' )
          continue;
        
        /* Strip newline. */     
        p = buf;
        while ( *p != '\n' && *p != '\r' && *p )
          p++;
        *p = 0;
        
        /* Names. */
        if ( buf[0] == ':' )
          {
             if ( found )
               break;
             
             p = buf + 1;
             
             while ( *p )
               {
                  p = get_string( p, name, 256 );
                  if ( !case_strcmp( arg, name ) )
                    {
                       /* This is it. Start printing. */
                       
                       found = 2;
                       empty_lines = 0;
                       
                       get_string( buf+1, name, 256 );
                       clientff( C_C "MAPPER HELP" C_0 " - " C_W "%s" C_0 "\r\n\r\n", name );
                       break;
                    }
               }
             
             continue;
          }
        
        /* One line that must be displayed. */
        if ( found )
          {
             /* Remember empty lines, print them later. */
             /* Helps to skip them at the beginning and at the end. */
             if ( !buf[0] )
               {
                  empty_lines++;
                  continue;
               }
             
             if ( found == 2 )
               empty_lines = 0, found = 1;
             else
               if ( empty_lines > 0 )
                 while ( empty_lines )
                   {
                      clientf( "\r\n" );
                      empty_lines--;
                   }
             
             print_mhelp_line( buf );
             clientf( "\r\n" );
          }
     }
   
   fclose( fl );
   
   if ( !found )
     clientfr( "No help file by that name." );
}



void do_old_mhelp( char *arg )
{
   if ( !strcmp( arg, "index" ) )
     {
        clientf( C_C "MAPPER HELP" C_0 "\r\n\r\n" );
        
        clientf( "- Index\r\n"
                 "- GettingStarted\r\n"
                 "- Commands\r\n" );
     }
   
   else if ( !strcmp( arg, "gettingstarted" ) )
     {
        clientf( C_C "MAPPER HELP" C_0 " - Getting started.\r\n\r\n"
                 "Greetings.  If you're reading this, then you obviously have a new\r\n"
                 "mapper to play with.\r\n"
                 "\r\n"
                 "First thing you should do, is to check if it's working.  Do so with a\r\n"
                 "simple 'look' command, in a common room.  If the area name is not shown\r\n"
                 "after the room title, then you must set the room title color.  Do so\r\n"
                 "with 'map color'.  When the color matches the one set on Imperian, and\r\n"
                 "the room you're currently in is mapped, all should work.\r\n"
                 "\r\n"
                 "If the area name was shown, that means it now knows where it is, so\r\n"
                 "the 'map' command is available.\r\n"
                 "\r\n"
                 "You will then most likely want to make a path to somewhere, and walk\r\n"
                 "to it.  Do so by choosing a room to go to, and find its vnum (room's\r\n"
                 "virtual number).  As an example, if you'd want to go to the Shuk, in\r\n"
                 "Antioch, find its vnum with 'room find shuk'.  In this case, the vnum\r\n"
                 "is 605, so a path to it would be made by 'map path 605'.  Once the\r\n"
                 "path is built, use 'go' to start auto-walking, and 'stop' in the\r\n"
                 "middle of an auto-walking to stop it.\r\n"
                 "\r\n"
                 "These are the basics of showing a map, and going somewhere.\r\n" );
     }
   
   else if ( !strcmp( arg, "commands" ) )
     {
        clientf( C_C "MAPPER HELP" C_0 " - Commands.\r\n\r\n"
                 "Basics\r\n"
                 "------\r\n"
                 "\r\n"
                 "The mapper has four base commands, each having some subcommands.  The\r\n"
                 "base commands are 'map', 'area', 'room' and 'exit'.  Each base\r\n"
                 "command has the subcommand 'help', which lists all other subcommands.\r\n"
                 "An example would be 'map help'.\r\n"
                 "\r\n"
                 "Abbreviating\r\n"
                 "------------\r\n"
                 "\r\n"
                 "As an example, use 'area list'.  Base command is 'area', subcommand is\r\n"
                 "'list'.  The subcommand may be abbreviated to any number of letters,\r\n"
                 "so 'area lis', 'area li' and 'area l' all work.  The base command may\r\n"
                 "be abbreviated only to the first letter, 'a l'.  Also, as typing a\r\n"
                 "double space ('m p 605') is sometimes hard, the base command letter\r\n"
                 "may be used as uppercase, with no space after it.  As an example,\r\n"
                 "'Mp 605' instead of 'map path 605' or 'm p 605' would work.\r\n" );
     }
   
   else
     clientf( "Use 'mhelp index', for a list of mhelp files.\r\n" );
}




#define CMD_NONE        0
#define CMD_MAP         1
#define CMD_AREA        2
#define CMD_ROOM        3
#define CMD_EXIT        4

FUNC_DATA cmd_table[] =
{
   /* Map commands. */
     { "help",          do_map_help,    CMD_MAP },
     { "create",        do_map_create,  CMD_MAP },
     { "follow",        do_map_follow,  CMD_MAP },
     { "none",          do_map_none,    CMD_MAP },
     { "save",          do_map_save,    CMD_MAP },
     { "load",          do_map_load,    CMD_MAP },
     { "path",          do_map_path,    CMD_MAP },
     { "status",        do_map_status,  CMD_MAP },
     { "color",         do_map_color,   CMD_MAP },
     { "colour",        do_map_color,   CMD_MAP },
     { "bump",          do_map_bump,    CMD_MAP },
     { "old",           do_map_old,     CMD_MAP },
     { "file",          do_map_file,    CMD_MAP },
     { "teleport",      do_map_teleport,CMD_MAP },
     { "queue",         do_map_queue,   CMD_MAP },
     { "config",        do_map_config,  CMD_MAP },
     { "window",        do_map_window,  CMD_MAP },
   
   /* Area commands. */
     { "help",          do_area_help,   CMD_AREA },
     { "list",          do_area_list,   CMD_AREA },
     { "switch",        do_area_switch, CMD_AREA },
     { "check",         do_area_check,  CMD_AREA },
     { "update",        do_area_update, CMD_AREA },
     { "off",           do_area_off,    CMD_AREA },
     { "orig",          do_area_orig,   CMD_AREA },
     { "orig",          do_map_orig,    CMD_MAP },
   
   /* Room commands. */
     { "help",          do_room_help,   CMD_ROOM },
     { "switch",        do_room_switch, CMD_ROOM },
     { "find",          do_room_find,   CMD_ROOM },
     { "look",          do_room_look,   CMD_ROOM },
     { "destroy",       do_room_destroy,CMD_ROOM },
     { "list",          do_room_list,   CMD_ROOM },
     { "underwater",    do_room_underw, CMD_ROOM },
     { "merge",         do_room_merge,  CMD_ROOM },
     { "tag",           do_room_tag,    CMD_ROOM },
     { "mark",          do_room_mark,   CMD_ROOM },
     { "types",         do_room_types,  CMD_ROOM },
   
   /* Exit commands. */
     { "help",          do_exit_help,   CMD_EXIT },
     { "length",        do_exit_length, CMD_EXIT },
     { "stop",          do_exit_stop,   CMD_EXIT },
     { "map",           do_exit_map,    CMD_EXIT },
     { "link",          do_exit_link,   CMD_EXIT },
     { "unilink",       do_exit_unilink,CMD_EXIT },
     { "lock",          do_exit_lock,   CMD_EXIT },
     { "destroy",       do_exit_destroy,CMD_EXIT },
     { "joinareas",     do_exit_joinareas,CMD_EXIT },
     { "special",       do_exit_special,CMD_EXIT },
   
   /* Normal commands. */
     { "map",           do_map,         CMD_NONE },
     { "landmarks",     do_landmarks,   CMD_NONE },
     { "mhelp",         do_mhelp,       CMD_NONE },
   
     { NULL, NULL, 0 }
};


int i_mapper_process_client_aliases( char *line )
{
   int i;
   char command[4096], *l;
   int base_cmd;
   
   DEBUG( "i_mapper_process_client_aliases" );
   
   /* Room commands queue. */
   if ( !strcmp( line, "l" ) ||
        !strcmp( line, "look" ) ||
        !strcmp( line, "ql" ) )
     {
        /* Add -1 to queue. */
        if ( mode == CREATING || mode == FOLLOWING )
          add_queue( -1 );
        
        return 0;
     }
   
   /* Accept swimming. */
   if ( !strncmp( line, "swim ", 5 ) )
     line += 5;
   
   /* Same with Saboteur/Idrasi evading. */
   if ( !strncmp( line, "evade ", 6 ) )
     line += 6;
   
   /* And Idrasi wolfvaulting too. */
   if ( !strncmp( line, "wolfvault ", 10 ) )
     line += 10;
   
   for ( i = 1; dir_name[i]; i++ )
     {
        if ( !strcmp( line, dir_name[i] ) ||
             !strcmp( line, dir_small_name[i] ) )
          {
             if ( mode == FOLLOWING && current_room && current_room->special_exits )
               {
                  EXIT_DATA *spexit;
                  
                  for ( spexit = current_room->special_exits; spexit; spexit = spexit->next )
                    {
                       if ( spexit->alias && spexit->command && spexit->alias == i )
                         {
                            send_to_server( spexit->command );
                            send_to_server( "\r\n" );
                            return 1;
                         }
                    }
               }
             
             if ( mode == CREATING || mode == FOLLOWING )
               add_queue( i );
             return 0;
          }
     }
   
   /* Go? */
   if ( !strcmp( line, "go" ) || !strcmp( line, "go sprint" ) || !strcmp( line, "go dash" ) )
     {
        if ( q_top )
          {
             clientfr( "Command queue isn't empty, clear it first." );
             show_prompt( );
             return 1;
          }
        
        if ( !strcmp( line, "go dash" ) )
          dash_command = "dash ";
        else if ( !strcmp( line, "go sprint" ) )
          dash_command = "sprint ";
        else
          dash_command = NULL;
        
        /* Go! */
        go_next( );
        clientf( "\r\n" );
        return 1;
     }
   
   if ( !strcmp( line, "stop" ) )
     {
        if ( auto_walk != 0 )
          {
             clientfr( "Okay, I'm frozen." );
             auto_walk = 0;
             return 1;
          }
        else if ( capture_special_exit )
          {
             capture_special_exit = 0;
             clientfr( "Capturing disabled." );
             show_prompt( );
             return 1;
          }
     }
   
   /* Process from command table. */
   
   l = line;
   
   /* First, check for compound commands. */
   if ( !strncmp( l, "room ", 5 ) )
     base_cmd = CMD_ROOM, l += 5;
   else if ( !strncmp( l, "r ", 2 ) )
     base_cmd = CMD_ROOM, l += 2;
   else if ( !strncmp( l, "R", 1 ) )
     base_cmd = CMD_ROOM, l += 1;
   else if ( !strncmp( l, "area ", 5 ) )
     base_cmd = CMD_AREA, l += 5;
   else if ( !strncmp( l, "a ", 2 ) )
     base_cmd = CMD_AREA, l += 2;
   else if ( !strncmp( l, "A", 1 ) )
     base_cmd = CMD_AREA, l += 1;
   else if ( !strncmp( l, "map ", 4 ) )
     base_cmd = CMD_MAP, l += 4;
   else if ( !strncmp( l, "m ", 2 ) )
     base_cmd = CMD_MAP, l += 2;
   else if ( !strncmp( l, "M", 1 ) )
     base_cmd = CMD_MAP, l += 1;
   else if ( !strncmp( l, "exit ", 5 ) )
     base_cmd = CMD_EXIT, l += 5;
   else if ( !strncmp( l, "ex ", 3 ) )
     base_cmd = CMD_EXIT, l += 3;
   else if ( !strncmp( l, "E", 1 ) )
     base_cmd = CMD_EXIT, l += 1;
   else
     base_cmd = CMD_NONE;
   
   l = get_string( l, command, 4096 );
   
   if ( !command[0] )
     return 0;
   
   for ( i = 0; cmd_table[i].name; i++ )
     {
        /* If a normal command, compare it full.
         * If after a base command, it's okay to abbreviate it.. */
        if ( base_cmd == cmd_table[i].base_cmd &&
             ( ( base_cmd != CMD_NONE && !strncmp( command, cmd_table[i].name, strlen( command ) ) ) ||
               ( base_cmd == CMD_NONE && !strcmp( command, cmd_table[i].name ) ) ) )
          {
             (cmd_table[i].func)( l );
             show_prompt( );
             return 1;
          }
     }
   
   /* A little shortcut to 'exit link'. */
   if ( base_cmd == CMD_EXIT && isdigit( command[0] ) )
     {
        do_exit_link( command );
        show_prompt( );
        return 1;
     }
   if ( base_cmd == CMD_MAP && isdigit( command[0] ) )
     {
        do_map( command );
        show_prompt( );
        return 1;
     }
   
   if ( capture_special_exit && !special_exit_nocommand )
     {
        strcpy( cse_command, line );
        clientff( C_R "[Command changed to '%s'.]\r\n" C_0, cse_command );
     }
   
   return 0;
}

