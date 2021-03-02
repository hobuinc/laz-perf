/*
===============================================================================

  FILE:  util.hpp
  
  CONTENTS:
    Utility classes

  PROGRAMMERS:

    martin.isenburg@rapidlasso.com  -  http://rapidlasso.com
    uday.karan@gmail.com - Hobu, Inc.
  
  COPYRIGHT:
  
    (c) 2007-2014, martin isenburg, rapidlasso - tools to catch reality
    (c) 2014, Uday Verma, Hobu, Inc.

    This is free software; you can redistribute and/or modify it under the
    terms of the GNU Lesser General Licence as published by the Free Software
    Foundation. See the COPYING file for more information.

    This software is distributed WITHOUT ANY WARRANTY and without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  
  CHANGE HISTORY:
  
===============================================================================
*/

#ifndef __util_hpp__
#define __util_hpp__

#include <array>
#include <cstdint>
#include <cstdlib>

#ifdef NDEBUG
#define LAZDEBUG(e) ((void)0)
#else
#define LAZDEBUG(e) (void)(e)
#endif

namespace laszip {
namespace utils {

inline int32_t sum(const uint8_t *buf, uint32_t size)
{
    int32_t total = 0;
    while (size--)
        total += *buf++;
    return total;
}

struct Summer
{
    Summer() : sum(0), cnt(0)
    {}

    template <typename T>
    void add(const T& t)
    {
        const uint8_t *b = reinterpret_cast<const uint8_t *>(&t);
        sum = utils::sum(b, sizeof(T));
        cnt++;
    }

    void add(uint8_t *b, size_t size)
    {
        sum = utils::sum(b, size);
    }

    uint32_t value()
    {
        uint32_t v = sum;
        sum = 0;
        return v;
    }

    uint32_t count()
    {
        uint32_t c = cnt;
        cnt = 0;
        return c;
    }

    uint32_t sum;
    uint32_t cnt;
};

#define ALIGN 64

static inline void *aligned_malloc(int size)
{
    void *mem = malloc(size+ALIGN+sizeof(void*));
    void **ptr = (void**)(( ((uintptr_t)mem)+ALIGN+sizeof(void*) ) & ~(uintptr_t)(ALIGN-1) );
    ptr[-1] = mem;
    return ptr;
}

static inline void aligned_free(void *ptr)
{
    free(((void**)ptr)[-1]);
}

template<typename T>
class streaming_median
{
public:
    std::array<T, 5> values;
    BOOL high;

    void init() {
        values.fill(T(0));
        high = true;
    }

    inline void add(const T& v)
    {
        if (high) {
            if (v < values[2]) {
                values[4] = values[3];
                values[3] = values[2];
                if (v < values[0]) {
                    values[2] = values[1];
                    values[1] = values[0];
                    values[0] = v;
                }
                else if (v < values[1]) {
                    values[2] = values[1];
                    values[1] = v;
                }
                else {
                    values[2] = v;
                }
            }
            else {
                if (v < values[3]) {
                    values[4] = values[3];
                    values[3] = v;
                }
                else {
                    values[4] = v;
                }
                high = false;
            }
        }
        else {
            if (values[2] < v) {
                values[0] = values[1];
                values[1] = values[2];
                if (values[4] < v) {
                    values[2] = values[3];
                    values[3] = values[4];
                    values[4] = v;
                }
                else if (values[3] < v) {
                    values[2] = values[3];
                    values[3] = v;
                }
                else {
                    values[2] = v;
                }
            }
            else {
                if (values[1] < v) {
                    values[0] = values[1];
                    values[1] = v;
                }
                else {
                    values[0] = v;
                }
                high = true;
            }
        }
    }

    inline T get() const {
        return values[2];
    }

    streaming_median() {
        init();
    }
};

} // namespace utils
} // namespace laszip

#endif // __util_hpp__
