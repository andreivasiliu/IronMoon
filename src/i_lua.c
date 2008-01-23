/* Prototype ILua Module */

#define I_LUA_ID "$Name: HEAD $ $Id: i_lua.c,v 1.1 2008/01/23 12:10:21 andreivasiliu Exp $"
#define BUILTIN_MODULE 1

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <pcre.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>

#include "module.h"


int i_lua_version_major = 0;
int i_lua_version_minor = 8;

char *i_lua_id = I_LUA_ID "\r\n" HEADER_ID "\r\n" MODULE_ID "\r\n";


typedef struct ilua_mod ILUA_MOD;

struct ilua_mod
{
   char *name;
   char *work_dir;
   
   short disabled;
   
   lua_State *L;
   
   ILUA_MOD *next;
};

struct pcre_userdata
{
   pcre *data;
   pcre_extra *extra;
};


ILUA_MOD *ilua_modules;

static LINES *current_paragraph;

void ilua_open_mbapi( lua_State *L );
void ilua_open_mbcolours( lua_State *L );
void read_ilua_config( char *file_name, char *mod_name );

#define ILUA_PREF C_R "[" C_W "ILua" C_R "]: "

/* Here we register our functions. */

void i_lua_init_data( );
void i_lua_unload( );
void i_lua_process_server_paragraph( LINES *l );
int i_lua_process_client_aliases( char *cmd );
int i_lua_process_client_command( char *cmd );


ENTRANCE( i_lua_module_register )
{
   self->name = strdup( "ILua" );
   self->version_major = i_lua_version_major;
   self->version_minor = i_lua_version_minor;
   self->id = i_lua_id;
   
   self->init_data = i_lua_init_data;
   self->unload = i_lua_unload;
   self->process_server_paragraph = i_lua_process_server_paragraph;
   self->process_client_command = i_lua_process_client_command;
   self->process_client_aliases = i_lua_process_client_aliases;
}



void i_lua_init_data( )
{
   read_ilua_config( "config.ilua.txt", NULL );
}



static int ilua_errorhandler( lua_State *L )
{
   lua_Debug ar;
   const char *errmsg;
   char where[256];
   int level;
   
   errmsg = lua_tostring( L, -1 );
   if ( !errmsg )
     errmsg = "Untrapped error from Lua";
   
   clientff( "\r\n" ILUA_PREF "%s\r\n" C_0, errmsg );
   
   clientff( C_R "Traceback:\r\n" C_0 );
   level = 1;
   while ( lua_getstack( L, level++, &ar ) )
     {
        lua_getinfo( L, "nSl", &ar );
        
        if ( ar.name )
          snprintf( where, 256, "in function '%s'", ar.name);
        else if ( ar.what[0] == 'm' )
          snprintf( where, 256, "in main chunk" );
        else if ( ar.what[0] == 't' )
          snprintf( where, 256, "in a tail call (I forgot the function)" );
        else if ( ar.what[0] == 'C' )
          snprintf( where, 256, "in a C function" );
        else
          snprintf( where, 256, "in a Lua function" );
        
        clientff( C_W " %s" C_0 ":" C_G "%d" C_0 ": %s\r\n", ar.short_src, ar.currentline, where );
     }
   
   return 1;
}



void close_ilua_module( ILUA_MOD *module )
{
   ILUA_MOD *m;
   
   /* Unlink it. */
   if ( module == ilua_modules )
     ilua_modules = module->next;
   else
     {
        m = ilua_modules;
        while ( m && m->next != module )
          m = m->next;
        
        if ( m )
          m->next = module->next;
     }
   
   if ( module->L )
     lua_close( module->L );
   
   if ( module->name )
     free( module->name );
   if ( module->work_dir )
     free( module->work_dir );
   
   free( module );
}



void read_ilua_config( char *file_name, char *mod_name )
{
   FILE *fl;
   ILUA_MOD *mod = NULL;
   lua_State *L = NULL;
   struct stat stat_buf;
   const char *errmsg;
   char line[4096], cmd[256], arg[4096], *p;
   char full_path[4096], current_work_dir[4096];
   int ignore = 0, all_okay = 1, err;
   
   if ( stat( file_name, &stat_buf ) )
     {
        fl = fopen( file_name, "w" );
        if ( !fl )
          {
             debugerr( file_name );
             return;
          }
        
        fprintf( fl, "# ILua configuration file.\n\n" );
        
        fprintf( fl, "# This file is sectioned in modules. Each module has its own globals, and\n"
		 "# modules cannot communicate between them. A module may load any number of\n"
		 "# Lua script files.\n\n");
        
	fprintf( fl, "# Example of an ILua module:\n\n" );
	fprintf( fl, "#module \"Foo\"\n"
		 "#load \"example1.lua\"\n"
		 "#load \"example2.lua\"\n\n" );
        
        fclose( fl );
     }
   
   fl = fopen( file_name, "r" );
   
   if ( !fl )
     return;
   
   get_timer( );
   
   while( 1 )
     {
        fgets( line, 4096, fl );
        
        if ( feof( fl ) )
          break;
        
        p = get_string( line, cmd, 256 );
        p = get_string( p, arg, 4096 );
        
        if ( !strcmp( cmd, "module" ) )
          {
             ignore = 0;
             if ( mod_name && strcmp( arg, mod_name ) )
               {
                  ignore = 1;
                  continue;
               }
             
             /* Create a shiny, brand new module. */
             if ( !ilua_modules )
               {
                  mod = calloc( 1, sizeof( ILUA_MOD ) );
                  ilua_modules = mod;
               }
             else
               {
                  mod = ilua_modules;
                  while ( mod->next )
                    mod = mod->next;
                  mod->next = calloc( 1, sizeof( ILUA_MOD ) );
                  mod = mod->next;
               }
             
             mod->name = strdup( arg );
             
             /* See if it has the optional "working directory" argument. */
             get_string( p, arg, 4096 );
             if ( arg[0] )
               {
                  mod->work_dir = strdup( arg );
                  getcwd( current_work_dir, 4096 );
                  if ( chdir( arg ) )
                    {
                       debugf( "%s (ILua): %s: %s", mod->name, mod->work_dir,
                               strerror( errno ) );
                       close_ilua_module( mod );
                       ignore = 1;
                       all_okay = 0;
                       continue;
                    }
                  /* The path given may be relative. Don't store that one. */
                  getcwd( full_path, 4096 );
                  mod->work_dir = strdup( full_path );
               }
             
             /* Initialize it. */
             L = luaL_newstate( );
             mod->L = L;
             
             luaL_openlibs( L );
             ilua_open_mbapi( L );
             ilua_open_mbcolours( L );
             
             if ( mod->work_dir )
               chdir( current_work_dir );
          }
        
        if ( ignore )
          continue;
        
        if ( !strcmp( cmd, "load" ) )
          {
             if ( !L || !mod )
               {
                  debugf( "%s: Trying to 'load' outside a module!", file_name );
                  return;
               }
             
             if ( mod->work_dir )
               {
                  getcwd( current_work_dir, 4096 );
                  if ( chdir( mod->work_dir ) )
                    {
                       debugf( "%s (ILua): %s: %s", mod->name, mod->work_dir,
                               strerror( errno ) );
                       close_ilua_module( mod );
                       ignore = 1;
                       all_okay = 0;
                       continue;
                    }
               }
             
             lua_pushcfunction( L, ilua_errorhandler );
             
             err = luaL_loadfile( L, arg ) || lua_pcall( L, 0, 0, -2 );
             
             if ( mod->work_dir )
               chdir( current_work_dir );
             
             if ( err )
               {
                  errmsg = lua_tostring( L, -1 );
                  if ( errmsg )
                    {
                       debugf( "%s: %s", arg, errmsg );
                       clientff( ILUA_PREF "%s: %s\r\n" C_0, arg, errmsg );
                    }
                  
                  /* Close this module. Entirely. */
                  close_ilua_module( mod );
                  ignore = 1;
                  all_okay = 0;
               }
             else
               lua_pop( L, 1 );
          }
     }
   
   if ( all_okay && !mod_name )
     debugf( "ILua scripts loaded. (%d microseconds)", get_timer( ) );
   
   fclose( fl );
}



void set_lines_in_table( lua_State *L, LINES *l )
{
   int i;
   
   lua_getglobal( L, "mb" );
   if ( !lua_istable( L, -1 ) )
     {
        lua_pop( L, 1 );
        clientfr( "** What the hell did you do with the global 'mb' table? **" );
        ilua_open_mbapi( L );
        lua_getglobal( L, "mb" );
     }
   
   /* Put some info from our LINE structure in here. */
   
   lua_newtable( L );
   for ( i = 1; i <= l->nr_of_lines; i++ )
     {
	lua_pushnumber( L, i );
	lua_pushstring( L, l->line[i] );
	lua_settable( L, -3 );
     }
   lua_setfield( L, -2, "lines" );
   
   lua_newtable( L );
   for ( i = 1; i <= l->nr_of_lines; i++ )
     {
	lua_pushnumber( L, i );
	lua_pushnumber( L, l->len[i] );
	lua_settable( L, -3 );
     }
   lua_setfield( L, -2, "line_len" );
   
   lua_newtable( L );
   for ( i = 1; i <= l->nr_of_lines; i++ )
     {
	lua_pushnumber( L, i );
	lua_pushnumber( L, l->line_start[i] );
	lua_settable( L, -3 );
     }
   lua_setfield( L, -2, "line_start" );
   
   lua_pushstring( L, l->prompt );
   lua_setfield( L, -2, "prompt" );
   lua_pushnumber( L, l->prompt_len );
   lua_setfield( L, -2, "prompt_len" );
   lua_pushnumber( L, l->line_start[l->nr_of_lines + 1] );
   lua_setfield( L, -2, "prompt_start" );
   
   lua_pushlstring( L, l->raw, l->raw_len );
   lua_setfield( L, -2, "raw" );
   lua_pushnumber( L, l->raw_len );
   lua_setfield( L, -2, "raw_len" );
   lua_pushlstring( L, l->lines, l->full_len );
   lua_setfield( L, -2, "paragraph" );
   lua_pushnumber( L, l->full_len );
   lua_setfield( L, -2, "paragraph_len" );
   lua_pushnumber( L, l->nr_of_lines );
   lua_setfield( L, -2, "nr_of_lines" );
   
   lua_pushnil( L );
   lua_setfield( L, -2, "line" );
   lua_pushnil( L );
   lua_setfield( L, -2, "len" );
   lua_pushnil( L );
   lua_setfield( L, -2, "current_line" );
   
   lua_pop( L, 1 );
}



void set_atcp_table( lua_State *L )
{
   static int *a_hp, *a_mana, *a_end, *a_will, *a_exp, *a_on;
   static int *a_max_hp, *a_max_mana, *a_max_end, *a_max_will;
   static char *a_name, *a_title;
   
   if ( !a_on )
     {
        a_on = get_variable( "a_on" );
        a_hp = get_variable( "a_hp" );
        a_mana = get_variable( "a_mana" );
        a_end = get_variable( "a_end" );
        a_will = get_variable( "a_will" );
        a_exp = get_variable( "a_exp" );
        a_max_hp = get_variable( "a_max_hp" );
        a_max_mana = get_variable( "a_max_mana" );
        a_max_end = get_variable( "a_max_end" );
        a_max_will = get_variable( "a_max_will" );
        a_name = get_variable( "a_name" );
        a_title = get_variable( "a_title" );
     }
   
   if ( !a_on || !*a_on )
     {
        lua_pushnil( L );
        lua_setglobal( L, "atcp" );
        return;
     }
   
   lua_newtable( L );
   
   if ( a_hp )
     lua_pushinteger( L, *a_hp ), lua_setfield( L, -2, "health" );
   if ( a_mana )
     lua_pushinteger( L, *a_mana ), lua_setfield( L, -2, "mana" );
   if ( a_end )
     lua_pushinteger( L, *a_end ), lua_setfield( L, -2, "endurance" );
   if ( a_will )
     lua_pushinteger( L, *a_will ), lua_setfield( L, -2, "willpower" );
   if ( a_exp )
     lua_pushinteger( L, *a_exp ), lua_setfield( L, -2, "exp" );
   if ( a_max_hp )
     lua_pushinteger( L, *a_max_hp ), lua_setfield( L, -2, "max_health" );
   if ( a_max_mana )
     lua_pushinteger( L, *a_max_mana ), lua_setfield( L, -2, "max_mana" );
   if ( a_max_end )
     lua_pushinteger( L, *a_max_end ), lua_setfield( L, -2, "max_endurance" );
   if ( a_max_will )
     lua_pushinteger( L, *a_max_will ), lua_setfield( L, -2, "max_willpower" );
   if ( a_name )
     lua_pushstring( L, a_name ), lua_setfield( L, -2, "name" );
   if ( a_title )
     lua_pushstring( L, a_title ), lua_setfield( L, -2, "title" );
   
   lua_setglobal( L, "atcp" );
}



int ilua_callback( lua_State *L, char *func, char *arg, char *dir )
{
   char current_work_dir[4096];
   int ret_value = 0;
   
   if ( dir )
     {
        getcwd( current_work_dir, 4096 );
        if ( chdir( dir ) )
          {
             clientff( "\r\n" ILUA_PREF "%s: %s\r\n" C_0, dir, strerror( errno ) );
             return 0;
          }
     }
   
   lua_getglobal( L, "mb" );
   if ( lua_istable( L, -1 ) )
     {
        lua_pushcfunction( L, ilua_errorhandler );
        lua_getfield( L, -2, func );
        if ( lua_isfunction( L, -1 ) )
          {
             if ( arg )
               lua_pushstring( L, arg );
             
             if ( lua_pcall( L, arg ? 1 : 0, 1, arg ? -3 : -2 ) )
               {
                  clientff( C_R "Error in the '%s' callback.\r\n" C_0, func );
                  ret_value = 1;
               }
             else
               {
                  if ( lua_isboolean( L, -1 ) && lua_toboolean( L, -1 ) )
                    ret_value = 1;
               }
             lua_pop( L, 1 );
          }
        else
          lua_pop( L, 1 ); /* The callback */
        
        lua_pop( L, 1 ); /* The error handler */
     }
   
   lua_pop( L, 1 ); /* 'mb' table */
   
   if ( dir )
     chdir( current_work_dir );
   
   if ( lua_gettop( L ) )
     {
        debugf( "Warning! Stack isn't empty! %d elements left.", lua_gettop( L ) );
        lua_pop( L, lua_gettop( L ) );
     }
   
   return ret_value;
}



void i_lua_unload( )
{
   ILUA_MOD *m;
   
   for ( m = ilua_modules; m; m = m->next )
     {
        ilua_callback( m->L, "unload", NULL, m->work_dir );
        lua_close( m->L );
     }
}



void i_lua_process_server_paragraph( LINES *l )
{
   ILUA_MOD *m;
   
   /* Ideally, this wouldn't exist.. it's needed for get_color_at() though. */
   current_paragraph = l;
   
   for ( m = ilua_modules; m; m = m->next )
     {
	set_lines_in_table( m->L, l );
	set_atcp_table( m->L );
	ilua_callback( m->L, "server_lines", NULL, m->work_dir );
     }
   
   current_paragraph = NULL;
}



int i_lua_process_client_aliases( char *cmd )
{
   int i = 0;
   ILUA_MOD *m;
   
   for ( m = ilua_modules; m; m = m->next )
     {
        i |= ilua_callback( m->L, "client_aliases", cmd, m->work_dir );
     }
   
   return i;
}



int i_lua_process_client_command( char *cmd )
{
   char buf[4096], *p;
   ILUA_MOD *mod;
   
   /* Skip `il/ilua, and get the command */
   p = get_string( cmd, buf, 4096 );
   if ( strcmp( buf, "`il" ) && strcmp( buf, "`ilua" ) )
     return 1;
   
   p = get_string( p, buf, 4096 );
   
   if ( !strcmp( buf, "mods" ) )
     {
        clientfr( "Lua Modules:");
        for ( mod = ilua_modules; mod; mod = mod->next )
          clientff( " - %s\r\n", mod->name );
     }
   else if ( !strcmp( buf, "help" ) )
     {
        clientfr( "ILua commands:" );
        clientf( " mods   - Lists loaded ILua modules.\r\n"
                 " help   - I'll give you three guesses.\r\n"
                 " load   - Load all files of the given module, after unloading.\r\n"
                 " reload - An alias of the above.\r\n"
                 " unload - Unload the given module.\r\n" );
        clientfr( "All commands must be prefixed by `il or `ilua." );
     }
   else if ( !strcmp( buf, "unload" ) )
     {
        get_string( p, buf, 4096 );
        
        if ( !buf[0] )
          {
             clientfr( "Which module should I unload?" );
             return 0;
          }
        
        for ( mod = ilua_modules; mod; mod = mod->next )
          if ( !strcmp( mod->name, buf ) )
            break;
        
        if ( !mod )
          clientfr( "Careful with spelling and capitalization. That module doesn't exist." );
        else
          {
             clientff( C_R "[Unloading '%s'.]\r\n" C_0, mod->name );
             close_ilua_module( mod );
          }
     }
   else if ( !strcmp( buf, "load" ) || !strcmp( buf, "reload" ) )
     {
        get_string( p, buf, 4096 );
        
        if ( !buf[0] )
          {
             clientfr( "Which module should I load?" );
             return 0;
          }
        
        for ( mod = ilua_modules; mod; mod = mod->next )
          if ( !strcmp( mod->name, buf ) )
            break;
        
        if ( mod )
          {
             clientff( C_R "[Unloading current module.]\r\n" C_0 );
             ilua_callback( mod->L, "unload", NULL, mod->work_dir );
             close_ilua_module( mod );
          }
        
        clientff( C_R "[Loading '%s'.]\r\n" C_0, buf );
        read_ilua_config( "config.ilua.txt", buf );
     }
   else
     {
        clientfr( "Unknown ILua command. Use `il help for a useful list." );
     }
   
   return 0;
}



static int ilua_echo( lua_State *L )
{
   const char *str;
   
   /* This thingie also leaves an empty string on no arguments. Neat. */
   lua_pushstring( L, "\r\n" );
   lua_concat( L, lua_gettop( L ) );
   
   str = lua_tostring( L, -1 );
   if ( str )
     clientf( (char*) str );
   lua_pop( L, 1 );
   
   return 0;
}



static int ilua_send( lua_State *L )
{
   const char *str;
   
   lua_pushstring( L, "\r\n" );
   lua_concat( L, lua_gettop( L ) );
   
   str = lua_tostring( L, -1 );
   if ( str )
     send_to_server( (char*) str );
   lua_pop( L, 1 );
   
   return 0;
}



static int ilua_debug( lua_State *L )
{
   const char *str;
   
   lua_concat( L, lua_gettop( L ) );
   
   str = lua_tostring( L, -1 );
   if ( str )
     debugf( "%s", str );
   lua_pop( L, 1 );
   
   return 0;
}



void paragraph_manip( lua_State *L,
		     void (*func_at)( int line, char *string ),
		     void (*func)( char *string ) )
{
   char *str;
   int line = 0, use_line = 0;
   
   if ( lua_isnumber( L, 1 ) )
     {
	line = lua_tonumber( L, 1 );
	lua_remove( L, 1 );
	use_line = 1;
     }
   
   lua_concat( L, lua_gettop( L ) );
   
   str = (char*) lua_tostring( L, -1 );
   if ( str )
     {
	if ( use_line )
	  func_at( line, str );
	else
	  func( str );
     }
   lua_pop( L, 1 );
}



static int ilua_prefix( lua_State *L )
{
   paragraph_manip( L, prefix_at, prefix );
   
   return 0;
}



static int ilua_suffix( lua_State *L )
{
   paragraph_manip( L, suffix_at, suffix );
   
   return 0;
}



static int ilua_insert( lua_State *L )
{
   const char *str;
   int pos;
   int line = 0, use_line = 0;
   
   if ( lua_isnumber( L, 2 ) )
     {
	line = luaL_checkint( L, 1 );
	use_line = 1;
	lua_remove( L, 1 );
     }
   
   pos = luaL_checkint( L, 1 );
   
   lua_concat( L, lua_gettop( L ) - 1 );
   
   str = lua_tostring( L, -1 );
   if ( str )
     {
	if ( use_line )
	  insert_at( line, pos, (char*) str );
	else
	  insert( pos, (char*) str );
     }
   
   lua_pop( L, 2 );
   
   return 0;
}



static int ilua_replace( lua_State *L )
{
   paragraph_manip( L, replace_at, replace );
   
   return 0;
}



static int ilua_show_prompt( lua_State *L )
{
   show_prompt( );
   
   return 0;
}



static int ilua_get_color_at( lua_State *L )
{
   char *colour;
   int line = 0, use_line = 0;
   int pos;
   
   if ( !current_paragraph )
     {
	lua_pushstring( L, "get_color_at() called at a wrong time." );
	lua_error( L );
     }
   
   if ( lua_isnumber( L, 2 ) )
     {
	line = luaL_checkint( L, 1 );
	use_line = 1;
     }
   
   pos = luaL_checkint( L, 1 + use_line );
   
   if ( use_line )
     colour = get_poscolor_at( line, pos );
   else
     colour = get_poscolor( pos );
   
   lua_pop( L, 1 + use_line );
   lua_pushstring( L, colour );
   
   return 1;
}



static int ilua_hide_line( lua_State *L )
{
   if ( lua_isnumber( L, 1 ) )
     {
	int line;
	
	line = lua_tonumber( L, 1 );
	hide_line_at( line );
	
	lua_remove( L, 1 );
     }
   else
     hide_line( );
   
   return 0;
}



static int ilua_set_line( lua_State *L )
{
   int line;
   
   line = luaL_checkint( L, 1 );
   
   set_line( line );
   
   lua_getglobal( L, "mb" );
   if ( !lua_istable( L, -1 ) )
     {
        lua_pop( L, 1 );
        clientfr( "** What the hell did you do with the global 'mb' table? **" );
        ilua_open_mbapi( L );
        lua_getglobal( L, "mb" );
     }
   
   if ( line == -1 )
     {
	lua_getfield( L, -1, "prompt" );
	lua_setfield( L, -2, "line" );
	
	lua_getfield( L, -1, "prompt_len" );
	lua_setfield( L, -2, "len" );
     }
   else
     {
	lua_getfield( L, -1, "lines" );
	lua_pushnumber( L, line );
	lua_gettable( L, -2 );
	lua_setfield( L, -3, "line" );
	lua_pop( L, 1 );
	
	lua_getfield( L, -1, "line_len" );
	lua_pushnumber( L, line );
	lua_gettable( L, -2 );
	lua_setfield( L, -3, "len" );
	lua_pop( L, 1 );
     }
   
   lua_pushnumber( L, line );
   lua_setfield( L, -2, "current_line" );
   lua_pop( L, 1 );
   
   return 0;
}


static int ilua_cmp( lua_State *L )
{
   const char *pattern, *text;
   
   pattern = luaL_checkstring( L, 1 );
   text = luaL_checkstring( L, 2 );
   
   lua_pop( L, 2 );
   
   lua_pushboolean( L, !cmp( (char*) pattern, (char*) text ) );
   
   return 1;
}



void ilua_timer( TIMER *self )
{
   ILUA_MOD *m;
   lua_State *L;
   int args, i;
   int timertable;
   char current_work_dir[4096];
   
   for ( m = ilua_modules; m; m = m->next )
     if ( m->L == (lua_State *) self->pdata[0] )
       break;
   
   if ( !m )
     {
        debugf( "ILua Warning: this timer has an unknown Lua thread." );
        return;
     }
   
   L = m->L;
   
   lua_pushlightuserdata( L, (void *) self );
   lua_gettable( L, LUA_REGISTRYINDEX );
   
   if ( !lua_istable( L, -1 ) )
     {
        debugf( "ILua Warning: this timer has lost its associated info." );
        lua_pop( L, 1 );
        return;
     }
   
   timertable = lua_gettop( L );
   
   lua_pushcfunction( L, ilua_errorhandler );
   lua_getfield( L, timertable, "args" );
   lua_getfield( L, timertable, "func" );
   
   if ( !lua_isfunction( L, -1 ) || !lua_isnumber( L, -2 ) )
     {
        lua_pop( L, 3 );
        return;
     }
   
   if ( m->work_dir )
     {
        getcwd( current_work_dir, 4096 );
        if ( chdir( m->work_dir ) )
          {
             debugf( "%s (ILua): %s: %s", m->name, m->work_dir,
                     strerror( errno ) );
          }
     }
   
   /* Get the arguments ready... */
   args = lua_tointeger( L, -2 );
   lua_checkstack( L, args );
   for ( i = 1; i <= args; i++ )
     {
        lua_pushinteger( L, i );
        lua_gettable( L, timertable );
     }
   
   /* Fire away! */
   if ( lua_pcall( L, args, 0, timertable+1 ) )
     {
        clientff( C_R "Error in the '%s' timer.\r\n" C_0, self->name );
        lua_pop( L, 1 );
     }
   
   /* The table, error handler, and number of arguments. */
   lua_pop( L, 3 );
   
   if ( m->work_dir )
     chdir( current_work_dir );
}



void ilua_destroy_timer( TIMER *timer )
{
   ILUA_MOD *m;
   
   for ( m = ilua_modules; m; m = m->next )
     if ( m->L == (lua_State *) timer->pdata[0] )
       break;
   
   if ( !m )
     return;
   
   /* Lua's garbage collector will take care of the table that was there. */
   lua_pushlightuserdata( m->L, (void *) timer );
   lua_pushnil( m->L );
   lua_settable( m->L, LUA_REGISTRYINDEX );
}



/* add_timer( number/time, function/action, optstring/name, ... */
static int ilua_add_timer( lua_State *L )
{
   TIMER *timer;
   lua_Number fire_at;
   const char *name;
   lua_Number delay;
   int optargs, i;
   
   delay = luaL_checknumber( L, 1 );
   luaL_checktype( L, 2, LUA_TFUNCTION );
   name = luaL_optstring( L, 3, "*unnamed_lua_timer" );
   
   timer = add_timer( name, (float) delay, ilua_timer, 0, 0, 0 );
   timer->pdata[0] = (void *) L;
   timer->destroy_cb = ilua_destroy_timer;
   
   optargs = lua_gettop( L ) - 3;
   if ( optargs < 0 )
     optargs = 0;
   
   lua_pushlightuserdata( L, (void *) timer );
   lua_createtable( L, optargs, 2 );
   
   lua_pushstring( L, "func" );
   lua_pushvalue( L, 2 );
   lua_settable( L, -3 );
   
   lua_pushstring( L, "args" );
   lua_pushinteger( L, optargs );
   lua_settable( L, -3 );
   
   /* Optional values, to be passed as arguments later. */
   for ( i = 0; i < optargs; i++ )
     {
        lua_pushinteger( L, i+1 );
        lua_pushvalue( L, i+4 );
        lua_settable( L, -3 );
     }
   
   lua_settable( L, LUA_REGISTRYINDEX );
   
   /* Return the fire_at time. */
   fire_at = (lua_Number) (timer->fire_at_sec & 0xFFFFFF) +
             (lua_Number) timer->fire_at_usec / 1000000;
   lua_pushnumber( L, fire_at );
   
   return 1;
}



static int ilua_del_timer( lua_State *L )
{
   const char *name;
   
   name = luaL_checkstring( L, 1 );
   
   del_timer( name );
   
   return 0;
}



static int ilua_gettimeofday( lua_State *L )
{
   struct timeval tv;
   lua_Number timeofday;
   
   gettimeofday( &tv, NULL );
   
   /* Lowering the upper precision, so that the fractional position can fit. */
   timeofday = (lua_Number) (tv.tv_sec & 0xFFFFFF) +
               (lua_Number) tv.tv_usec / 1000000;
   
   lua_pushnumber( L, timeofday );
   
   return 1;
}



static int ilua_regex_compile( lua_State *L )
{
   struct pcre_userdata *pattern;
   pcre *pattern_data;
   const char *str, *error;
   int error_offset;
   
   str = luaL_checkstring( L, 1 );
   
   pattern_data = pcre_compile( str, 0, &error, &error_offset, NULL );
   
   if ( error )
     {
	char buf[256];
	
	sprintf( buf, "Cannot compile regular expression, at character "
		 "%d.\nReason: %s.\nPattern was: '%s'",
		 error_offset, error, str );
	
	lua_pushstring( L, buf );
	lua_error( L );
     }
   
   pattern = lua_newuserdata( L, sizeof( struct pcre_userdata ) );
   
   pattern->data = pattern_data;
   pattern->extra = pcre_study( pattern_data, 0, &error );
   
   if ( error )
     {
	char buf[256];
	
	sprintf( buf, "Pattern study error: %s.\nPattern was: '%s'",
		 error, str );
	
	lua_pushstring( L, buf );
	lua_error( L );
     }
   
   lua_remove( L, 1 );
   
   return 1;
}



static int ilua_regex_match( lua_State *L )
{
   pcre *pattern_data;
   pcre_extra *extra;
   const char *error;
   int error_offset;
   const char *text;
   int offset;
   size_t text_len;
   int temporary = 0;
   
   int ovector[90];
   int rc, i;
   
   int name_count, name_entry_size, nr;
   char *name_table;
   
   if ( lua_isuserdata( L, 1 ) )
     {
	struct pcre_userdata *pattern;
	
	pattern = lua_touserdata( L, 1 );
	pattern_data = pattern->data;
	extra = pattern->extra;
     }
   else
     {
	const char *str;
	
	str = luaL_checkstring( L, 1 );
	
	pattern_data = pcre_compile( str, 0, &error, &error_offset, NULL );
	extra = NULL;
	temporary = 1;
	
	if ( error )
	  {
	     char buf[256];
	     
	     sprintf( buf, "Cannot compile regular expression, at character "
		      "%d.\nReason: %s.\nPattern was: '%s'",
		      error_offset, error, str );
	     
	     lua_pushstring( L, buf );
	     lua_error( L );
	  }
     }
   
   if ( lua_isnumber( L, 2 ) )
     {
	offset = lua_tonumber( L, 2 );
	lua_remove( L, 2 );
     }
   else
     offset = 0;
   
   text = luaL_checklstring( L, 2, &text_len );
   
   rc = pcre_exec( pattern_data, extra, text, text_len,
		   offset, 0, ovector, 30 );
   
   /* No match. */
   if ( rc < 0 )
     {
	lua_pop( L, 2 );
	lua_pushnil( L );
	if ( temporary )
	  free( pattern_data );
	return 1;
     }
   
   lua_newtable( L );
   
   /* .start_offset */
   lua_newtable( L );
   /* .end_offset */
   lua_newtable( L );
   
   /* Put the substrings into the table. */
   
   if ( rc == 0 )
     rc = 30;
   for ( i = 0; i < rc; i++ )
     {
	lua_pushnumber( L, i );
	lua_pushlstring( L, text + ovector[i*2],
			 ovector[i*2+1] - ovector[i*2] );
	lua_settable( L, -5 );
	
	lua_pushnumber( L, i );
	lua_pushnumber( L, ovector[i*2] );
	lua_settable( L, -4 );
	
	lua_pushnumber( L, i );
	lua_pushnumber( L, ovector[i*2+1] );
	lua_settable( L, -3 );
     }
   
   lua_setfield( L, -3, "end_offset" );
   lua_setfield( L, -2, "start_offset" );
   
   /* Save named substrings into the table as well. */
   
   pcre_fullinfo( pattern_data, NULL, PCRE_INFO_NAMECOUNT, &name_count );
   pcre_fullinfo( pattern_data, NULL, PCRE_INFO_NAMEENTRYSIZE, &name_entry_size );
   pcre_fullinfo( pattern_data, NULL, PCRE_INFO_NAMETABLE, &name_table );
   
   for ( i = 0; i < name_count; i++ )
     {
        nr  = ((unsigned char *)name_table)[name_entry_size*i] * 256;
	nr += ((unsigned char *)name_table)[name_entry_size*i+1];
	
        if ( nr >= rc )
          continue;
        
	lua_pushstring( L, name_table + name_entry_size*i + 2 );
	lua_pushlstring( L, text + ovector[nr*2],
			 ovector[nr*2+1] - ovector[nr*2] );
	lua_settable( L, -3 );
     }
   
   lua_remove( L, 2 );
   lua_remove( L, 1 );
   
   if ( temporary )
     free( pattern_data );
   
   return 1;
}


/*
 * mb =
 * {
 *    version = "M.m"
 *    
 *    -- Callbacks
 *    unload = nil,
 *    server_paragraph = nil,
 *    client_aliases = nil,
 *    
 *    -- Communication
 *    echo
 *    send
 *    debug
 *    prefix
 *    suffix
 *    insert
 *    replace
 *    show_prompt
 *    
 *    -- Utility
 *    cmp
 *    
 *    -- MXP / not implemented
 *    
 *    -- Timers
 *    add_timer
 *    del_timer
 *    
 *    -- Networking / not implemented
 *    
 *    -- LINE structure / old
 *    line
 *    raw_line
 *    ending
 *    len
 *    raw_len
 *    
 *    gag_line
 *    gag_ending
 * }
 * 
 */


void ilua_open_mbapi( lua_State *L )
{
   lua_newtable( L );
   
   lua_pushfstring( L, "%d.%d", i_lua_version_major, i_lua_version_minor );
   lua_setfield( L, -2, "version" );
   
   /* Register the "mb" table as a global. */
   lua_setglobal( L, "mb" );
   
   /* C Functions. */
   lua_register( L, "echo", ilua_echo );
   lua_register( L, "send", ilua_send );
   lua_register( L, "debug", ilua_debug );
   lua_register( L, "prefix", ilua_prefix );
   lua_register( L, "suffix", ilua_suffix );
   lua_register( L, "insert", ilua_insert );
   lua_register( L, "replace", ilua_replace );
   lua_register( L, "hide_line", ilua_hide_line );
   lua_register( L, "set_line", ilua_set_line );
   lua_register( L, "get_color_at", ilua_get_color_at );
   lua_register( L, "show_prompt", ilua_show_prompt );
   lua_register( L, "cmp", ilua_cmp );
   lua_register( L, "add_timer", ilua_add_timer );
   lua_register( L, "del_timer", ilua_del_timer );
   lua_register( L, "gettimeofday", ilua_gettimeofday );
   lua_register( L, "regex_compile", ilua_regex_compile );
   lua_register( L, "regex_match", ilua_regex_match );
}



void ilua_open_mbcolours( lua_State *L )
{
   lua_createtable( L, 0, 16 );
   
   lua_pushstring( L, C_d );
   lua_setfield( L, -2, "d" );
   lua_pushstring( L, C_r );
   lua_setfield( L, -2, "r" );
   lua_pushstring( L, C_g );
   lua_setfield( L, -2, "g" );
   lua_pushstring( L, C_y );
   lua_setfield( L, -2, "y" );
   lua_pushstring( L, C_b );
   lua_setfield( L, -2, "b" );
   lua_pushstring( L, C_m );
   lua_setfield( L, -2, "m" );
   lua_pushstring( L, C_c );
   lua_setfield( L, -2, "c" );
   lua_pushstring( L, C_0 );
   lua_setfield( L, -2, "x" );
   lua_pushstring( L, C_D );
   lua_setfield( L, -2, "D" );
   lua_pushstring( L, C_R );
   lua_setfield( L, -2, "R" );
   lua_pushstring( L, C_G );
   lua_setfield( L, -2, "G" );
   lua_pushstring( L, C_Y );
   lua_setfield( L, -2, "Y" );
   lua_pushstring( L, C_B );
   lua_setfield( L, -2, "B" );
   lua_pushstring( L, C_M );
   lua_setfield( L, -2, "M" );
   lua_pushstring( L, C_C );
   lua_setfield( L, -2, "C" );
   lua_pushstring( L, C_W );
   lua_setfield( L, -2, "W" );
   
   /* Register the "C" colour table as a global. */
   lua_setglobal( L, "C" );
}


