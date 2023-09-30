#ifndef STACK_HPP
#define STACK_HPP

#include <stddef.h>
#include <stdint.h>
#include "Utils.hpp"

#define HASH_PROTECTION
// #undef HASH_PROTECTION

typedef int StackElement_t;

typedef size_t canary_t;

#define STACK_PRINTF_SPECIFIER "%d"

const size_t STACK_GROW_FACTOR = 2;

const size_t DEFAULT_CAPACITY = 8;

const StackElement_t POISON = INT32_MAX;

struct Stack;

struct StackOption
{
    Stack* stack;
    ErrorCode error;
};

struct StackElementOption
{
    StackElement_t value;
    ErrorCode error;
};

#define StackInit()                                                             \
({                                                                              \
    Owner _owner = {__FILE__, __LINE__, __func__};                              \
    _stackInit(_owner);                                                         \
})

#define StackDump(where, stack)                                                 \
do                                                                              \
{                                                                               \
    Owner caller = {__FILE__, __LINE__, __func__};                              \
    _stackDump(where, stack, #stack, &caller);                                  \
} while (0);                    

StackOption _stackInit(Owner owner);

ErrorCode StackDestructor(Stack* stack);

ErrorCode CheckStackIntegrity(Stack* stack);

ErrorCode _stackDump(FILE* where, Stack* stack, const char* stackName, Owner* caller);

ErrorCode Push(Stack* stack, StackElement_t value);

StackElementOption Pop(Stack* stack);

#endif
