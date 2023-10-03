#include <time.h>
#include "Stack.hpp"

typedef unsigned int hash_t;

static const hash_t HASH_SEED = 0xBEBDA;

#ifdef DEBUG
    #define RETURN_ERROR(error, stack)                                        \
    do                                                                        \
    {                                                                         \
        __typeof__(error) _tempError = error;                                 \
        __typeof__(stack) _tempStack = stack;                                 \
                                                                            \
        if (_tempError)                                                       \
        {                                                                     \
            StackDump(logFilePath, _tempStack, _tempError);                       \
            return _tempError;                                                \
        }                                                                     \
    } while (0);                
#else
    #define RETURN_ERROR(...)
#endif

#ifdef CANARY_PROTECTION
    static size_t _getRandomCanary()
    {
        srand((unsigned int)time(NULL));
        return ((canary_t)rand() << 32) + (canary_t)rand();
    }

    static const size_t _CANARY = _getRandomCanary();
#endif

struct Stack
{
    #ifdef CANARY_PROTECTION
    canary_t leftCanary;
    #endif

    size_t realDataSize;
    StackElement_t* data;

    Owner owner;
    size_t size;
    size_t capacity;

    #ifdef HASH_PROTECTION
    hash_t hashData;
    hash_t hashStack;
    #endif

    #ifdef CANARY_PROTECTION
    canary_t rightCanary;
    #endif
};

static ErrorCode _checkCanary(const Stack* stack);

static ErrorCode _checkHash(Stack* stack);

static ErrorCode _stackRealloc(Stack* stack);

static canary_t* _getLeftDataCanaryPtr(const StackElement_t* data);

static canary_t* _getRightDataCanaryPtr(const StackElement_t* data, size_t realDataSize);

static ErrorCode _reHashify(Stack* stack);

static hash_t _calculateDataHash(const Stack* stack);

static hash_t _calculateStackHash(Stack* stack);

StackOption _stackInit(Owner owner)
{
    Stack* stack = (Stack*)calloc(1, sizeof(Stack));

    if (!stack)
        return {NULL, ERROR_NO_MEMORY};

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

    #ifdef CANARY_PROTECTION
    *_getLeftDataCanaryPtr(data) = _CANARY;

    *_getRightDataCanaryPtr(data, realDataSize) = _CANARY;
    #endif

    ErrorCode error = EVERYTHING_FINE;
    if (!data)
    {
        error = ERROR_NO_MEMORY;
        stack->capacity = 0;
        realDataSize = 0;
    }

    for (size_t i = 0; i < stack->capacity; i++)
        data[i] = POISON;
    
    stack->data = data;
    stack->realDataSize = realDataSize;

    #ifdef HASH_PROTECTION
    _reHashify(stack);
    #endif

    return {stack, error};
}

ErrorCode StackDestructor(Stack* stack)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    ErrorCode error = CheckStackIntegrity(stack);

    RETURN_ERROR(error, stack);

    if (stack->data == NULL)
        error = ERROR_NO_MEMORY;
    else
        free((void*)stack->data - sizeof(canary_t));

    stack->size = POISON;
    stack->capacity = POISON;

    stack->data = NULL;

    stack->owner = {};

    #ifdef HASH_PROTECTION
    stack->hashData = POISON;
    stack->hashStack = POISON;
    #endif

    #ifdef CANARY_PROTECTION
    stack->leftCanary = POISON;
    stack->rightCanary = POISON;
    #endif
    
    free((void*)stack);

    return error;
}

ErrorCode CheckStackIntegrity(Stack* stack)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    if (stack->capacity < stack->size)
        return ERROR_INDEX_OUT_OF_BOUNDS;
    if (!stack->data || stack->capacity < DEFAULT_CAPACITY)
        return ERROR_NO_MEMORY;
    
    #ifdef CANARY_PROTECTION
        RETURN_ERROR(_checkCanary(stack), stack);
    #endif

    #ifdef HASH_PROTECTION
        return _checkHash(stack);
    #else
        return EVERYTHING_FINE;
    #endif
}

ErrorCode _stackDump(FILE* where, Stack* stack, const char* stackName, Owner* caller, ErrorCode error)
{
    fprintf(where,
            "Stack[%p] \"%s\" from %s(%zu) %s()\n"
            "called from %s(%zu) %s()\n"
            "Stack condition - %s\n"

            #ifdef HASH_PROTECTION
            "Data hash = %u (should be %u)\n"
            "Stack hash = %u (should be %u)\n"
            #endif

            #ifdef CANARY_PROTECTION
            "Left stack canary = %zu (should be %zu)\n"
            "Right stack canary = %zu (should be %zu)\n"
            #endif

            "{\n"
            "    size = %zu\n"
            "    capacity = %zu\n"
            "    data[%p]\n"

            #ifdef CANARY_PROTECTION
            "    Left data canary = %zu (should be %zu)\n",
            #endif

            stack, stackName, stack->owner.fileName, stack->owner.line, stack->owner.name,
            caller->fileName, caller->line, caller->name,
            ERROR_CODE_NAMES[error],

            #ifdef HASH_PROTECTION
            stack->hashData, _calculateDataHash(stack),
            stack->hashStack, _calculateStackHash(stack),
            #endif

            #ifdef CANARY_PROTECTION
            stack->leftCanary, _CANARY,
            stack->rightCanary, _CANARY,
            #endif

            stack->size,
            stack->capacity,
            stack->data,

            #ifdef CANARY_PROTECTION
            *_getLeftDataCanaryPtr(stack->data), _CANARY
            #endif
            );

    const StackElement_t* data = stack->data;
    size_t capacity = stack->capacity;

    for (size_t i = 0; i < capacity; i++)
    {
        fprintf(where, "    ");

        if (data[i] != POISON)
            fprintf(where, "*");
        else
            fprintf(where, " ");

        fprintf(where, "[%zu] = " STACK_PRINTF_SPECIFIER "\n", i, data[i]);
    }


    #ifdef CANARY_PROTECTION
    fprintf(where, "    Right data canary = %zu (should be %zu)\n}\n",
            *_getRightDataCanaryPtr(stack->data, stack->realDataSize), _CANARY);

    RETURN_ERROR(_checkCanary(stack), stack);
    #endif

    return EVERYTHING_FINE;
}

ErrorCode Push(Stack* stack, StackElement_t value)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    RETURN_ERROR(CheckStackIntegrity(stack), stack);

    RETURN_ERROR(_stackRealloc(stack), stack);

    stack->data[stack->size] = value;
    stack->size++;

    #ifdef CANARY_PROTECTION
    RETURN_ERROR(_checkCanary(stack), stack);
    #endif

    #ifdef HASH_PROTECTION
    _reHashify(stack);
    #endif

    stack->data[stack->size - 1] = 0xbebda;

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

    error = _reHashify(stack);

    return {value, error};
}

static ErrorCode _stackRealloc(Stack* stack)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    RETURN_ERROR(CheckStackIntegrity(stack), stack);

    if ((stack->size == stack->capacity) ||
        (stack->size > 0 && stack->capacity > DEFAULT_CAPACITY &&
         stack->size <= stack->capacity / (STACK_GROW_FACTOR * STACK_GROW_FACTOR)))
    {
        size_t numberOfElementsToAdd = stack->capacity * (STACK_GROW_FACTOR - 1);

        StackElement_t* oldData = (StackElement_t*)((void*)stack->data - sizeof(canary_t));
        size_t realDataSize = stack->realDataSize;

        #ifdef CANARY_PROTECTION
        canary_t* oldRightCanaryPtr = _getRightDataCanaryPtr(stack->data, stack->realDataSize);
        canary_t oldRightCanary     = *oldRightCanaryPtr;

        *oldRightCanaryPtr = 0;
        #endif

        realDataSize += numberOfElementsToAdd * sizeof(StackElement_t);

        StackElement_t* newData = (StackElement_t*)(realloc((void*)oldData, realDataSize) + sizeof(canary_t));

        if (newData == NULL)
            return ERROR_NO_MEMORY;

        #ifdef CANARY_PROTECTION
        *_getRightDataCanaryPtr(newData, realDataSize) = oldRightCanary;
        #endif

        stack->data = newData;
        stack->realDataSize = realDataSize;
        stack->capacity += numberOfElementsToAdd;

        for (size_t i = stack->size; i < stack->capacity; i++)
            newData[i] = POISON;
    }

    #ifdef CANARY_PROTECTION
    RETURN_ERROR(_checkCanary(stack), stack);
    #endif

    #ifdef HASH_PROTECTION
    RETURN_ERROR(_reHashify(stack), stack);
    #endif

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
    MyAssertSoft(stack, ERROR_NULLPTR);

    hash_t hashData = _calculateDataHash(stack);
    hash_t hashStack = _calculateStackHash(stack);

    stack->hashData = hashData;
    stack->hashStack = hashStack;

    return EVERYTHING_FINE;
}

static hash_t _calculateDataHash(const Stack* stack)
{
    return CalculateHash((const void*)stack->data - sizeof(canary_t), stack->realDataSize, HASH_SEED);
}

static hash_t _calculateStackHash(Stack* stack)
{
    hash_t oldHashData = stack->hashData;
    hash_t oldHashStack = stack->hashStack;

    stack->hashData = 0;
    stack->hashStack = 0;

    hash_t stackHash = CalculateHash((const void*)stack, sizeof(*stack), HASH_SEED);

    stack->hashData = oldHashData;
    stack->hashStack = oldHashStack;

    return stackHash;
}

static ErrorCode _checkHash(Stack* stack)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    if (stack->hashData != _calculateDataHash(stack) || stack->hashStack != _calculateStackHash(stack))
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
