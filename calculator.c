#include <stdio.h>
#include <string.h>
#include <math.h>
#include "Lists/Stacks/stack.h"

#define BUFFER 128
#define CHUNK_SIZE 8
#define DIVZERO_ERR -8

typedef enum {
	divZero,
	unkownToken,
	unpairedBrackets,
	extraDecimalSep,
	hangingDecimalSep
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

char** strToMathArray(char* inputString);
double shuntingYard(char* inputString);
int popAndEval(Stack* opStack, Stack* evalStack);
double* applyOperation(void* operatorPtr, void* lOperandPtr, void* rOperandPtr);
int checkPriority(void* operator1Ptr, void* operator2Ptr);
AssocType checkAssoc(void* operator);
TokenType tokenType(void* token);

int main(void){
	while(1){
		char inputString[BUFFER];

		printf("Cal>> ");
		fgets(inputString, sizeof(inputString), stdin);

		if(strncmp(inputString, "quit\n", 5) == 0){
			printf("\nQuitting...\n");
			break;
		}

		double result = shuntingYard(inputString);
		printf("ANS>> %g\n", result);
	}

	return 0;
}

// Converts the input string into a ragged array.
// Returns a pointer to the array.
char** strToMathArray(char* inputString){
	char** exprArray = malloc(CHUNK_SIZE * sizeof(char*));
	int exprPos = 0, exprSize = CHUNK_SIZE;

	// Using a for loop so that iterations are capped in case of string
	// formatting error. E.g. when reading expressions from a badly
	// formatted file.
	for(unsigned int i=0; i<strlen(inputString); ++i){
		// Reallocate more space in chunks if necessary.
		if((exprPos + 1) % CHUNK_SIZE == 0){
			exprSize += CHUNK_SIZE;
			exprArray = realloc(exprArray, exprSize * sizeof(char*));
		}

		char token = inputString[i];

		// Is the variable a positive (+) or negative (-) sign.
		int isSign = 0;

		if(token == '+' || token == '-'){
			if(i == 0){
				isSign = (tokenType(inputString + i + 1) == digit);
			} else{
				TokenType prevToken = tokenType(inputString + i - 1);

				if(prevToken == lbracket || prevToken == operator){
					isSign = 1;
				}
			}
		}

		if(tokenType(&token) == EOL){
			char* endToken = malloc(1);
			endToken[0] = '\n';

			exprArray[exprPos] = endToken;
			++exprPos;

		// Necessary to make sure the string is split correctly into
		// full numbers and not just single digits.
		} else if(tokenType(&token) == digit || \
				tokenType(&token) == decimalSep || isSign){

			int numLen = 1, tokenSize = CHUNK_SIZE;
			char* numToken = malloc(tokenSize);

			// Ignore the positive sign as it is assumed.
			if(token == '+'){
				++i;
			}

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

			// Copy data over and attach the pointer to the array.
			strncpy(numToken, inputString + i, numLen);
			numToken[numLen] = '\0';
			exprArray[exprPos] = numToken;
			++exprPos;

			// Skip to after the number.
			i += numLen - 1;
		// Handle brackets and operators.
		} else if(tokenType(&token) == lbracket \
				|| tokenType(&token) == rbracket
				|| tokenType(&token) == operator){
			char* symToken = malloc(sizeof(char*));
			*symToken = token;
			*(symToken + 1) = '\0';
			exprArray[exprPos] = symToken;
			++exprPos;
		// Ignore spaces, and tabs.
		} else if(tokenType(&token) == whitespace){
			continue;
		} else if(tokenType(&token) == unknown){
			char* endToken = malloc(1);
			endToken[0] = '\n';

			exprArray[exprPos] = endToken;
			++exprPos;
			break;
		}
	}

	return exprArray;
}

double shuntingYard(char* inputString){
	Stack* opStack = stackCreate(free);
	Stack* evalStack = stackCreate(free);
	char** exprArray = strToMathArray(inputString);

	int exprPos = 0;
	do{
		char* token = exprArray[exprPos];
		int isSign = 0;

		if(*token == '+' || *token == '-'){
			// If the following character is a digit.
			if(tokenType(token + 1) == digit){
				isSign = 1;
			}
		}

		// If the current token is a number.
		if(tokenType(token) == digit \
				|| tokenType(token) == decimalSep || isSign){

			double* mathToken = malloc(sizeof(double));
			*mathToken = atof(token);
			stackPush(evalStack, mathToken);

			free(token);

		// If the current token is an operator.
		} else if(tokenType(token) == operator){
			// If operator stack is non-empty and is not topped by
			// a left bracket.
			if(getStackSize(opStack) > 0 && *(char*)stackPeek(opStack) != '('){
				// If there is an operator on the opStack with
				// greater precedence.
				while(checkPriority(token, stackPeek(opStack)) <= 0){
					if(popAndEval(opStack, evalStack) == DIVZERO_ERR){
						stackDestroy(opStack);
						stackDestroy(evalStack);
						free(exprArray);
						return DIVZERO_ERR;
					}

					if(getStackSize(opStack) == 0){
						break;
					}
				}
			}

			stackPush(opStack, token);

		} else if(tokenType(token) == lbracket){
			stackPush(opStack, token);
		} else if(tokenType(token) == rbracket){
			while(*(char*)stackPeek(opStack) != '('){
				if(popAndEval(opStack, evalStack) == DIVZERO_ERR){
					stackDestroy(opStack);
					stackDestroy(evalStack);
					free(exprArray);
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
			free(token);
		}

		++exprPos;
	} while(*exprArray[exprPos] != '\n');

	// Free the endToken (marked by the newline).
	free(exprArray[exprPos]);

	while(getStackSize(opStack) > 0){
		if(popAndEval(opStack, evalStack) == DIVZERO_ERR){
			stackDestroy(opStack);
			stackDestroy(evalStack);
			free(exprArray);
			return DIVZERO_ERR;
		}
	}

	double result = *(double*)stackPeek(evalStack);
	stackDestroy(opStack);
	stackDestroy(evalStack);
	free(exprArray);

	return result;
}

int popAndEval(Stack* opStack, Stack* evalStack){
	void** opToken = malloc(sizeof(void*));
	stackPop(opStack, opToken);
	double *result;

	if(getStackSize(evalStack) == 1){
		void** operand = malloc(sizeof(void*));
		stackPop(evalStack, operand);

		if(**(char**)opToken != '+' && **(char**)opToken != '-'){
			// ***Handle Error***
		} else{
			if(**(char**)opToken == '-'){
				**(double**)operand = -1 * (**(double**)operand);
				stackPush(evalStack, *operand);

			}
		}

		free(operand);
	}

	// If the evalStack has enough items to
	// evaluate.
	if(getStackSize(evalStack) >= 2){
		void** lOperand = malloc(sizeof(void*));
		void** rOperand = malloc(sizeof(void*));
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
			break;
		case '-':
			*result = lOperand - rOperand;
			break;
		case '*':
			*result = lOperand * rOperand;
			break;
		case '/':
			*result = lOperand / rOperand;
			break;
		case '^':
			*result = pow(lOperand, rOperand);
			break;
	}

	return result;
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
