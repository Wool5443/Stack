#include "Stack.hpp"

ErrorCode _checkCanary(const Stack* stack, canary_t canary);

ErrorCode _stackRealloc(Stack* stack, canary_t canary);

canary_t* _getLeftDataCanaryPtr(const StackElement_t* data);

canary_t* _getRightDataCanaryPtr(const StackElement_t* data, size_t realDataSize);

StackOption _stackInit(Owner owner, canary_t canary)
{
    Stack stack = {.leftCanary = canary, .owner = owner,
                   .size = 0, .capacity = DEFAULT_CAPACITY, .rightCanary = canary};

    size_t realDataSize = DEFAULT_CAPACITY * sizeof(StackElement_t) + 2 * sizeof(canary_t);

    StackElement_t* data = (StackElement_t*)calloc(realDataSize, 1);
    data = (StackElement_t*)((void*)data + sizeof(canary_t));

    *_getLeftDataCanaryPtr(data) = canary;

    *_getRightDataCanaryPtr(data, realDataSize) = canary;

    ErrorCode error = EVERYTHING_FINE;
    if (!data)
        error = ERROR_NO_MEMORY;

    for (size_t i = 0; i < stack.capacity; i++)
        data[i] = POISON;
    
    stack.data = data;
    stack.realDataSize = realDataSize;

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

    return EVERYTHING_FINE;
}

ErrorCode CheckStackIntegrity(const Stack* stack, canary_t canary)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    if (stack->capacity < stack->size)
        return ERROR_INDEX_OUT_OF_BOUNDS;
    if (!stack->data)
        return ERROR_NO_MEMORY;

    _checkCanary(stack, canary);

    return EVERYTHING_FINE;
}

ErrorCode _stackDump(FILE* where, const Stack* stack, const char* stackName, Owner* caller, canary_t canary)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    ErrorCode error = CheckStackIntegrity(stack, canary);

    fprintf(where, 
            "Stack[%p] \"%s\" from %s(%zu) %s()\n"
            "called from %s(%zu) %s()\n"
            "Stack condition - %d\n"
            "Left stack canary = %zu\n"
            "Right stack canary = %zu\n"
            "{\n"
            "    size = %zu\n"
            "    capacity = %zu\n"
            "    data[%p]\n"
            "    Left data canary = %zu\n",
            stack, stackName + 1, stack->owner.fileName, stack->owner.line, stack->owner.name,
            caller->fileName, caller->line, caller->name,
            error,
            stack->leftCanary,
            stack->rightCanary,
            stack->size,
            stack->capacity,
            stack->data,
            *_getLeftDataCanaryPtr(stack->data)
            );

    const StackElement_t* data = stack->data;
    size_t size = stack->size;
    for (size_t i = 0; i < size; i++)
    {
        fprintf(where, "    ");

        if (data[i] != POISON)
        {
            SetConsoleColor(where, GREEN);
            fprintf(where, "*");
        }
        else
        {
            SetConsoleColor(where, RED);
            fprintf(where, " ");
        }

        fprintf(where, "[%zu] = " PRINTF_SPECIFIER "\n", i, data[i]);
    }

    SetConsoleColor(where, WHITE);

    fprintf(where, "    Right data canary = %zu\n}\n", *_getRightDataCanaryPtr(stack->data, stack->realDataSize));

    _checkCanary(stack, canary);

    return EVERYTHING_FINE;
}

ErrorCode Push(Stack* stack, StackElement_t value, canary_t canary)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    ErrorCode error = CheckStackIntegrity(stack, canary);

    if (error != EVERYTHING_FINE)
        return error;

    error = _stackRealloc(stack, canary);

    if (error != EVERYTHING_FINE)
        return error;

    stack->data[stack->size] = value;
    stack->size++;

    _checkCanary(stack, canary);

    return EVERYTHING_FINE;
}

StackElementOption Pop(Stack* stack, canary_t canary)
{
    ErrorCode error = CheckStackIntegrity(stack, canary);

    if (error != EVERYTHING_FINE)
        return {POISON, error};

    if (stack->size == 0)
        return {POISON, ERROR_INDEX_OUT_OF_BOUNDS};

    size_t size = stack->size - 1;
    StackElement_t* data = stack->data;

    StackElement_t el = data[size];
    
    data[size] = POISON;

    stack->size = size;

    error = _stackRealloc(stack, canary);

    _checkCanary(stack, canary);

    return {el, error};
}

ErrorCode _stackRealloc(Stack* stack, canary_t canary)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    if ((stack->size == stack->capacity) ||
        (stack->size > 0 && stack->capacity > DEFAULT_CAPACITY &&
         stack->size == stack->capacity / (STACK_GROW_FACTOR * STACK_GROW_FACTOR)))
    {
        size_t numberOfElementsToAdd = stack->capacity * (STACK_GROW_FACTOR - 1);

        StackElement_t* oldData = (StackElement_t*)((void*)stack->data - sizeof(canary_t));
        size_t realDataSize = stack->realDataSize;

        canary_t* oldRightCanaryPtr = _getRightDataCanaryPtr(stack->data, stack->realDataSize);
        canary_t oldRightCanary = *oldRightCanaryPtr;

        *oldRightCanaryPtr = 0;

        realDataSize += numberOfElementsToAdd * sizeof(StackElement_t);

        StackElement_t* newData = (StackElement_t*)realloc((void*)oldData, realDataSize);
        newData = (StackElement_t*)((void*)newData + sizeof(canary_t));

        *_getRightDataCanaryPtr(newData, realDataSize) = oldRightCanary;

        if (newData == NULL)
            return ERROR_NO_MEMORY;

        stack->data = newData;
        stack->realDataSize = realDataSize;
        stack->capacity += numberOfElementsToAdd;
    }


    _checkCanary(stack, canary);

    return EVERYTHING_FINE;
}

ErrorCode _checkCanary(const Stack* stack, size_t canary)
{
    if (stack->leftCanary != canary ||
        stack->rightCanary != canary ||
        *_getLeftDataCanaryPtr(stack->data) != canary ||
        *_getRightDataCanaryPtr(stack->data, stack->realDataSize) != canary)
        MyAssertHard(0, ERROR_DEAD_CANARY);


    return EVERYTHING_FINE;
}

canary_t* _getLeftDataCanaryPtr(const StackElement_t* data)
{
    return (canary_t*)((void*)data - sizeof(canary_t));
}

canary_t* _getRightDataCanaryPtr(const StackElement_t* data, size_t realDataSize)
{
    return (canary_t*)((void*)data + realDataSize - 2 * sizeof(canary_t));
}
