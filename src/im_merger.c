/*
 * map.mode
 * 
 * 
 * Lua function-wrappers needed:
 * parse_title( str, [exits!] )
 * mode var
 * a room-type changer?
 */


#include <lua.h>

#include "module.h"
#include "i_mapper.h"


/* Declarations from i_mapper.c */
ROOM_DATA *get_room( int vnum );
void mapper_start( );
void destroy_map( );
void i_mapper_module_unload( );
int imapper_commands( const char *cmd );
int parse_title( const char *line );
int load_map( const char *file );
ROOM_DATA *locate_nearby_room( const char *title, int dir );
ROOM_DATA *locate_faraway_room( const char *title, int *detected_exits,
                                int *rooms_found );

extern ROOM_DATA *current_room;
extern AREA_DATA *current_area;
extern char *dir_name[];
extern char *dir_small_name[];
extern int mode;


/* Our own structures, used in userdata payloads. */
typedef struct m_exit_data
{
   ROOM_DATA *room;
   int dir;
} M_EXIT;



void merger_push_room( lua_State *L, ROOM_DATA *room )
{
   ROOM_DATA **room_udata;
   
   if ( !room )
     lua_pushnil( L );
   else
     {
        room_udata = (ROOM_DATA**) lua_newuserdata( L, sizeof(ROOM_DATA*) );
        *room_udata = room;
        lua_getfield( L, LUA_REGISTRYINDEX, "merger_room_metatable" );
        lua_setmetatable( L, -2 );
     }
}


void merger_push_spexit( lua_State *L, EXIT_DATA *spexit )
{
   EXIT_DATA **spexit_udata;
   
   if ( !spexit )
     lua_pushnil( L );
   else
     {
        spexit_udata = (EXIT_DATA**) lua_newuserdata( L, sizeof(EXIT_DATA*) );
        *spexit_udata = spexit;
        lua_getfield( L, LUA_REGISTRYINDEX, "merger_spexit_metatable" );
        lua_setmetatable( L, -2 );
     }
}


/***
 ** Map interface **
 ***/

int merger_parse_title( lua_State *L )
{
   const char *str;
   
   if ( !lua_isstring( L, 1 ) )
     {
        lua_pushstring( L, "Invalid argument passed to 'parse_title'." );
        lua_error( L );
     }
   
   str = lua_tostring( L, 1 );
   
   lua_pushboolean( L, parse_title( str ) );
   
   return 1;
}


int merger_load_map( lua_State *L )
{
   const char *str;
   
   if ( !lua_isstring( L, 1 ) )
     {
        lua_pushstring( L, "Invalid argument passed to 'load_map'." );
        lua_error( L );
     }
   
   str = lua_tostring( L, 1 );
   
   lua_pushboolean( L, !load_map( str ) );
   
   return 1;
}


int merger_destroy_map( lua_State *L )
{
   destroy_map( );
   
   return 0;
}


int merger_locate_nearby( lua_State *L )
{
   ROOM_DATA *room;
   const char *title;
   int dir = 0;
   
   if ( lua_gettop( L ) < 1 ||
        !lua_isstring( L, 1 ) )
     {
        lua_pushstring( L, "No valid arguments provided to locate_nearby." );
        lua_error( L );
     }
   
   title = lua_tostring( L, 1 );
   
   if ( lua_gettop( L ) >= 2 && lua_isstring( L, 2 ) )
     {
        const char *dirstring;
        
        dirstring = lua_tostring( L, 2 );
        
        if ( strcmp( dirstring, "look" ) )
          {
             for ( dir = 1; dir_name[dir]; dir++ )
               if ( !strcmp( dirstring, dir_name[dir] ) ||
                    !strcmp( dirstring, dir_small_name[dir] ) )
                 break;
             
             if ( !dir_name[dir] )
               {
                  lua_pushstring( L, "Invalid second argument to locate_nearby." );
                  lua_error( L );
               }
          }
     }
   
   room = locate_nearby_room( title, dir );
   
   merger_push_room( L, room );
   
   return 1;
}


int merger_locate_faraway( lua_State *L )
{
   ROOM_DATA *room;
   const char *title;
   int matches;
   int detected_exits[13];
   int *de = NULL;
   
   if ( lua_gettop( L ) < 1 ||
        !lua_isstring( L, 1 ) )
     {
        lua_pushstring( L, "No valid arguments provided to locate_faraway." );
        lua_error( L );
     }
   
   title = lua_tostring( L, 1 );
   
   if ( lua_gettop( L ) >= 2 && lua_istable( L, 2 ) )
     {
        const char *dirstring;
        int dir;
        int i = 1;
        
        for ( dir = 0; dir < 13; dir++ )
          detected_exits[dir] = 0;
        de = detected_exits;
        
        while ( 1 )
          {
             lua_pushinteger( L, i );
             lua_gettable( L, 2 );
             
             if ( lua_isstring( L, -1 ) )
               {
                  dirstring = lua_tostring( L, -1 );
                  for ( dir = 1; dir_name[dir]; dir++ )
                    if ( !strcmp( dirstring, dir_name[dir] ) ||
                         !strcmp( dirstring, dir_small_name[dir] ) )
                      break;
                  
                  if ( dir_name[dir] )
                    detected_exits[dir] = 1;
               }
             
             if ( lua_isnil( L, -1 ) )
               {
                  lua_pop( L, 1 );
                  break;
               }
             
             lua_pop( L, 1 );
             i++;
          }
     }
   
   room = locate_faraway_room( title, de, &matches );
   
   merger_push_room( L, room );
   lua_pushinteger( L, matches );
   
   return 2;
}



int merger_map_newindex( lua_State *L )
{
   const char *field;
   
   if ( !lua_isstring( L, 2 ) )
     {
        lua_pushstring( L, "Cannot index the map-userdata with a non-string." );
        lua_error( L );
     }
   
   field = lua_tostring( L, 2 );
   
   if ( !strcmp( field, "mode" ) )
     {
        const char *value;
        
        if ( !lua_isstring( L, 3 ) )
          {
             lua_pushstring( L, "map.mode accepts only string values." );
             lua_error( L );
          }
        
        value = lua_tostring( L, 3 );
        
        if ( !strcmp( value, "none" ) )
          mode = NONE;
        else if ( !strcmp( value, "get_unlost" ) )
          mode = GET_UNLOST;
        else if ( !strcmp( value, "following" ) ||
                  !strcmp( value, "follow" ) )
          mode = FOLLOWING;
        else if ( !strcmp( value, "creating" ) ||
                  !strcmp( value, "create" ) )
          mode = CREATING;
        else
          {
             lua_pushstring( L, "map.mode accepts only 'none', "
                             "'get_unlost', 'following', or 'creating'." );
             lua_error( L );
          }
     }
   else if ( !strcmp( field, "current_room" ) )
     {
        if ( lua_isnil( L, 3 ) )
          {
             current_room = NULL;
             current_area = NULL;
          }
        else if ( !lua_isuserdata( L, 3 ) )
          {
             lua_pushstring( L, "map.current_room accepts only 'nil' or a room-userdata." );
             lua_error( L );
          }
        else
          {
             ROOM_DATA *room;
             
             room = *(ROOM_DATA**) lua_touserdata( L, 3 );
             
             current_room = room;
             if ( current_room )
               current_area = current_room->area;
          }
     }
   else
     {
        lua_pushfstring( L, "'map' does not accept a writable '%s' property.",
                        field );
        lua_error( L );
     }
   
   return 0;
}



int merger_map_index( lua_State *L )
{
   const char *field;
   
   if ( !lua_isstring( L, 2 ) )
     {
        lua_pushstring( L, "Cannot index the map-userdata with a non-string." );
        lua_error( L );
     }
   
   field = lua_tostring( L, 2 );
   
   if ( !strcmp( field, "current_room" ) )
     {
        merger_push_room( L, current_room );
     }
   else if ( !strcmp( field, "parse_title" ) )
     {
        lua_pushcfunction( L, merger_parse_title );
     }
   else if ( !strcmp( field, "load" ) )
     {
        lua_pushcfunction( L, merger_load_map );
     }
   else if ( !strcmp( field, "destroy" ) )
     {
        lua_pushcfunction( L, merger_destroy_map );
     }
   else if ( !strcmp( field, "locate_nearby" ) )
     {
        lua_pushcfunction( L, merger_locate_nearby );
     }
   else if ( !strcmp( field, "locate_faraway" ) )
     {
        lua_pushcfunction( L, merger_locate_faraway );
     }
   else if ( !strcmp( field, "mode" ) )
     {
        if ( mode == GET_UNLOST )
          lua_pushstring( L, "get_unlost" );
        else if ( mode == FOLLOWING )
          lua_pushstring( L, "following" );
        else if ( mode == CREATING )
          lua_pushstring( L, "creating" );
        else
          lua_pushstring( L, "none" );
     }
   else
     {
        lua_pushnil( L );
     }
   
   return 1;
}



/***
 ** Rooms and exits interface. **
 ***/

int merger_rooms_index( lua_State *L )
{
   int vnum;
   
   if ( lua_isnumber( L, 2 ) )
     {
        vnum = (int) lua_tointeger( L, 2 );
        
        merger_push_room( L, get_room( vnum ) );
        
        return 1;
     }
   else
     {
        lua_pushstring( L, "Can only index rooms[] with a number." );
        lua_error( L );
     }
   
   return 0;
}


int merger_room_equality( lua_State *L )
{
   ROOM_DATA *room1, *room2;
   
   if ( lua_isuserdata( L, 1 ) &&
        lua_isuserdata( L, 2 ) )
     {
        room1 = *(ROOM_DATA**) lua_touserdata( L, 1 );
        room2 = *(ROOM_DATA**) lua_touserdata( L, 2 );
        
        if ( room1 == room2 )
          {
             lua_pushboolean( L, 1 );
             return 1;
          }
     }
   
   lua_pushboolean( L, 0 );
   return 1;
}


int merger_room_index( lua_State *L )
{
   ROOM_DATA *room;
   EXIT_DATA *spexit;
   const char *field;
   
   if ( !lua_isuserdata( L, 1 ) )
     {
        lua_pushstring( L, "Indexing non-userdata. Internal error!" );
        lua_error( L );
     }
   
   if ( !lua_isstring( L, 2 ) )
     {
        lua_pushstring( L, "Cannot index a room-userdata with a non-string." );
        lua_error( L );
     }
   
   room = *(ROOM_DATA**) lua_touserdata( L, 1 );
   field = lua_tostring( L, 2 );
   
   if ( !strcmp( field, "name" ) || !strcmp( field, "title" ) )
     {
        lua_pushstring( L, room->name );
     }
   else if ( !strcmp( field, "vnum" ) )
     {
        lua_pushinteger( L, room->vnum );
     }
   else if ( !strcmp( field, "underwater" ) )
     {
        lua_pushboolean( L, room->underwater );
     }
   else if ( !strcmp( field, "pf_next" ) )
     {
        merger_push_room( L, room->pf_parent );
     }
   else if ( !strcmp( field, "pf_command" ) )
     {
        if ( room->pf_direction == 0 || !room->pf_parent )
          lua_pushnil( L );
        else if ( room->pf_direction > 0 )
          lua_pushstring( L, dir_name[room->pf_direction] );
        else
          {
             for ( spexit = room->special_exits; spexit; spexit = spexit->next )
               if ( spexit->to == room->pf_parent )
                 break;
             
             if ( spexit && spexit->command )
               lua_pushstring( L, spexit->command );
             else
               lua_pushnil( L );
          }
     }
   else if ( !strcmp( field, "exits" ) )
     {
        ROOM_DATA **r;
        
        r = (ROOM_DATA**) lua_newuserdata( L, sizeof(ROOM_DATA*) );
        *r = room;
        
        lua_getfield( L, LUA_REGISTRYINDEX, "merger_exits_metatable" );
        lua_setmetatable( L, -2 );
     }
   else if ( !strcmp( field, "special_exits" ) )
     {
        ROOM_DATA **r;
        
        r = (ROOM_DATA**) lua_newuserdata( L, sizeof(ROOM_DATA*) );
        *r = room;
        
        lua_getfield( L, LUA_REGISTRYINDEX, "merger_spexits_metatable" );
        lua_setmetatable( L, -2 );
     }
   else if ( !strcmp( field, "area" ) )
     {
        AREA_DATA **a;
        
        if ( !room->area )
          lua_pushnil( L );
        else
          {
             a = (AREA_DATA**) lua_newuserdata( L, sizeof(AREA_DATA*) );
             *a = room->area;
             
             lua_getfield( L, LUA_REGISTRYINDEX, "merger_area_metatable" );
             lua_setmetatable( L, -2 );
          }
     }
   else if ( !strcmp( field, "environment" ) || !strcmp( field, "type" ) )
     {
        ROOM_TYPE **t;
        
        if ( !room->room_type )
          lua_pushnil( L );
        else
          {
             t = (ROOM_TYPE**) lua_newuserdata( L, sizeof(ROOM_TYPE*) );
             *t = room->room_type;
             
             lua_getfield( L, LUA_REGISTRYINDEX, "merger_environment_metatable" );
             lua_setmetatable( L, -2 );
          }
     }
   else
     {
        int dir;
        
        /* room.east; Shortcut to room.exits.east.room. */
        for ( dir = 1; dir_name[dir]; dir++ )
          if ( !strcmp( dir_name[dir], field ) ||
               !strcmp( dir_small_name[dir], field ) )
            break;
        
        if ( dir_name[dir] )
          merger_push_room( L, room->exits[dir] );
        else
          lua_pushnil( L );
     }
   
   return 1;
}


int merger_exits_index( lua_State *L )
{
   ROOM_DATA *room;
   const char *field;
   int dir;
   
   if ( !lua_isuserdata( L, 1 ) )
     {
        lua_pushstring( L, "Indexing non-userdata. Internal error!" );
        lua_error( L );
     }
   
   if ( !lua_isstring( L, 2 ) )
     {
        lua_pushstring( L, "Cannot index an exits-userdata with a non-string." );
        lua_error( L );
     }
   
   room = *(ROOM_DATA**) lua_touserdata( L, 1 );
   field = lua_tostring( L, 2 );
   
   for ( dir = 1; dir_name[dir]; dir++ )
     if ( !strcmp( dir_name[dir], field ) ||
          !strcmp( dir_small_name[dir], field ) )
       break;
   
   if ( !dir_name[dir] )
     {
        lua_pushnil( L );
     }
   else
     {
        M_EXIT *e;
        
        e = lua_newuserdata( L, sizeof( M_EXIT ) );
        e->room = room;
        e->dir = dir;
        
        lua_getfield( L, LUA_REGISTRYINDEX, "merger_exit_metatable" );
        lua_setmetatable( L, -2 );
     }
   
   return 1;
}


int merger_exits_call( lua_State *L )
{
   ROOM_DATA *room;
   const char *val;
   int dir, i;
   
   if ( !lua_isuserdata( L, 1 ) )
     {
        lua_pushstring( L, "Calling non-userdata. Internal error!" );
        lua_error( L );
     }
   
   room = *(ROOM_DATA**) lua_touserdata( L, 1 );
   
   /* No arguments: create an iterator. */
   if ( lua_gettop( L ) == 1 )
     {
        /* It needs to return itself (already on the stack),
         * followed by a state and an initial value. */
        
        lua_pushnil( L );
        lua_pushstring( L, "start" );
        
        return 3;
     }
   
   if ( lua_gettop( L ) != 3 || !lua_isstring( L, 3 ) )
     {
        lua_pushstring( L, "Invalid call to an exits iterator." );
        lua_error( L );
     }
   
   /* Find out the next direction. */
   val = lua_tostring( L, 3 );
   
   if ( !strcmp( val, "start" ) )
     dir = 0;
   else
     {
        for ( dir = 0; dir_name[dir]; dir++ )
          if ( !strcmp( val, dir_name[dir] ) )
            break;
        
        if ( !dir_name[dir] )
          {
             lua_pushnil( L );
             return 1;
          }
     }
   
   for ( i = dir + 1; dir_name[i]; i++ )
     if ( room->exits[i] )
       break;
   
   if ( !dir_name[i] )
     {
        lua_pushnil( L );
        return 1;
     }
   
   lua_pushstring( L, dir_name[i] );
   merger_push_room( L, room->exits[i] );
   
   return 2;
}


int merger_exit_index( lua_State *L )
{
   M_EXIT *e;
   const char *field;
   
   if ( !lua_isuserdata( L, 1 ) )
     {
        lua_pushstring( L, "Indexing non-userdata. Internal error!" );
        lua_error( L );
     }
   
   if ( !lua_isstring( L, 2 ) )
     {
        lua_pushstring( L, "Cannot index an exits-userdata with a non-string." );
        lua_error( L );
     }
   
   e = (M_EXIT*) lua_touserdata( L, 1 );
   field = lua_tostring( L, 2 );
   
   if ( !strcmp( field, "room" ) )
     {
        merger_push_room( L, e->room->exits[e->dir] );
     }
   else
     {
        /* Should probably error instead.. */
        lua_pushnil( L );
     }
   
   return 1;
}


/***
 ** Special exits interface
 ***/

int merger_spexits_index( lua_State *L )
{
   ROOM_DATA *room;
   EXIT_DATA *spexit;
   const char *field;
   
   if ( !lua_isuserdata( L, 1 ) )
     {
        lua_pushstring( L, "Indexing non-userdata. Internal error!" );
        lua_error( L );
     }
   
   if ( !lua_isstring( L, 2 ) )
     {
        lua_pushstring( L, "Cannot index a spexits-userdata with a non-string." );
        lua_error( L );
     }
   
   room = *(ROOM_DATA**) lua_touserdata( L, 1 );
   field = lua_tostring( L, 2 );
   
   for ( spexit = room->special_exits; spexit; spexit = spexit->next )
     if ( spexit->command && !strcmp( field, spexit->command ) )
       break;
   
   merger_push_spexit( L, spexit );
   
   return 1;
}


int merger_spexit_index( lua_State *L )
{
   EXIT_DATA *spexit;
   const char *field;
   
   if ( !lua_isuserdata( L, 1 ) )
     {
        lua_pushstring( L, "Indexing non-userdata. Internal error!" );
        lua_error( L );
     }
   
   if ( !lua_isstring( L, 2 ) )
     {
        lua_pushstring( L, "Cannot index a spexits-userdata with a non-string." );
        lua_error( L );
     }
   
   spexit = *(EXIT_DATA**) lua_touserdata( L, 1 );
   field = lua_tostring( L, 2 );
   
   if ( !strcmp( field, "command" ) || !strcmp( field, "name" ) )
     {
        lua_pushstring( L, spexit->command );
     }
   else if ( !strcmp( field, "message" ) )
     {
        lua_pushstring( L, spexit->message );
     }
   else if ( !strcmp( field, "to" ) || !strcmp( field, "destination" ) )
     {
        merger_push_room( L, spexit->to );
     }
   else
     {
        lua_pushfstring( L, "A special exit has no '%s' property.", field );
        lua_error( L );
     }
   
   return 1;
}


int merger_spexits_call( lua_State *L )
{
   ROOM_DATA *room;
   EXIT_DATA *spexit;
   
   if ( !lua_isuserdata( L, 1 ) )
     {
        lua_pushstring( L, "Calling non-userdata. Internal error!" );
        lua_error( L );
     }
   
   room = *(ROOM_DATA**) lua_touserdata( L, 1 );
   
   /* No arguments: create an iterator. */
   if ( lua_gettop( L ) == 1 )
     {
        /* It needs to return itself (already on the stack),
         * followed by a state and an initial value. */
        
        lua_pushnil( L );
        lua_pushnil( L );
        
        return 3;
     }
   
   if ( lua_gettop( L ) != 3 )
     {
        lua_pushstring( L, "Invalid call to a spexits iterator." );
        lua_error( L );
     }
   
   /* Find out the next direction. */
   if ( lua_isnil( L, 3 ) )
     {
        merger_push_spexit( L, room->special_exits );
     }
   else if ( !lua_isuserdata( L, 3 ) )
     {
        lua_pushstring( L, "Invalid call to a spexits iterator." );
        lua_error( L );
     }
   else
     {
        spexit = *(EXIT_DATA**) lua_touserdata( L, 3 );
        
        merger_push_spexit( L, spexit->next );
     }
   
   return 1;
}


/***
 ** Area tiny-interface
 ***/

int merger_area_index( lua_State *L )
{
   AREA_DATA *area;
   const char *field;
   
   if ( !lua_isuserdata( L, 1 ) )
     {
        lua_pushstring( L, "Indexing non-userdata. Internal error!" );
        lua_error( L );
     }
   
   if ( !lua_isstring( L, 2 ) )
     {
        lua_pushstring( L, "Cannot index an area-userdata with a non-string." );
        lua_error( L );
     }
   
   area = *(AREA_DATA**) lua_touserdata( L, 1 );
   field = lua_tostring( L, 2 );
   
   if ( !strcmp( field, "name" ) )
     {
        lua_pushstring( L, area->name );
     }
   else
     {
        lua_pushnil( L );
     }
   
   return 1;
}



/***
 ** Environment (room type) interface.
 ***/

int merger_environment_index( lua_State *L )
{
   ROOM_TYPE *type;
   const char *field;
   
   if ( !lua_isuserdata( L, 1 ) )
     {
        lua_pushstring( L, "Indexing non-userdata. Internal error!" );
        lua_error( L );
     }
   
   if ( !lua_isstring( L, 2 ) )
     {
        lua_pushstring( L, "Cannot index a type-userdata with a non-string." );
        lua_error( L );
     }
   
   type = *(ROOM_TYPE**) lua_touserdata( L, 1 );
   field = lua_tostring( L, 2 );
   
   if ( !strcmp( field, "name" ) )
     {
        lua_pushstring( L, type->name );
     }
   else if ( !strcmp( field, "color" ) )
     {
        lua_pushstring( L, type->color );
     }
   else if ( !strcmp( field, "must_swim" ) )
     {
        lua_pushboolean( L, type->must_swim );
     }
   else if ( !strcmp( field, "underwater" ) )
     {
        lua_pushboolean( L, type->underwater );
     }
   else if ( !strcmp( field, "avoid" ) )
     {
        lua_pushboolean( L, type->avoid );
     }
   else
     lua_pushnil( L );
   
   return 1;
}




/***                               ***                               ***/
/** Merger API. This should be the only thing visible to the outside. **/
/***                               ***                               ***/


void merger_init( )
{
   mapper_start( );
   
   
}

void merger_unload( )
{
   i_mapper_module_unload( );
   
}

int merger_client_input( char *cmd )
{
   return imapper_commands( cmd );
}


void merger_open_api( lua_State *L )
{
   /* Create the map interface. */
   lua_newuserdata( L, sizeof(void*) );
   
   /* Create the metatable for it. */
   lua_newtable( L );
   lua_pushcfunction( L, merger_map_index );
   lua_setfield( L, -2, "__index" );
   lua_pushcfunction( L, merger_map_newindex );
   lua_setfield( L, -2, "__newindex" );
   
   lua_setmetatable( L, -2 );
   
   lua_setglobal( L, "map" );
   
   /* Create the rooms[] interface. */
   lua_newuserdata( L, sizeof(void*) );
   
   /* Create the metatable for it. */
   lua_newtable( L );
   lua_pushcfunction( L, merger_rooms_index );
   lua_setfield( L, -2, "__index" );
   
   lua_setmetatable( L, -2 );
   lua_setglobal( L, "rooms" );
   
   /* Create the room metatable, and store it for later use. */
   lua_newtable( L );
   lua_pushcfunction( L, merger_room_index );
   lua_setfield( L, -2, "__index" );
   lua_pushcfunction( L, merger_room_equality );
   lua_setfield( L, -2, "__eq" );
   
   lua_setfield( L, LUA_REGISTRYINDEX, "merger_room_metatable" );
   
   /* Create the exits metatable. */
   lua_newtable( L );
   lua_pushcfunction( L, merger_exits_index );
   lua_setfield( L, -2, "__index" );
   lua_pushcfunction( L, merger_exits_call );
   lua_setfield( L, -2, "__call" );
   
   lua_setfield( L, LUA_REGISTRYINDEX, "merger_exits_metatable" );
   
   /* Create the exit metatable. */
   lua_newtable( L );
   lua_pushcfunction( L, merger_exit_index );
   lua_setfield( L, -2, "__index" );
   
   lua_setfield( L, LUA_REGISTRYINDEX, "merger_exit_metatable" );
   
   /* Create the special exits metatable. */
   lua_newtable( L );
   lua_pushcfunction( L, merger_spexits_index );
   lua_setfield( L, -2, "__index" );
   lua_pushcfunction( L, merger_spexits_call );
   lua_setfield( L, -2, "__call" );
   
   lua_setfield( L, LUA_REGISTRYINDEX, "merger_spexits_metatable" );
   
   /* Create the special exit metatable. */
   lua_newtable( L );
   lua_pushcfunction( L, merger_spexit_index );
   lua_setfield( L, -2, "__index" );
   
   lua_setfield( L, LUA_REGISTRYINDEX, "merger_spexit_metatable" );
   
   /* Create area metatable. */
   lua_newtable( L );
   lua_pushcfunction( L, merger_area_index );
   lua_setfield( L, -2, "__index" );
   
   lua_setfield( L, LUA_REGISTRYINDEX, "merger_area_metatable" );
   
   /* Create environment metatable. */
   lua_newtable( L );
   lua_pushcfunction( L, merger_environment_index );
   lua_setfield( L, -2, "__index" );
   
   lua_setfield( L, LUA_REGISTRYINDEX, "merger_environment_metatable" );
}

