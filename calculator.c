#include <stdio.h>
#include <string.h>
#include <math.h>
#include "Lists/Stacks/stack.h"
#include "Lists/Queues/queue.h"

#define BUFFER 128
#define CHUNK_SIZE 8
#define DIVZERO_ERR -8

typedef enum {
	divZero,
	unkownToken,
	unpairedBrackets,
	extraDecimalSep
} Error;

typedef enum {
	unknown = -1,
	digit,
	decimalSep,
	operator,
	lbracket,
	rbracket,
	function,
	whitespace,
	EOL
} TokenType;

typedef enum {
	left,
	right
} AssocType;

typedef enum {
	unitary = 1,
	binary
} OpType;

Queue* strToMathQueue(char* inputString, int maxStringSize);
int shuntingYard(Queue* inQueue, Stack* opStack, Stack* evalStack);
int popAndEval(Stack* opStack, Stack* evalStack);
double* applyOperation(void* operatorPtr, void* lOperandPtr, void* rOperandPtr);
int checkPriority(void* operator1Ptr, void* operator2Ptr);
AssocType checkAssoc(void* operator);
TokenType tokenType(void* token);

int main(void){
	while(1){
		char inputString[BUFFER];
		Stack* opStack = stackCreate(free);
		Stack* evalStack = stackCreate(free);

		printf("Cal>> ");
		fgets(inputString, sizeof(inputString), stdin);

		if(strncmp(inputString, "quit\n", 5) == 0){
			printf("\nQuitting...\n");
			stackDestroy(opStack);
			stackDestroy(evalStack);
			break;
		}

		Queue* inQueue = strToMathQueue(inputString, BUFFER);

		if(inQueue == NULL){
			stackDestroy(opStack);
			stackDestroy(evalStack);
			continue;
		}

		if(shuntingYard(inQueue, opStack, evalStack) == DIVZERO_ERR){
			fprintf(stderr, "Error: Division by zero.\n");
		} else{
			printf("ANS>> %g\n", *(double*)stackPeek(evalStack));
		}

		stackDestroy(opStack);
		stackDestroy(evalStack);
		queueDestroy(inQueue);
	}

	return 0;
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
		if(tokenType(&token) == EOL){
			// If the user submitted an empty input.
			if(strcmp(inputString, "\n") == 0){
				queueDestroy(mathQueue);
				return NULL;
			}

			break;
		// Necessary to make sure the string is split correctly into
		// full numbers and not just single digits.
		} else if(tokenType(&token) == digit || tokenType(&token) == decimalSep){
			int numLen = 1, tokenSize = CHUNK_SIZE;
			char* numToken = malloc(tokenSize);

			// Count the length of the number by iterating until
			// token is no longer a digit or decimal point.
			while(tokenType(inputString + i + numLen) == digit\
					|| tokenType(inputString + i + numLen) == decimalSep){
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
		} else if(tokenType(&token) == lbracket \
				|| tokenType(&token) == rbracket
				|| tokenType(&token) == operator){
			char* symToken = malloc(sizeof(char*));
			*symToken = token;
			*(symToken + 1) = '\0';
			enqueue(mathQueue, symToken);
		// Ignore spaces, and tabs.
		} else if(tokenType(&token) == whitespace){
			continue;
		// Print error and return NULL if none of the token criteria
		// match.
		} else if(tokenType(&token) == unknown){
			fprintf(stderr, "Error: %c was unrecognised token.\n ", token);
			queueDestroy(mathQueue);
			return NULL;
		}
	}

	return mathQueue;
}

int shuntingYard(Queue* inQueue, Stack* opStack, Stack* evalStack){
	while(getQueueSize(inQueue) > 0){
		void** token = malloc(sizeof(void*));
		dequeue(inQueue, token);

		// If the current token is a number.
		if(tokenType(*token) == digit || tokenType(*token) == decimalSep){
			double* mathToken = malloc(sizeof(double));
			*mathToken = atof(*(char**)token);
			stackPush(evalStack, mathToken);

			free(*token);
		// If the current token is an operator.
		} else if(tokenType(*token) == operator){
			// If operator stack is non-empty and is not topped by
			// a left bracket.
			if(getStackSize(opStack) > 0 && *(char*)stackPeek(opStack) != '('){
				// If there is an operator on the opStack with
				// greater precedence.
				while(checkPriority(*token, stackPeek(opStack)) <= 0){
					if(popAndEval(opStack, evalStack) == DIVZERO_ERR){
						free(token);
						return DIVZERO_ERR;
					}

					if(getStackSize(opStack) == 0){
						break;
					}
				}
			}

			stackPush(opStack, *(char**)token);

		} else if(tokenType(*token) == lbracket){
			stackPush(opStack, *(char**)token);
		} else if(tokenType(*token) == rbracket){
			while(*(char*)stackPeek(opStack) != '('){
				if(popAndEval(opStack, evalStack) == DIVZERO_ERR){
					free(token);
					return DIVZERO_ERR;
				}
			}

			// Dummy variable to hold the left bracket before it is
			// deallocated.
			void** tempLBracket = malloc(sizeof(void*));
			stackPop(opStack, tempLBracket);
			// Discard brackets.
			free(*tempLBracket);
			free(tempLBracket);
			free(*token);
		}

		free(token);
	}

	while(getStackSize(opStack) > 0){
		if(popAndEval(opStack, evalStack) == DIVZERO_ERR){
			return DIVZERO_ERR;
		}
	}

	return 0;
}

int popAndEval(Stack* opStack, Stack* evalStack){
	void** opToken = malloc(sizeof(void*));
	stackPop(opStack, opToken);

	// If the evalStack has enough items to
	// evaluate.
	if(getStackSize(evalStack) >= 2){
		void** lOperand = malloc(sizeof(void*));
		void** rOperand = malloc(sizeof(void*));
		double *result;
		stackPop(evalStack, rOperand);
		stackPop(evalStack, lOperand);

		if((**(double**)rOperand) == 0 && **(char**)opToken == '/'){
			free(*opToken);
			free(opToken);
			free(*lOperand);
			free(lOperand);
			free(*rOperand);
			free(rOperand);
			return DIVZERO_ERR;
		}

		result = applyOperation(*opToken,*lOperand,*rOperand);

		stackPush(evalStack, result);
		free(*lOperand);
		free(lOperand);
		free(*rOperand);
		free(rOperand);
	}

	free(*opToken);
	free(opToken);
	return 0;
}

double* applyOperation(void* operatorPtr, void* lOperandPtr, void* rOperandPtr){
	double* result = malloc(sizeof(double));
	char operator = *(char*)operatorPtr;
	double lOperand = *(double*)lOperandPtr;
	double rOperand = *(double*)rOperandPtr;

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

int checkPriority(void* operator1Ptr, void* operator2Ptr){
	char operator1 = *(char*) operator1Ptr;
	char operator2 = *(char*) operator2Ptr;
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

AssocType checkAssoc(void* operator){
	switch(*(char*)operator){
		case '^':
		case '!':
			return right;
		default:
			return left;
	}
}

TokenType tokenType(void* token){
	char* charToken = (char*)token;

	switch(*charToken){
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			return digit;
		case '.':
			return decimalSep;
		case '+':
		case '-':
		case '*':
		case '/':
		case '^':
			return operator;
		case '(':
			return lbracket;
		case ')':
			return rbracket;
		case ' ':
		case '\t':
			return whitespace;
		case '\n':
		case '\0':
			return EOL;
	}


	// Add function patterns in if-else blocks.

	return unknown;
}
