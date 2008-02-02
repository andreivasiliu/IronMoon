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


/* Header file specific to Imperian Mapper. */

#define I_MAPPER_H_ID "$Name: HEAD $ $Id: i_mapper.h,v 1.1 2008/01/23 12:10:21 andreivasiliu Exp $"

typedef struct room_data ROOM_DATA;
typedef struct area_data AREA_DATA;
typedef struct exit_data EXIT_DATA;
typedef struct room_type_data ROOM_TYPE;
typedef struct function_data FUNC_DATA;
typedef struct color_data COLOR_DATA;
typedef struct map_element MAP_ELEMENT;
typedef struct element_data ELEMENT;
typedef struct reverse_exit_data REVERSE_EXIT;

/* Special exit structure. */
struct exit_data
{
   char *command; /* This command will trigger the exit. */
   char *message; /* Or, this message will trigger the exit. */
   
   int alias;     /* This will contain a direction. */
   
   /* And when it was triggered, it will take us here. */
   /* If vnum is -1, then set get_unlost. */
   int vnum;
   ROOM_DATA *to;
   
   /* A switch to turn it on and off. */
   ELEMENT *tag;
   
   /* Chain links. */
   EXIT_DATA *next;
   ROOM_DATA *owner;
};


struct reverse_exit_data
{
   /* An exit leads from this room, to here. */
   ROOM_DATA *from;
   
   /* Via this special exit.. (!= NULL) */
   EXIT_DATA *spexit;
   
   /* ..or this direction. (!= 0) */
   int direction;
   
   REVERSE_EXIT *next;
};


/* Room structure. */
struct room_data
{
   char *name;
   int vnum;
   
   ROOM_TYPE *room_type;
   
   short mapped;
   short landmark;
   short underwater;
   
   ROOM_DATA *next_in_world;
   ROOM_DATA *next_in_area;
   ROOM_DATA *next_in_hash;
   
   int next_direction;
   
   /* NULL, N, NE, E, SE, S, SW, W, NW, UP, DOWN, IN, OUT */
   ROOM_DATA *exits[13];
   
   REVERSE_EXIT *rev_exits;
   
   /* Beginning of binary-saved values. If you change these, change save_binary_map! */
   int vnum_exits[13];
   short detected_exits[13];
   short locked_exits[13];
   short exit_length[13];
   short use_exit_instead[13];
   short exit_stops_mapping[13];
   short exit_joins_areas[13];
   /* End. */
   
   ELEMENT *tags;
   
   EXIT_DATA *special_exits;
   AREA_DATA *area;
   
   /* Pathfinder. */
   int pf_cost;
   int pf_direction;
   int pf_highlight;
   ROOM_DATA *pf_parent;
   ROOM_DATA *next_in_pfno;
   ROOM_DATA *next_in_pfco;
   
   /* Pathfinder 2. */
   ROOM_DATA *pf_next;
   ROOM_DATA **pf_prev;
   
   /* Person in this room. */
   char *person_here;
};

/* Area structure. */
struct area_data
{
   char *name;
   
   int disabled;
   
   int needs_cleaning;
   
   ROOM_DATA *rooms;
   ROOM_DATA *last_room;
   
   AREA_DATA *next;
};


/* Exit flags. If more are added, change from 'short' to 'int'. */
#define EXIT_NORMAL	1
#define EXIT_STOPPING	2
#define EXIT_OTHER_AREA	4
#define EXIT_PATH	8
#define EXIT_UNLINKED  16
#define EXIT_LOCKED    32


/* One room on the map. */
struct map_element
{
   ROOM_DATA *room;
   
   char *color;
   short warn;
   
   short in_out;
   
   short e;		/* - */
   short s;		/* | */
   short se;		/* \ */  /* se AND se_rev: */
   short se_rev;	/* / */  /* X */
   
   short extra_e;
   short extra_s;
   short extra_se;
   short extra_se_rev;
};


/* Room type structure. */
struct room_type_data
{
   char *name;
   char *color;
   
   int must_swim;
   int underwater;
   int avoid;
   
   ROOM_TYPE *next;
};


/* Command table. */
struct function_data
{
   char *name;
   
   void (*func)( char *arg );
   
   int base_cmd;
};

/* Colors table. */
struct color_data
{
   char *name;
   char *code;
   
   char *title_code;
   int length;
};


/* A link in a chain. This can be anything. */

struct element_data
{
   ELEMENT *next;
   ELEMENT *prev;
   ELEMENT **first;
   
   /* One pointer value. */
   void *p;
   
   /* And one integer value. */
   int value;
};




#define MAX_HASH 512

//#define MAP_X 18
//#define MAP_Y 14
#define MAP_X 14
#define MAP_Y 10


#define EX_N	1
#define EX_NE	2
#define EX_E	3
#define EX_SE	4
#define EX_S	5
#define EX_SW	6
#define EX_W	7
#define EX_NW	8
#define EX_U	9
#define EX_D	10
#define EX_IN	11
#define EX_OUT	12

/* Map modes */
#define NONE		0
#define FOLLOWING	1
#define CREATING	2
#define GET_UNLOST	3
