/*
 * baregear - A programming language compiler
 * Copyright (C) 2026 First Person
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef THREADING_H
#define THREADING_H

#include <stdbool.h>

/* enum {
    THREAD_STATE_NEW,        // Thread is created but not started
    THREAD_STATE_READY,      // Thread is waiting for a CPU core
    THREAD_STATE_RUNNING,    // Thread is actively executing code
    THREAD_STATE_BLOCKED,    // Thread is waiting on I/O or a Mutex
    THREAD_STATE_FINISHED,   // Thread has completed its execution
    THREAD_STATE_ERROR       // Thread crashed or failed to initialize
}; */

typedef struct {
    void* originalArg;
    int threadId;
} ThreadCallbackArgs;

#ifdef __cplusplus
extern "C" {
#endif

extern void threadingInit();
extern int threadNew(void callback(void*), ...);
extern int threadJoin(int threadId);
extern int threadDetach(int threadId);
extern bool threadIsRunning(int threadId);
extern void threadCleanup();

#ifdef __cplusplus
}
#endif

#endif // THREADING_H