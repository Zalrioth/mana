#pragma once
#ifndef CORE_COMMON_H
#define CORE_COMMON_H

// TODO: Move this to custom file io code
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#define IS_WINDOWS
#include <direct.h>
#else
#include <unistd.h>
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define randf() (float)rand() / (float)(RAND_MAX)

#endif  // CORE_COMMON_H
