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
extern ROOM_DATA *current_room;
void mapper_start( );
void destroy_map( );
void i_mapper_module_unload( );
int  i_mapper_process_client_aliases( char *cmd );
extern char *dir_name[];
extern char *dir_small_name[];
int parse_title( const char *line );
int load_map( const char *file );


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
   else if ( !strcmp( field, "exits" ) )
     {
        ROOM_DATA **r;
        
        r = (ROOM_DATA**) lua_newuserdata( L, sizeof(ROOM_DATA*) );
        *r = room;
        
        lua_getfield( L, LUA_REGISTRYINDEX, "merger_exits_metatable" );
        lua_setmetatable( L, -2 );
     }
   else
     {
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
   return i_mapper_process_client_aliases( cmd );
}


void merger_open_api( lua_State *L )
{
   /* Create the map interface. */
   lua_newuserdata( L, sizeof(void*) );
   
   /* Create the metatable for it. */
   lua_newtable( L );
   
   lua_pushcfunction( L, merger_map_index );
   lua_setfield( L, -2, "__index" );
   
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
}

