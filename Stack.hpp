#ifndef STACK_HPP
#define STACK_HPP

#include <stddef.h>
#include <stdint.h>
#include "Utils.hpp"

#include "HashSettings.settings"

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

#define StackInit()                                                                      \
({                                                                                       \
    Owner _owner = {__FILE__, __LINE__, __func__};                                       \
    _stackInit(_owner);                                                                  \
})

#define StackDump(where, stack, error)                                                   \
do                                                                                       \
{                                                                                        \
    if (stack)                                                                           \
    {                                                                                    \
        Owner _caller = {__FILE__, __LINE__, __func__};                                  \                  
        FILE* _logFile = fopen(where, "a");                                              \
        if (_logFile)                                                                    \
            _stackDump(_logFile, stack, #stack, &_caller, error);                        \
        fclose(_logFile);                                                                \
    }                                                                                    \
} while (0);                        

StackOption _stackInit(Owner owner);

ErrorCode StackDestructor(Stack* stack);

ErrorCode CheckStackIntegrity(Stack* stack);

ErrorCode _stackDump(FILE* where, Stack* stack, const char* stackName, Owner* caller, ErrorCode error);

ErrorCode Push(Stack* stack, StackElement_t value);

StackElementOption Pop(Stack* stack);

#endif
