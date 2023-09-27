#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "Utils.hpp"
#include "Stack.hpp"

int main()
{
    srand((unsigned int)time(NULL));
    size_t canaryShift = sizeof(size_t) - sizeof(unsigned int);
    size_t canary = ((unsigned int)rand() << canaryShift) + ((unsigned int)rand() % (1 << canaryShift));

    StackOption tempStack = StackInit(canary);

    if (tempStack.error != EVERYTHING_FINE)
    {
        fprintf(stderr, "%s\n", ValueToString(tempStack.error));
        return tempStack.error;

    }
    Stack sex = tempStack.stack;
    for (int i = 0; i < 10; i++)
    {
        Push(&sex, i, canary);
    }
    for (int i = 0; i < 0; i++)
        printf("%d ", Pop(&sex, canary).value);
    puts("");

    StackDump(stderr, &sex, canary);

    StackDestructor(&sex);

    return 0;
}