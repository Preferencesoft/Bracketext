#ifndef STDINT_PORTABLE_H
#define STDINT_PORTABLE_H

// Check if stdint.h is available
#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || \
    defined(__GNUC__) || defined(__clang__) || \
    (defined(_MSC_VER) && _MSC_VER >= 1600)
    
    #include <stdint.h>
    
#else

    // 8-bit types
    typedef signed char int8_t;
    typedef unsigned char uint8_t;

    // 16-bit types
    typedef signed short int16_t;
    typedef unsigned short uint16_t;

    // 32-bit types
    #if defined(_MSC_VER)
        typedef signed __int32 int32_t;
        typedef unsigned __int32 uint32_t;
    #else
        typedef signed int int32_t;
        typedef unsigned int uint32_t;
    #endif

    // 64-bit types
    #if defined(_MSC_VER)
        typedef signed __int64 int64_t;
        typedef unsigned __int64 uint64_t;
    #elif defined(__GNUC__) || defined(__clang__)
        typedef signed long long int64_t;
        typedef unsigned long long uint64_t;
    #else
        // Fallback - may not work on all platforms
        typedef signed long long int64_t;
        typedef unsigned long long uint64_t;
    #endif

#endif // stdint.h availability check

#endif // STDINT_PORTABLE_H