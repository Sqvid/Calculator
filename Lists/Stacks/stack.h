#ifndef STACK_H
#define STACK_H

#include "../list.h"

typedef List Stack;
typedef ListElement StackElement;


#define stackCreate listCreate
#define getStackSize getListSize
#define stackDestroy listDestroy
#define stackPeek(stack) ((stack)->head->data == NULL ? NULL : (stack)->head->data)

int stackPush(Stack* stack, const void* data);
int stackPop(Stack* stack, void** data);


// Function definitions:

int stackPush(Stack* stack, const void* data) {
	return listAddNext(stack, NULL, data);
}

int stackPop(Stack* stack, void** data) {
	return listDelNext(stack, NULL, data);
}

#endif
