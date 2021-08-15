#ifndef PORTABLE_GET_RANDOM_H
#define PORTABLE_GET_RANDOM_H
#pragma once

#include <stddef.h>

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    #ifdef WIN_EXPORT
        // Exporting...
        #ifdef __GNUC__
        #define PORTABLE_GET_RANDOM_EXPORT __attribute__ ((dllexport))
        #else
        #define PORTABLE_GET_RANDOM_EXPORT __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
        #endif
    #else
        #ifdef __GNUC__
        #define PORTABLE_GET_RANDOM_EXPORT __attribute__ ((dllimport))
        #else
        #define PORTABLE_GET_RANDOM_EXPORT __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
        #endif
    #endif
    #define PORTABLE_GET_RANDOM_PRIVATE
#else
    #if (defined(__GNUC__) && __GNUC__ >= 4) || defined(__clang__)
        #define PORTABLE_GET_RANDOM_EXPORT  __attribute__ ((visibility ("default")))
        #define PORTABLE_GET_RANDOM_PRIVATE __attribute__ ((visibility ("hidden")))
    #else
        #define PORTABLE_GET_RANDOM_EXPORT extern
        #define PORTABLE_GET_RANDOM_PRIVATE
    #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

PORTABLE_GET_RANDOM_EXPORT int portable_get_random(unsigned char *buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif // PORTABLE_GET_RANDOM_H
