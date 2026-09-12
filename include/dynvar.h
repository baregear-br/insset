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

#ifndef DYNVAR_H
#define DYNVAR_H

#include <stdint.h>

#define VECTOR_FORMULA(var, idx)    ((void *)((char *)(var)->address + ((idx) * (var)->sizePerBlks)))
#define MAX_STACK_SIZE (64 * 1024)
typedef char lgr[MAX_STACK_SIZE];

typedef struct {
    uintptr_t       address;
    unsigned int    length;
} dynvar;

typedef struct {
    uintptr_t       address;
    unsigned int    sizePerBlks;
    unsigned int    count;
} vector;

typedef enum {
    DYNVAR_SUCCESS,
    ILVAR,
    BFROVRFLW,
    NOT_FOUND,
    OOM
} DYNVAR_CODE;

#ifdef __cplusplus
extern "C" {
#endif

extern vector emvec;
extern void vectorInit(vector* var, unsigned int length);
extern DYNVAR_CODE vectorAppend(vector* var, lgr source);
extern DYNVAR_CODE vectorGetValue(vector* var, int index, lgr* result);
extern int vectorFind(vector* var, lgr value);
extern DYNVAR_CODE vectorDelete(vector* var, int index);
extern void vectorDeleteAll(vector* var);

extern DYNVAR_CODE setValue(dynvar* var, lgr value);
extern void getValue(dynvar var, lgr* out);

#ifdef __cplusplus
}
#endif

#endif // DYNVAR_H