#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "Utils.hpp"
#include "Stack.hpp"

int main()
{
    StackOption tempStack = StackInit();

    if (tempStack.error != EVERYTHING_FINE)
    {
        fprintf(stderr, "%s\n", ValueToString(tempStack.error));
        return tempStack.error;

    }
    Stack* stack = tempStack.stack;
    for (int i = 0; i < 9; i++)
    {
        ErrorCode error = Push(stack, i);
        if (error)
        {
            fprintf(stderr, "ERROR\n");
            StackDestructor(stack);
            return error;
        }
    }

    for (int i = 0; i < 9; i++)
    {
        StackElementOption el = Pop(stack);
        if (!el.error)
            printf("%d, cap = %zu\n", el.value, *(size_t*)((void*)stack + 56));
    }

    StackDestructor(stack);

    return 0;
}