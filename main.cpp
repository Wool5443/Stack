#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "Utils.hpp"
#include "Stack.hpp"

int main()
{
    StackResult tempStack = StackInit();

    if (tempStack.error != EVERYTHING_FINE)
    {
        fprintf(stderr, "%s\n", ValueToString(tempStack.error));
        return tempStack.error;

    }
    Stack* stack = tempStack.value;
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
        StackElementResult el = Pop(stack);
        if (el.error != EVERYTHING_FINE)
        {
            fprintf(stderr, "ERROR\n");
            return el.error;
        }
        printf("%d\n", el.value);
    }

    StackDestructor(stack);

    return 0;
}
