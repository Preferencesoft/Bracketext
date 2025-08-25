// lua_config.h
#ifndef LUA_CONFIG_H
#define LUA_CONFIG_H

// Determine the largest available integer type
#if defined(INT64_MAX) || defined(INT64_C)
    #include <stdint.h>
    #define LUA_INTEGER int64_t 
    #define LUA_INTEGER_FORMAT PRId64
#elif defined(LLONG_MAX) || defined(LONG_LONG_MAX)
    #define LUA_INTEGER long long
    #define LUA_INTEGER_FORMAT "lld"
#elif defined(_MSC_VER)
    #define LUA_INTEGER __int64
    #define LUA_INTEGER_FORMAT "I64d"
#else
    #define LUA_INTEGER long
    #define LUA_INTEGER_FORMAT "ld"
#endif

#endif // LUA_CONFIG_H