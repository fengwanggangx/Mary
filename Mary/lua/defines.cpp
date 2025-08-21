#include "defines.h"

lua_getglobal_t func_lua_getglobal = nullptr;
lua_gettable_t func_lua_gettable = nullptr;
lua_getfield_t func_lua_getfield = nullptr;
lua_geti_t func_lua_geti = nullptr;
lua_rawget_t func_lua_rawget = nullptr;
lua_rawgeti_t func_lua_rawgeti = nullptr;
lua_rawgetp_t func_lua_rawgetp = nullptr;

luaL_newmetatable_t func_luaL_newmetatable = nullptr;

lua_pcallk_t func_lua_pcallk = nullptr;
luaL_loadfilex_t func_luaL_loadfilex = nullptr;
luaL_loadstring_t func_luaL_loadstring = nullptr;

lua_isnumber_t func_lua_isnumber = nullptr;
lua_iscfunction_t func_lua_iscfunction = nullptr;
lua_isuserdata_t func_lua_isuserdata = nullptr;
lua_type_t func_lua_type = nullptr;
lua_typename_t func_lua_typename = nullptr;

lua_tonumberx_t func_lua_tonumberx = nullptr;
lua_tointegerx_t func_lua_tointegerx = nullptr;
lua_toboolean_t func_lua_toboolean = nullptr;
lua_tolstring_t func_lua_tolstring = nullptr;
lua_rawlen_t func_lua_rawlen = nullptr;
lua_tocfunction_t func_lua_tocfunction = nullptr;
lua_touserdata_t func_lua_touserdata = nullptr;
lua_tothread_t func_lua_tothread = nullptr;
lua_topointer_t func_lua_topointer = nullptr;

luaL_checklstring_t func_luaL_checklstring = nullptr;
luaL_error_t func_luaL_error = nullptr;

lua_pushnil_t func_lua_pushnil = nullptr;
lua_pushnumber_t func_lua_pushnumber = nullptr;
lua_pushinteger_t func_lua_pushinteger = nullptr;
lua_pushlstring_t func_lua_pushlstring = nullptr;
lua_pushstring_t func_lua_pushstring = nullptr;
lua_pushvfstring_t func_lua_pushvfstring = nullptr;
lua_pushfstring_t func_lua_pushfstring = nullptr;
lua_pushcclosure_t func_lua_pushcclosure = nullptr;
lua_pushboolean_t func_lua_pushboolean = nullptr;
lua_pushlightuserdata_t func_lua_pushlightuserdata = nullptr;
lua_pushthread_t func_lua_pushthread = nullptr;

lua_gc_t func_lua_gc = nullptr;

lua_setglobal_t func_lua_setglobal = nullptr;
lua_settable_t func_lua_settable = nullptr;
lua_setfield_t func_lua_setfield = nullptr;
lua_seti_t func_lua_seti = nullptr;
lua_rawset_t func_lua_rawset = nullptr;
lua_rawseti_t func_lua_rawseti = nullptr;
lua_rawsetp_t func_lua_rawsetp = nullptr;
lua_setmetatable_t func_lua_setmetatable = nullptr;
lua_setiuservalue_t func_lua_setiuservalue = nullptr;

lua_absindex_t func_lua_absindex = nullptr;
lua_gettop_t func_lua_gettop = nullptr;
lua_settop_t func_lua_settop = nullptr;
lua_pushvalue_t func_lua_pushvalue = nullptr;
lua_rotate_t func_lua_rotate = nullptr;
lua_copy_t func_lua_copy = nullptr;
lua_checkstack_t func_lua_checkstack = nullptr;

luaL_newstate_t func_luaL_newstate = nullptr;
lua_close_t func_lua_close = nullptr;
luaL_openlibs_t func_luaL_openlibs = nullptr;
lua_isinteger_t func_lua_isinteger = nullptr;
lua_isstring_t func_lua_isstring = nullptr;