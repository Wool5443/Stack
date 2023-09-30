#include <time.h>
#include "Stack.hpp"

typedef unsigned int hash_t;

#define RETURN_ERROR(error) if (error) return error;

static size_t _getRandomCanary()
{
    srand((unsigned int)time(NULL));
    return ((canary_t)rand() << 32) + (canary_t)rand();
}

static const size_t _CANARY = _getRandomCanary();

struct Stack
{
    canary_t leftCanary;

    size_t realDataSize;
    StackElement_t* data;

    Owner owner;
    size_t size;
    size_t capacity;

    hash_t hashData;
    hash_t hashStack;

    canary_t rightCanary;
};

static ErrorCode _checkCanary(const Stack* stack);

static ErrorCode _checkHash(Stack* stack);

static ErrorCode _stackRealloc(Stack* stack);

static canary_t* _getLeftDataCanaryPtr(const StackElement_t* data);

static canary_t* _getRightDataCanaryPtr(const StackElement_t* data, size_t realDataSize);

static ErrorCode _reHashify(Stack* stack);

StackOption _stackInit(Owner owner)
{
    Stack* stack = (Stack*)calloc(1, sizeof(Stack));
    *stack = 
    {
        .leftCanary = _CANARY,
        .owner = owner,
        .size = 0,
        .capacity = DEFAULT_CAPACITY,
        .rightCanary = _CANARY
    };

    size_t realDataSize = DEFAULT_CAPACITY * sizeof(StackElement_t) + 2 * sizeof(canary_t);

    StackElement_t* data = (StackElement_t*)calloc(realDataSize, 1);
    data = (StackElement_t*)((void*)data + sizeof(canary_t));

    *_getLeftDataCanaryPtr(data) = _CANARY;

    *_getRightDataCanaryPtr(data, realDataSize) = _CANARY;

    ErrorCode error = EVERYTHING_FINE;
    if (!data)
        error = ERROR_NO_MEMORY;

    for (size_t i = 0; i < stack->capacity; i++)
        data[i] = POISON;
    
    stack->data = data;
    stack->realDataSize = realDataSize;

    _reHashify(stack);

    return {stack, error};
}

ErrorCode StackDestructor(Stack* stack)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    free((void*)stack->data - sizeof(canary_t));

    stack->size = POISON;
    stack->capacity = POISON;

    stack->data = NULL;

    stack->owner = {};

    stack->hashData = POISON;
    stack->hashStack = POISON;

    stack->leftCanary = POISON;
    stack->rightCanary = POISON;
    
    free((void*)stack);

    return EVERYTHING_FINE;
}

ErrorCode CheckStackIntegrity(Stack* stack)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    if (stack->capacity < stack->size)
        return ERROR_INDEX_OUT_OF_BOUNDS;
    if (!stack->data)
        return ERROR_NO_MEMORY;
    
    RETURN_ERROR(_checkCanary(stack));

    return _checkHash(stack);
}

ErrorCode _stackDump(FILE* where, Stack* stack, const char* stackName, Owner* caller)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    ErrorCode error = CheckStackIntegrity(stack);

    fprintf(where, 
            "Stack[%p] \"%s\" from %s(%zu) %s()\n"
            "called from %s(%zu) %s()\n"
            "Stack condition - %d\n"
            "Data hash = %u\n"
            "Stack hash = %u\n"
            "Left stack canary = %zu (should be %zu)\n"
            "Right stack canary = %zu (should be %zu)\n"
            "{\n"
            "    size = %zu\n"
            "    capacity = %zu\n"
            "    data[%p]\n"
            "    Left data canary = %zu (should be %zu)\n",
            stack, stackName + 1, stack->owner.fileName, stack->owner.line, stack->owner.name,
            caller->fileName, caller->line, caller->name,
            error,
            stack->hashData,
            stack->hashStack,
            stack->leftCanary, _CANARY,
            stack->rightCanary, _CANARY,
            stack->size,
            stack->capacity,
            stack->data,
            *_getLeftDataCanaryPtr(stack->data), _CANARY
            );

    const StackElement_t* data = stack->data;
    size_t capacity = stack->capacity;

    for (size_t i = 0; i < capacity; i++)
    {
        fprintf(where, "    ");

        if (data[i] != POISON)
        {
            // SetConsoleColor(where, GREEN);
            fprintf(where, "*");
        }
        else
        {
            // SetConsoleColor(where, RED);
            fprintf(where, " ");
        }

        fprintf(where, "[%zu] = " STACK_PRINTF_SPECIFIER "\n", i, data[i]);
    }

    // SetConsoleColor(where, WHITE);

    fprintf(where, "    Right data canary = %zu (should be %zu)\n}\n",
            *_getRightDataCanaryPtr(stack->data, stack->realDataSize), _CANARY);

    RETURN_ERROR(_checkCanary(stack));

    return EVERYTHING_FINE;
}

ErrorCode Push(Stack* stack, StackElement_t value)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    RETURN_ERROR(CheckStackIntegrity(stack));

    RETURN_ERROR(_stackRealloc(stack));

    stack->data[stack->size] = value;
    stack->size++;

    RETURN_ERROR(_checkCanary(stack));

    _reHashify(stack);

    return EVERYTHING_FINE;
}

StackElementOption Pop(Stack* stack)
{
    ErrorCode error = CheckStackIntegrity(stack);

    if (error)
        return {POISON, error};

    if (stack->size == 0)
        return {POISON, ERROR_INDEX_OUT_OF_BOUNDS};

    size_t size = stack->size - 1;
    StackElement_t* data = stack->data;

    StackElement_t value = data[size];
    
    data[size] = POISON;

    stack->size = size;

    error = _stackRealloc(stack);

    if (error)
        return {value, error};

    error = _checkCanary(stack);

    if (error)
        return {POISON, error};

    _reHashify(stack);

    return {value, error};
}

static ErrorCode _stackRealloc(Stack* stack)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    RETURN_ERROR(CheckStackIntegrity(stack));

    if ((stack->size == stack->capacity) ||
        (stack->size > 0 && stack->capacity > DEFAULT_CAPACITY &&
         stack->size <= stack->capacity / (STACK_GROW_FACTOR * STACK_GROW_FACTOR)))
    {
        size_t numberOfElementsToAdd = stack->capacity * (STACK_GROW_FACTOR - 1);

        StackElement_t* oldData = (StackElement_t*)((void*)stack->data - sizeof(canary_t));
        size_t realDataSize = stack->realDataSize;

        canary_t* oldRightCanaryPtr = _getRightDataCanaryPtr(stack->data, stack->realDataSize);
        canary_t oldRightCanary     = *oldRightCanaryPtr;

        *oldRightCanaryPtr = 0;

        realDataSize += numberOfElementsToAdd * sizeof(StackElement_t);

        StackElement_t* newData = (StackElement_t*)(realloc((void*)oldData, realDataSize) + sizeof(canary_t));

        if (newData == NULL)
            return ERROR_NO_MEMORY;

        *_getRightDataCanaryPtr(newData, realDataSize) = oldRightCanary;

        stack->data = newData;
        stack->realDataSize = realDataSize;
        stack->capacity += numberOfElementsToAdd;

        for (size_t i = stack->size; i < stack->capacity; i++)
            newData[i] = POISON;
    }

    RETURN_ERROR(_checkCanary(stack));

    _reHashify(stack);

    return EVERYTHING_FINE;
}

static ErrorCode _checkCanary(const Stack* stack)
{
    if (stack->leftCanary != _CANARY ||
               stack->rightCanary != _CANARY ||
               *_getLeftDataCanaryPtr(stack->data) != _CANARY ||
               *_getRightDataCanaryPtr(stack->data, stack->realDataSize) != _CANARY)
    {
        return ERROR_DEAD_CANARY;
    }
    
    return EVERYTHING_FINE;
}

static ErrorCode _reHashify(Stack* stack)
{
    #ifndef HASH_PROTECTION

    return EVERYTHING_FINE;

    #endif

    stack->hashData = 0;
    stack->hashStack = 0;

    hash_t hashData = MurmurHash2A((const void*)stack->data - sizeof(canary_t), stack->realDataSize, 0xBEBDA);
    hash_t hashStack = MurmurHash2A((const void*)stack, sizeof(*stack), 0xBEBDA);

    stack->hashData = hashData;
    stack->hashStack = hashStack;

    return EVERYTHING_FINE;
}

static ErrorCode _checkHash(Stack* stack)
{
    #ifndef HASH_PROTECTION
    
    return EVERYTHING_FINE;

    #endif

    hash_t oldHashData  = stack->hashData;
    hash_t oldHashStack = stack->hashStack;

    _reHashify(stack);

    if (oldHashData != stack->hashData || oldHashStack != stack->hashStack)
        return ERROR_BAD_HASH;
    
    return EVERYTHING_FINE;
}

static canary_t* _getLeftDataCanaryPtr(const StackElement_t* data)
{
    return (canary_t*)((void*)data - sizeof(canary_t));
}

static canary_t* _getRightDataCanaryPtr(const StackElement_t* data, size_t realDataSize)
{
    return (canary_t*)((void*)data + realDataSize - 2 * sizeof(canary_t));
}
