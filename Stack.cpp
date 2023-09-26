#include "Stack.hpp"

ErrorCode _checkCanary(const Stack* stack, size_t canary);

ErrorCode _stackRealloc(Stack* stack, size_t canary);

size_t _getLeftDataCanary(const Stack* stack);

size_t _getRightDataCanary(const Stack* stack);

StackOption _stackInit(Owner owner, size_t canary)
{
    Stack stack = {.leftCanary = canary, .owner = owner,
                   .size = 0, .capacity = DEFAULT_CAPACITY, .rightCanary = canary};

    size_t realDataSize = DEFAULT_CAPACITY * sizeof(StackElement_t) + 2 * sizeof(canary);
    size_t allignmentSize = (8 - (realDataSize - sizeof(canary)) % 8) % 8;
    realDataSize += allignmentSize;

    StackElement_t* data = (StackElement_t*)calloc(realDataSize, 1);

    ((size_t*)data)[0] = canary;

    *(size_t*)((void*)data + realDataSize - sizeof(canary)) = canary;

    data = (StackElement_t*)((void*)data + sizeof(canary));

    ErrorCode error = EVERYTHING_FINE;
    if (!data)
        error = ERROR_NO_MEMORY;
    
    stack.data = data;
    stack.realDataSize = realDataSize;

    return {stack, error};
}

ErrorCode StackDestructor(Stack* stack, size_t canary)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    free((void*)stack->data - sizeof(canary));
    stack->size = POISON;
    stack->capacity = POISON;
    stack->data = NULL;

    return EVERYTHING_FINE;
}

ErrorCode CheckStackIntegrity(const Stack* stack, size_t canary)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    if (stack->capacity < stack->size)
        return ERROR_INDEX_OUT_OF_BOUNDS;
    if (!stack->data)
        return ERROR_NO_MEMORY;

    _checkCanary(stack, canary);

    return EVERYTHING_FINE;
}

ErrorCode _stackDump(FILE* where, const Stack* stack, const char* stackName, Owner* caller, size_t canary)
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
            _getLeftDataCanary(stack)
            );

    const StackElement_t* data = stack->data;
    size_t size = stack->size;
    for (size_t i = 0; i < size; i++)
    {
        fprintf(where, "    ");

        if (data[i] != POISON)
            fprintf(where, "*");
        else
            fprintf(where, " ");

        fprintf(where, "[%zu] = " PRINTF_SPECIFIER "\n", i, data[i]);
    }

    fprintf(where, "    Right data canary = %zu\n}\n", _getRightDataCanary(stack));

    _checkCanary(stack, canary);

    return EVERYTHING_FINE;
}

ErrorCode Push(Stack* stack, StackElement_t value, size_t canary)
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

StackElementOption Pop(Stack* stack, size_t canary)
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

ErrorCode _stackRealloc(Stack* stack, size_t canary)
{
    MyAssertSoft(stack, ERROR_NULLPTR);

    if ((stack->size == stack->capacity) ||
        (stack->size > 0 && stack->capacity > DEFAULT_CAPACITY &&
         stack->size == stack->capacity / (STACK_GROW_FACTOR * STACK_GROW_FACTOR)))
    {
        size_t numberOfElementsToAdd = stack->capacity * (STACK_GROW_FACTOR - 1);

        StackElement_t* oldData = (StackElement_t*)((void*)stack->data - sizeof(canary));
        size_t realDataSize = stack->realDataSize;
        size_t dataCanary = *(size_t*)oldData;

        *(size_t*)((void*)oldData + realDataSize - sizeof(canary)) = 0;

        realDataSize += numberOfElementsToAdd * sizeof(StackElement_t);
        size_t allignmentSize = (8 - (realDataSize - sizeof(canary)) % 8) % 8;
        realDataSize += allignmentSize;

        StackElement_t* newData = (StackElement_t*)realloc((void*)oldData, realDataSize);

        *(size_t*)((void*)newData + realDataSize - sizeof(canary)) = dataCanary;

        newData = (StackElement_t*)((void*)newData + sizeof(canary));

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
        _getLeftDataCanary(stack) != canary ||
        _getRightDataCanary(stack) != canary)
        MyAssertHard(0, ERROR_DEAD_CANARY);


    return EVERYTHING_FINE;
}

size_t _getLeftDataCanary(const Stack* stack)
{
    return *(size_t*)((void*)stack->data - sizeof(size_t));
}

size_t _getRightDataCanary(const Stack* stack)
{
    return *(size_t*)((void*)stack->data + stack->realDataSize - 2 * sizeof(size_t));
}
