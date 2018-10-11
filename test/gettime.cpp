// Copyright (c) Team CharLS. All rights reserved. See the accompanying "LICENSE.md" for licensed use.

#include "gettime.h"

// for best accuracy, getTime is implemented platform dependent.
// to avoid a global include of windows.h, this is a separate file.


#if defined(_WIN32)

#include <Windows.h>

// returns a point in time in milliseconds (can only be used for time differences, not an absolute time)
double getTime() noexcept
{
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);

    return (static_cast<double>(time.LowPart) * 1000.0) / static_cast<double>(freq.LowPart);
}

#else

#include <sys/time.h>
double getTime() noexcept
{
    timeval t;
    gettimeofday(&t, nullptr);

    return (t.tv_sec * 1000000.0 + t.tv_usec) / 1000.0;
}

#endif
