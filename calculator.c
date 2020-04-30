#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "Lists/Stacks/stack.h"
#include "Lists/Queues/queue.h"

#define BUFFER 128
#define CHUNK_SIZE 8
#define DIVZERO_ERR -8

int isOperator(char token);
int checkPriority(char operator1, char operator2);
Queue* strToMathQueue(char* inputString, int maxStringSize);
int shuntingYard(Queue* inQueue, Stack* opStack, Stack* evalStack);
double* applyOperation(char operator, double lOperand, double rOperand);
int popAndEval(char opToken, Stack* evalStack);

int main(void){
	while(1){
		char inputString[BUFFER];
		Stack* opStack = stackCreate(free);
		Stack* evalStack = stackCreate(free);

		printf("Cal>> ");
		fgets(inputString, sizeof(inputString), stdin);

		if(strncmp(inputString, "quit\n", 2) == 0){
			printf("\nQuitting...\n");
			stackDestroy(opStack);
			stackDestroy(evalStack);
			break;
		}

		Queue* inQueue = strToMathQueue(inputString, BUFFER);

		if(inQueue == NULL){
			continue;
		}

		if(shuntingYard(inQueue, opStack, evalStack) == DIVZERO_ERR){
			fprintf(stderr, "Error: Division by zero.");
			stackDestroy(opStack);
			stackDestroy(evalStack);
		} else{
			printf("ANS>> %g\n", *(double*)stackPeek(evalStack));
		}

		stackDestroy(opStack);
		stackDestroy(evalStack);
		queueDestroy(inQueue);
	}

	return 0;
}

int isOperator(char token){
	switch(token){
		case '+':
		case '-':
		case '*':
		case '/':
		case '^':
			return 1;
		default:
			return 0;
	}
}

int checkPriority(char operator1, char operator2){
	int priority1, priority2;

	if(operator1 == '+' || operator1 == '-'){
		priority1 = 1;
	} else if(operator1 == '*' || operator1 == '/'){
		priority1 = 2;
	} else if(operator1 == '^'){
		priority1 = 3;
	}

	if(operator2 == '+' || operator2 == '-'){
		priority2 = 1;
	} else if(operator2 == '*' || operator2 == '/'){
		priority2 = 2;
	} else if(operator2 == '^'){
		priority2 = 3;
	}

	return priority1 - priority2;
}

// Converts a math string into a queue. Returns a pointer to the queue if
// successful, and a NULL pointer if not.
Queue* strToMathQueue(char* inputString, int maxStringSize){
	Queue* mathQueue = queueCreate(free);

	// Using a for loop so that iterations are capped in case of string
	// formatting error. E.g. when reading expressions from a badly
	// formatted file.
	for(int i=0; i<maxStringSize; ++i){
		char token = inputString[i];

		// Reached the end of the string early.
		if(token == '\n' || token == '\0'){
			break;
		// Necessary to make sure the string is split correctly into
		// full numbers and not just single digits.
		} else if(isdigit(token) || token == '.'){
			int numLen = 1, tokenSize = CHUNK_SIZE;
			char* numToken = malloc(tokenSize);

			// Count the length of the number by iterating until
			// token is no longer a digit or decimal point.
			while(isdigit(inputString[i + numLen]) || (inputString[i + numLen] == '.')){
				// Add more space in chunks if needed.
				if((numLen + 1) % CHUNK_SIZE == 0){
					tokenSize += CHUNK_SIZE;
					numToken = realloc(numToken, tokenSize);
				}

				++numLen;
			}

			// Copy data over to the queue.
			strncpy(numToken, &inputString[i], numLen);
			numToken[numLen] = '\0';
			enqueue(mathQueue, numToken);

			// Skip to after the number.
			i += numLen - 1;
		// Handle brackets and operators.
		} else if(token == '(' || token == ')' || isOperator(token)){
			char* symToken = malloc(2);
			symToken[0] = token;
			symToken[1] = '\0';
			enqueue(mathQueue, symToken);
		//Ignore spaces, and tabs.
		} else if(token == ' ' || token == '\t'){
			continue;
		// Print error and return NULL if none of the token criteria
		// match.
		} else{
			fprintf(stderr, "Error: %c was unrecognised token.\n ", token);
			queueDestroy(mathQueue);
			return NULL;
		}
	}

	return mathQueue;
}

int shuntingYard(Queue* inQueue, Stack* opStack, Stack* evalStack){
	while(getQueueSize(inQueue) > 0){
		void** voidToken = malloc(sizeof(void*));
		dequeue(inQueue, voidToken);
		char* stringToken = *(char**)voidToken;

		// If the current token is a number.
		if(isdigit(*stringToken) || *stringToken == '.'){
			double* mathToken = malloc(sizeof(double));
			*mathToken = atof(stringToken);
			stackPush(evalStack, mathToken);

			free(*voidToken);
			free(voidToken);
		// If the current token is an operator.
		} else if(isOperator(*stringToken)){
			// If operator stack is non-empty and is not topped by
			// a left bracket.
			if(getStackSize(opStack) > 0 && *(char*)stackPeek(opStack) != '('){
				// If there is an operator on the opStack with
				// greater precedence.
				while(checkPriority(*stringToken, *(char*)stackPeek(opStack)) <= 0){
					void** opToken = malloc(sizeof(void*));
					stackPop(opStack, opToken);

					if(popAndEval(**(char**)opToken, evalStack) == DIVZERO_ERR){
						free(opToken);
						return DIVZERO_ERR;
					}

					free(opToken);

					if(getStackSize(opStack) == 0){
						break;
					}
				}

				stackPush(opStack, stringToken);
			} else{
				stackPush(opStack, stringToken);
			}
		} else if((*stringToken) == '('){
			stackPush(opStack, stringToken);
		} else if((*stringToken) == ')'){
			while(*(char*)stackPeek(opStack) != '('){
				void** opToken = malloc(sizeof(void*));
				stackPop(opStack, opToken);

				if(popAndEval(**(char**)opToken, evalStack) == DIVZERO_ERR){
					free(opToken);
					return DIVZERO_ERR;
				}

				free(opToken);
			}

			// Dummy variable to hold the left bracket before it is
			// deallocated.
			void** tempLBracket = malloc(sizeof(void*));
			stackPop(opStack, tempLBracket);
			// Discard brackets.
			free(*tempLBracket);
			free(tempLBracket);
			free(*voidToken);
			free(voidToken);
		}
	}

	while(getStackSize(opStack) > 0){
		void** opToken = malloc(sizeof(void*));
		stackPop(opStack, opToken);

		if(popAndEval(**(char**)opToken, evalStack) == DIVZERO_ERR){
			free(opToken);
			return DIVZERO_ERR;
		}

		free(opToken);
	}

	return 0;
}

double* applyOperation(char operator, double lOperand, double rOperand){
	double* result = malloc(sizeof(double));

	switch(operator){
		case '+':
			*result = lOperand + rOperand;
			return result;
		case '-':
			*result = lOperand - rOperand;
			return result;
		case '*':
			*result = lOperand * rOperand;
			return result;
		case '/':
			*result = lOperand / rOperand;
			return result;
		case '^':
			*result = pow(lOperand, rOperand);
			return result;
	}

	return 0;
}

int popAndEval(char opToken, Stack* evalStack){
	// If the evalStack has enough items to
	// evaluate.
	if(getStackSize(evalStack) >= 2){
		void** lOperand = malloc(sizeof(void*));
		void** rOperand = malloc(sizeof(void*));
		double *result;
		stackPop(evalStack, rOperand);
		stackPop(evalStack, lOperand);

		if((**(double**)rOperand) == 0 && opToken == '/'){
			free(*lOperand);
			free(lOperand);
			free(*rOperand);
			free(rOperand);
			return DIVZERO_ERR;
		}

		result = applyOperation(\
				opToken,\
				**(double**)lOperand,\
				**(double**)rOperand);

		stackPush(evalStack, result);
		free(*lOperand);
		free(lOperand);
		free(*rOperand);
		free(rOperand);
	}

	return 0;
}
