#ifndef STACK_HPP
#define STACK_HPP

#include <stddef.h>
#include <stdint.h>
#include "Utils.hpp"

typedef int StackElement_t;

typedef size_t canary_t;

#define PRINTF_SPECIFIER "%d"

const size_t STACK_GROW_FACTOR = 2;

const size_t DEFAULT_CAPACITY = 8;

const StackElement_t POISON = INT32_MAX;

struct Stack
{
    canary_t leftCanary;

    size_t realDataSize;
    StackElement_t* data;

    Owner owner;
    size_t size;
    size_t capacity;

    canary_t rightCanary;
};

struct StackOption
{
    Stack stack;
    ErrorCode error;
};

struct StackElementOption
{
    StackElement_t value;
    ErrorCode error;
};

#define StackInit(canary)                                                       \
({                                                                              \
    Owner _owner = {__FILE__, __LINE__, __func__};                              \
    _stackInit(_owner, canary);                                                 \
})

#define StackDump(where, stack, canary)                                         \
do                                                                              \
{                                                                               \
    Owner caller = {__FILE__, __LINE__, __func__};                              \
    _stackDump(where, stack, #stack, &caller, canary);                          \
} while (0);                    

StackOption _stackInit(Owner owner, canary_t canary);

ErrorCode StackDestructor(Stack* stack);

ErrorCode CheckStackIntegrity(const Stack* stack, canary_t canary);

ErrorCode _stackDump(FILE* where, const Stack* stack, const char* stackName, Owner* caller, canary_t canary);

ErrorCode Push(Stack* stack, StackElement_t value, canary_t canary);

StackElementOption Pop(Stack* stack, canary_t canary);

#endif
