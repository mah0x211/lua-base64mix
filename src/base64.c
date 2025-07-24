/**
 *  Copyright 2014 Masatoshi Teruya. All rights reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 *  base64.c
 *  lua-base64mix
 *
 *  Created by Masatoshi Teruya on 14/12/06.
 *
 */

#include "base64mix.h"
#include "lua_errno.h"
#include <lauxlib.h>
#include <lua.h>
// include system headers
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define encode_lua(L, fn)                                                      \
    do {                                                                       \
        size_t len      = 0;                                                   \
        const char *str = luaL_checklstring(L, 1, &len);                       \
        char *b64       = fn((unsigned char *)str, &len);                      \
        if (b64) {                                                             \
            lua_pushlstring(L, b64, len);                                      \
            free((void *)b64);                                                 \
            return 1;                                                          \
        }                                                                      \
        lua_pushnil(L);                                                        \
        lua_errno_new(L, errno, "base64.encode");                              \
        return 2;                                                              \
    } while (0)

static int encode_std_lua(lua_State *L)
{
    encode_lua(L, b64m_encode_std);
}

static int encode_url_lua(lua_State *L)
{
    encode_lua(L, b64m_encode_url);
}

#define decode_lua(L, fn)                                                      \
    do {                                                                       \
        size_t len      = 0;                                                   \
        const char *b64 = luaL_checklstring(L, 1, &len);                       \
        char *str       = fn((unsigned char *)b64, &len);                      \
        if (str) {                                                             \
            lua_pushlstring(L, str, len);                                      \
            free((void *)str);                                                 \
            return 1;                                                          \
        }                                                                      \
        lua_pushnil(L);                                                        \
        lua_errno_new(L, errno, "base64.decode");                              \
        return 2;                                                              \
    } while (0)

static int decode_std_lua(lua_State *L)
{
    decode_lua(L, b64m_decode_std);
}

static int decode_url_lua(lua_State *L)
{
    decode_lua(L, b64m_decode_url);
}

static int decode_mix_lua(lua_State *L)
{
    decode_lua(L, b64m_decode_mix);
}

LUALIB_API int luaopen_base64mix(lua_State *L)
{
    // Load errno library for error handling
    lua_errno_loadlib(L);
    // Export Base64 functions
    lua_createtable(L, 0, 5);
    // standard Base64 encoding/decoding
    lua_pushcfunction(L, encode_std_lua);
    lua_setfield(L, -2, "encode");
    lua_pushcfunction(L, decode_std_lua);
    lua_setfield(L, -2, "decode");
    // URL-safe Base64 encoding/decoding
    lua_pushcfunction(L, encode_url_lua);
    lua_setfield(L, -2, "encodeURL");
    lua_pushcfunction(L, decode_url_lua);
    lua_setfield(L, -2, "decodeURL");
    // mixed Base64 decoding
    lua_pushcfunction(L, decode_mix_lua);
    lua_setfield(L, -2, "decodeMix");

    return 1;
}
