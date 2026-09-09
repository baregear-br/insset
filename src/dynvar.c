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

#include <runtime.h>
#include <dynvar.h>
#include <stdint.h>
#include <string.h>

vector emvec = {0, 0, 0};
void vectorInit(vector* var, unsigned int length) {
    if (var->address)
        vectorDeleteAll(var);
    else {
        var->address = (uintptr_t)0;
        var->count = 0;
    }

    var->sizePerBlks = length;
}

DYNVAR_CODE vectorAppend(vector* var, lgr data) {
    if (!var->sizePerBlks)
        return ILVAR;

    if (var->address == 0) {
        var->address = (uintptr_t)falloc(NULL, var->sizePerBlks);
        var->count = (uintptr_t)1;
    } else {
        var->count += (uintptr_t)1;
        var->address = (uintptr_t)frealloc((void*)var->address, (var->count - 1) * var->sizePerBlks, var->count * var->sizePerBlks);
    }

    memcpy(VECTOR_FORMULA(var, var->count - 1), (void*)data, var->sizePerBlks);
    return DYNVAR_SUCCESS;
}

long vectorGetValue(vector* var, int index) {
    if (index >= var->count || index < 0)
        return -BFROVRFLW;
    return (long)VECTOR_FORMULA(var, index);
}

int vectorFind(vector* var, long value) {
    for (int i = 0; i < var->count; i++) {
        if (vectorGetValue(var, i) == value)
            return i;
    }
    return -1;
}

DYNVAR_CODE vectorDelete(vector* var, int index) {
    if (index < 0 || index >= var->count)
        return BFROVRFLW;

    // Shift elements after the index down by one
    for (int i = index; i < var->count - 1; i++)
        memcpy(VECTOR_FORMULA(var, i), VECTOR_FORMULA(var, i + 1), var->sizePerBlks);

    // Reduce count and reallocate
    var->count -= (uintptr_t)1;
    if (var->count > 0)
        var->address = (uintptr_t)frealloc((void*)var->address, (var->count + 1) * var->sizePerBlks, var->count * var->sizePerBlks);
    else {
        ffree((void*)var->address, var->sizePerBlks);
        var->address = (uintptr_t)0;
    }
    return DYNVAR_SUCCESS;
}

void vectorDeleteAll(vector* var) {
    if (var->count == 0)
        return;
    ffree((void*)var->address, var->count * var->sizePerBlks);
    var->address = (uintptr_t)0;
    var->count = 0;
}

void setValue(dynvar* var, lgr value) {
    size_t inputLength = strlen(value);
    size_t copyLength = inputLength;

    if (copyLength >= MAX_STACK_SIZE)
        copyLength = MAX_STACK_SIZE - 1;

    if (var->address == 0)
        var->address = (uintptr_t)falloc(NULL, copyLength + 1);
    else if (copyLength + 1 > var->length) {
        var->address = (uintptr_t)frealloc((void*)var->address, var->length, copyLength + 1);
    }

    if (var->address != 0) {
        if (copyLength > 0)
            memcpy((void*)var->address, (void*)value, copyLength);
        ((char*)var->address)[copyLength] = '\0';
    }

    var->length = (unsigned int)copyLength;
}

void getValue(dynvar var, lgr out) {
    size_t copyLength = var.length;

    if (out == NULL)
        return;

    memset(out, 0, MAX_STACK_SIZE);
    if (var.address == 0)
        return;

    if (copyLength >= MAX_STACK_SIZE)
        copyLength = MAX_STACK_SIZE - 1;

    if (copyLength > 0)
        memcpy(out, (void*)var.address, copyLength);
    out[copyLength] = '\0';
}