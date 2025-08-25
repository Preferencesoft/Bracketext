// lua_types.h
#ifndef LUA_TYPES_H
#define LUA_TYPES_H

// Define fixed-size integer types for Lua
#if defined(HAVE_STDINT_H)
    #include <stdint.h>
#else
    // Portable definitions
    typedef unsigned char  uint8_t;
    typedef unsigned short uint16_t;
    #if defined(_MSC_VER)
        typedef unsigned __int32 uint32_t;
        typedef unsigned __int64 uint64_t;
    #else
        typedef unsigned int       uint32_t;
        typedef unsigned long long uint64_t;
    #endif
#endif

#endif // LUA_TYPES_H