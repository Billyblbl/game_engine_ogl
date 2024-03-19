#ifndef GTMX_MODULE
# define GTMX_MODULE

#include <stdlib.h>

#include <tmx_utils.h>
#include <tmx.h>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif


#include <tmx_err.c>
#include <tmx_hash.c>
#include <tmx_mem.c>
#include <tmx_utils.c>
#include <tmx_xml.c>
#include <tmx.c>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif
