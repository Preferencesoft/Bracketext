// lua_cpp98_compat.hpp
#ifndef LUA_CPP98_COMPAT_HPP
#define LUA_CPP98_COMPAT_HPP

// Silence long long warnings for C++98
#if defined(__cplusplus) && __cplusplus < 201103L
    #if defined(__GNUC__) || defined(__clang__)
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wlong-long"
    #elif defined(_MSC_VER)
        #pragma warning(push)
        #pragma warning(disable: 4200) // Adjust MSVC warning number as needed
    #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <lua5.3/lua.h>
#include <lua5.3/lauxlib.h>
#include <lua5.3/lualib.h>

#ifdef __cplusplus
}
#endif

// Restore warnings
#if defined(__cplusplus) && __cplusplus < 201103L
    #if defined(__GNUC__) || defined(__clang__)
        #pragma GCC diagnostic pop
    #elif defined(_MSC_VER)
        #pragma warning(pop)
    #endif
#endif

#endif // LUA_CPP98_COMPAT_HPP