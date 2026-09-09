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

#include <threading.h>
#include <dynvar.h>
#include <pthread.h>
#include <runtime.h>
#include <string.h>
#include <stdarg.h>

vector runningThreads;
static pthread_mutex_t threadMutex = PTHREAD_MUTEX_INITIALIZER;
static int nextThreadId = 0;

typedef struct {
    int id;
    bool isRunning;
    pthread_t thread;
} ThreadInfo;

typedef struct {
    void (*callback)(void*);
    void* arg;
    va_list args;
} ThreadArgs;

void threadingInit() {
    vectorInit(&runningThreads, sizeof(ThreadInfo));
}

static void* threadWrapper(void* arg) {
    ThreadArgs* threadArgs = (ThreadArgs*)arg;
    threadArgs->callback(threadArgs->arg);
    ffree(threadArgs, sizeof(ThreadArgs));
    return NULL;
}

int threadNew(void callback(void*), ...) {
    ThreadInfo info;
    info.id = __sync_fetch_and_add(&nextThreadId, 1);
    info.isRunning = true;

    ThreadArgs* args = (ThreadArgs*)falloc(NULL, sizeof(ThreadArgs));
    if (!args) return -1;

    args->callback = callback;
    args->arg = NULL;

    va_list va;
    va_start(va, callback);
    // Check if there's an argument
    void* arg = va_arg(va, void*);
    if (arg != NULL)
        args->arg = arg;
    va_end(va);

    int result = pthread_create(&info.thread, NULL, threadWrapper, args);
    if (result != 0) {
        ffree(args, sizeof(ThreadArgs));
        return -1;
    }

    pthread_mutex_lock(&threadMutex);
    lgr infoBuffer;
    memcpy(infoBuffer, &info, sizeof(ThreadInfo));
    vectorAppend(&runningThreads, infoBuffer);
    pthread_mutex_unlock(&threadMutex);

    return info.id;
}

int threadJoin(int threadId) {
    pthread_mutex_lock(&threadMutex);
    for (unsigned int i = 0; i < runningThreads.count; i++) {
        ThreadInfo* info = (ThreadInfo*)VECTOR_FORMULA(&runningThreads, i);
        if (info->id == threadId && info->isRunning) {
            pthread_mutex_unlock(&threadMutex);
            pthread_join(info->thread, NULL);

            pthread_mutex_lock(&threadMutex);
            info->isRunning = false;
            pthread_mutex_unlock(&threadMutex);
            return 0;
        }
    }
    pthread_mutex_unlock(&threadMutex);
    return -1;
}

int threadDetach(int threadId) {
    pthread_mutex_lock(&threadMutex);
    for (unsigned int i = 0; i < runningThreads.count; i++) {
        ThreadInfo* info = (ThreadInfo*)VECTOR_FORMULA(&runningThreads, i);
        if (info->id == threadId && info->isRunning) {
            int result = pthread_detach(info->thread);
            if (result == 0) {
                info->isRunning = false;
                pthread_mutex_unlock(&threadMutex);
                return 0;
            }
            pthread_mutex_unlock(&threadMutex);
            return -1;
        }
    }
    pthread_mutex_unlock(&threadMutex);
    return -1;
}

bool threadIsRunning(int threadId) {
    pthread_mutex_lock(&threadMutex);
    for (unsigned int i = 0; i < runningThreads.count; i++) {
        ThreadInfo* info = (ThreadInfo*)VECTOR_FORMULA(&runningThreads, i);
        if (info->id == threadId) {
            bool running = info->isRunning;
            pthread_mutex_unlock(&threadMutex);
            return running;
        }
    }
    pthread_mutex_unlock(&threadMutex);
    return false;
}

void threadCleanup() {
    pthread_mutex_lock(&threadMutex);
    for (unsigned int i = 0; i < runningThreads.count; i++) {
        ThreadInfo* info = (ThreadInfo*)VECTOR_FORMULA(&runningThreads, i);
        if (info->isRunning) {
            pthread_join(info->thread, NULL);
            info->isRunning = false;
        }
    }
    vectorDeleteAll(&runningThreads);
    pthread_mutex_unlock(&threadMutex);
}