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
    for (int i = 0; i < 500; i++)
    {
        ErrorCode error = Push(stack, i);
        if (error)
        {
            fprintf(stderr, "ERROR\n");
            StackDestructor(stack);
            return error;
        }
    }

    StackDestructor(stack);

    return 0;
}