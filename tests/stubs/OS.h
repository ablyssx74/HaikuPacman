// Stand-ins for the Haiku headers, so the game's logic can be tested on a computer without Haiku (see tests/ghost_test.cpp).
#ifndef STUB_OS_H
#define STUB_OS_H
#include <stdint.h>
typedef int32_t int32;
typedef int32_t thread_id;
typedef int32 (*thread_func)(void*);
#define B_LOW_PRIORITY 5
inline thread_id spawn_thread(thread_func, const char*, int32, void*) { return -1; }
inline int32 resume_thread(thread_id) { return 0; }
#endif
