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
    Stack* sex = tempStack.stack;
    for (int i = 0; i < 10; i++)
    {
        Push(sex, i);
    }
    StackDump(stderr, sex);
    StackDump(stderr, sex);
    for (int i = 0; i < 5; i++)
    {
        StackElementOption value = Pop(sex);
        if (value.error == EVERYTHING_FINE)
            printf(STACK_PRINTF_SPECIFIER " ", value.value);
        else
            puts("Empty stack pop");
    }
    puts("");

    StackDestructor(sex);

    return 0;
}