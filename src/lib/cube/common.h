#ifndef CLOX_common_h
#define CLOX_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// #define UNICODE

#ifdef UNICODE
#include <wchar.h>
#endif

// #define NAN_TAGGING
// #define DEBUG_PRINT_CODE
// #define DEBUG_TRACE_EXECUTION
// #define DEBUG_TRACE_EXECUTION_ON_ERROR

// #define GC_DISABLED
#define GC_AUTO
// #define DEBUG_STRESS_GC
// #define DEBUG_LOG_GC
#define GC_HEAP_GROW_FACTOR 2

#define MAX_THREADS 8

#define UINT8_COUNT (UINT8_MAX + 1)
#define MAX_STRING_INPUT UINT8_COUNT

#endif