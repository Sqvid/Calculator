#include <stdio.h>
#include <string.h>
#include "Lists/Stacks/stack.h"

#define __USE_MISC
#include <math.h>

#define MK_STRING(x)      #x
#define CONV_TO_STRING(x) MK_STRING(x)

#define PI         CONV_TO_STRING(M_PI)
#define EULER      CONV_TO_STRING(M_E)
#define CONST_ACC  strlen(PI)
#define BUFFER     256
#define CHUNK_SIZE 8

typedef enum {
	success,
	divZero,
	evalFail,
	unknownToken,
	unpairedBracket,
	noDigit,
	noOperator,
	extraDecimalSep,
} Status;

typedef enum {
	unknown,
	digit,
	decimalSep,
	operator,
	lbracket,
	rbracket,
	constant,
	function,
	whitespace,
	EOL
} TokenType;

typedef enum {
	none,
	sqrt_,
	sin_,
	cos_,
	tan_
} FunctionType;

typedef enum {
	left,
	right
} AssocType;

void shuntingYard(char* inputString, double* prevAns);
char** strToMathArray(char* inputString, double* prevAns);
Status popAndEval(Stack* opStack, Stack* evalStack);
double* applyOperation(void* operator, void* lOperandPtr, void* rOperandPtr);
double* applyFunction(FunctionType functionKey, void* operandPtr);
int getPriority(void* operator1, void* operator2);
AssocType getAssoc(void* operator);
TokenType tokenType(void* token);
FunctionType functionType(void* token);
void printStatus(Status status);

double prevAns = 0.0;

int main(void) {
	while(1) {
		char inputString[BUFFER];

		printf("Cal>> ");
		fgets(inputString, sizeof(inputString), stdin);

		if(strncmp(inputString, "quit\n", 5) == 0) {
			printf("\nQuitting...\n");
			break;
		}

		shuntingYard(inputString, &prevAns);
	}

	return 0;
}

// Implements the shunting yard algorithm to evaluate the expression on a
// reverse polish stack.
void shuntingYard(char* inputString, double* prevAns) {
	Stack* opStack = stackCreate(free);
	Stack* evalStack = stackCreate(free);
	char** exprArray = strToMathArray(inputString, prevAns);

	Status tmpStatus, evalStatus = success;
	int exprPos, prevOpPos = -1;

	for(exprPos = 0; *exprArray[exprPos] != '\n'; ++exprPos) {
		char* token = exprArray[exprPos];
		TokenType tokenGroup = tokenType(token);
		int isSign = 0;

		if(*token == '+' || *token == '-') {
			// If the following character is a digit.
			if(tokenType(token + 1) == digit) {
				isSign = 1;
			}
		}

		// If the current token is a number.
		if(tokenGroup == digit || tokenGroup == decimalSep || isSign) {
			double* mathToken = malloc(sizeof(double));
			*mathToken = atof(token);
			stackPush(evalStack, mathToken);

			free(token);

		// If the current token is an operator.
		} else if(tokenGroup == operator) {
			// If previous token was an operator.
			if(exprPos > 1 && prevOpPos == exprPos - 1) {
				stackPush(opStack, token);
				continue;
			}

			// If operator stack is non-empty and is not topped by
			// a left bracket.
			if(getStackSize(opStack) > 0
					&& *(char*)stackPeek(opStack) != '(') {
				// While there is an operator on the opStack
				// with greater or equal precedence.
				while(tokenType(stackPeek(opStack)) == operator &&
						getPriority(token, stackPeek(opStack)) <= 0) {
					if(getPriority(token, stackPeek(opStack)) == 0
							&& getAssoc(token) == right) {
						break;
					}

					tmpStatus = popAndEval(opStack, evalStack);

					if(tmpStatus != success) {
						evalStatus = tmpStatus;
					}

					if(getStackSize(opStack) == 0) {
						break;
					}
				}
			}

			stackPush(opStack, token);
			prevOpPos = exprPos;

		} else if(tokenGroup == lbracket) {
			stackPush(opStack, token);
		} else if(tokenGroup == rbracket) {
			if(getStackSize(opStack) != 0) {
				while(*(char*)stackPeek(opStack) != '(') {
					tmpStatus = popAndEval(opStack, evalStack);

					if(tmpStatus != success) {
						evalStatus = tmpStatus;
					}
				}

				// Dummy variable to hold the left bracket before it is
				// deallocated.
				void** tempLBracket = malloc(sizeof(void*));
				stackPop(opStack, tempLBracket);
				// Discard left bracket.
				free(*tempLBracket);
				free(tempLBracket);
			} else {
				evalStatus = evalFail;
			}

			// Discard right bracket.
			free(token);
		} else if(tokenGroup == function) {
			stackPush(opStack, token);
		}
	}

	// Free the endToken (marked by the newline).
	free(exprArray[exprPos]);

	while(getStackSize(opStack) > 0 && evalStatus == success) {
		tmpStatus = popAndEval(opStack, evalStack);

		if(tmpStatus != success) {
			evalStatus = tmpStatus;
		}
	}

	printStatus(evalStatus);

	if(exprPos != 0 && evalStatus == success) {
		double result = *(double*)stackPeek(evalStack);
		printf("ANS>> %g\n", result);
		*prevAns = result;
	}

	stackDestroy(opStack);
	stackDestroy(evalStack);
	free(exprArray);
}

// Converts the input string into a ragged array.
// Returns a pointer to the array.
char** strToMathArray(char* inputString, double* prevAns) {
	int exprPos = 0, exprSize = CHUNK_SIZE;
	char** exprArray = malloc(exprSize * sizeof(char*));
	int lbracketCount = 0, rbracketCount = 0, digitCount = 0,
		emptyInput = 0, opCount = 0;
	Status parseStatus = success;

	for(unsigned int i = 0; i < strlen(inputString); ++i) {
		// Reallocate more space in chunks if necessary.
		if((exprPos + 1) % CHUNK_SIZE == 0) {
			exprSize += CHUNK_SIZE;
			exprArray = realloc(exprArray, exprSize * sizeof(char*));
		}

		char token = inputString[i];
		TokenType tokenGroup = tokenType(inputString + i);

		// Is the variable a positive (+) or negative (-) sign.
		int isSign = 0;

		if(token == '+' || token == '-') {
			TokenType nextToken = tokenType(inputString + i + 1);

			if(i == 0 && nextToken == digit) {
				isSign = 1;
			} else {
				TokenType prevToken = tokenType(inputString + i - 1);

				if((prevToken == lbracket
						|| prevToken == operator)
						&& nextToken == digit) {

					isSign = 1;
				}
			}
		}

		if(tokenGroup == EOL) {
			char* endToken = malloc(2);
			endToken[0] = '\n';
			endToken[1] = '\0';
			exprArray[exprPos] = endToken;
			++exprPos;

			emptyInput = (*inputString == '\n');

		// Necessary to make sure the string is split correctly into
		// full numbers and not just single digits.
		} else if(tokenGroup == digit ||
				tokenGroup == decimalSep
				|| isSign) {

			int numLen = 1, tokenSize = CHUNK_SIZE, sepCount;
			char* numToken = malloc(tokenSize);

			(tokenGroup == digit) ? (digitCount = 1) : (digitCount = 0);

			// Ignore the positive sign as it is assumed.
			if(token == '+') {
				++i;
			}

			// Count the length of the number by iterating until
			// token is no longer a digit or decimal point.
			while(tokenType(inputString + i + numLen) == digit
					|| tokenType(inputString + i + numLen)
					== decimalSep) {

				// Add more space in chunks if needed.
				if((numLen + 1) % CHUNK_SIZE == 0) {
					tokenSize += CHUNK_SIZE;
					numToken = realloc(numToken, tokenSize);
				}

				// Track number of digits and decimal points.
				if(tokenType(inputString + i + numLen) == digit) {
					++digitCount;
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

			// The number of decimal places in the number.
			sepCount = numLen - digitCount;

			if(sepCount > 1) {
				parseStatus = extraDecimalSep;
				break;
			}

		// Handle brackets and operators.
		} else if(tokenGroup == lbracket || tokenGroup == rbracket
				|| tokenGroup == operator) {

			if(tokenGroup == lbracket) {
				++lbracketCount;
			} else if(tokenGroup == rbracket) {
				++rbracketCount;
			} else {
				++opCount;
			}

			char* symToken = malloc(sizeof(char*));
			*symToken = token;
			*(symToken + 1) = '\0';
			exprArray[exprPos] = symToken;
			++exprPos;
		} else if(tokenGroup == constant) {
			char* constToken = malloc(CONST_ACC + 2);

			if(strncmp(inputString + i, "ans", 3) == 0) {
				snprintf(constToken, CONST_ACC + 2, "%lf\n", *prevAns);
				i += 2;
			} else if(strncmp(inputString + i, "pi", 2) == 0) {
				strncpy(constToken, PI, CONST_ACC + 1);
				i += 1;
			} else if(strncmp(inputString + i, "e", 1) == 0) {
				strncpy(constToken, EULER, CONST_ACC + 1);
			}

			++digitCount;
			exprArray[exprPos] = constToken;
			++exprPos;
		} else if(tokenGroup == function) {
			FunctionType functionKey = functionType(inputString + i);
			char* funcToken = malloc(CHUNK_SIZE);
			int functionLen;

			// Add function to the expression array and move to the
			// next token position.
			switch(functionKey) {
				case sqrt_:
					functionLen = 4;
					strncpy(funcToken, "sqrt", functionLen + 1);
					break;
				case sin_:
					functionLen = 3;
					strncpy(funcToken, "sin", functionLen + 1);
					break;
				case cos_:
					functionLen = 3;
					strncpy(funcToken, "cos", functionLen + 1);
					break;
				case tan_:
					functionLen = 3;
					strncpy(funcToken, "tan", functionLen + 1);
					break;
				default:
					break;
			}

			exprArray[exprPos] = funcToken;
			++opCount;
			++exprPos;
			i += functionLen - 1;

		// Ignore spaces, and tabs.
		} else if(tokenGroup == whitespace) {
			continue;
		} else if(tokenGroup == unknown) {
			parseStatus = unknownToken;
			fprintf(stderr, "Error: '%c' is an unrecognised token.\n", token);
			break;
		}
	}

	if(lbracketCount != rbracketCount) {
		parseStatus = unpairedBracket;
	} else if(digitCount == 0 && emptyInput == 0
			&& parseStatus != unknownToken) {

		parseStatus = noDigit;
	} else if(opCount == 0 && exprPos != 2 && emptyInput == 0) {
		parseStatus = noOperator;
	}

	if(parseStatus != success) {
		printStatus(parseStatus);

		// Deallocate all tokens.
		for(int i = 0; i < exprPos; ++i) {
			free(exprArray[i]);
		}

		// Mark exprArray as empty.
		char* endToken = malloc(2);
		endToken[0] = '\n';
		endToken[1] = '\0';
		exprArray[0] = endToken;
	}

	return exprArray;
}

// Pops operands off the evaluation stack, applies the next mathematical
// procedure, and pushes the result back.
// Returns the status of the evaluation.
Status popAndEval(Stack* opStack, Stack* evalStack) {
	void** opToken = malloc(sizeof(void*));
	stackPop(opStack, opToken);

	FunctionType functionKey = functionType(*opToken);

	if(functionKey != none) {
		void** operand = malloc(sizeof(void*));
		stackPop(evalStack, operand);

		double* result = applyFunction(functionKey, *operand);
		stackPush(evalStack, result);

		free(*opToken);
		free(opToken);
		free(*operand);
		free(operand);
		return success;
	}

	// evalStack only can only support unary operations.
	if(getStackSize(evalStack) == 1) {
		if(**(char**)opToken == '-') {
				void** operand = malloc(sizeof(void*));
				stackPop(evalStack, operand);

				**(double**)operand *= -1;
				stackPush(evalStack, *operand);

				free(operand);
		} else if(**(char**)opToken != '+' && **(char**)opToken != '-') {
			free(*opToken);
			free(opToken);
			return evalFail;
		}

	// evalStack has enough items for binary operator evaluation.
	} else {
		// (-) to be treated as a negative sign.
		if(getStackSize(opStack) >= 1
				&& *(char*)stackPeek(opStack) != '('
				&& getPriority(*opToken, stackPeek(opStack)) <= 0
				&& **(char**)opToken == '-') {

			void** operand = malloc(sizeof(void*));
			stackPop(evalStack, operand);

			**(double**)operand *= -1;
			stackPush(evalStack, *operand);

			free(operand);
		} else {
			void** lOperand = malloc(sizeof(void*));
			void** rOperand = malloc(sizeof(void*));
			double *result;
			stackPop(evalStack, rOperand);
			stackPop(evalStack, lOperand);

			if((**(double**)rOperand) == 0 && **(char**)opToken == '/') {
				free(*opToken);
				free(opToken);
				free(*lOperand);
				free(lOperand);
				free(*rOperand);
				free(rOperand);
				return divZero;
			}

			result = applyOperation(*opToken,*lOperand,*rOperand);

			stackPush(evalStack, result);
			free(*lOperand);
			free(lOperand);
			free(*rOperand);
			free(rOperand);
		}
	}

	free(*opToken);
	free(opToken);
	return success;
}

// Applies simple arithmetic operations.
double* applyOperation(void* operator, void* lOperandPtr, void* rOperandPtr) {
	double lOperand = *(double*)lOperandPtr;
	double rOperand = *(double*)rOperandPtr;
	double* result = malloc(sizeof(double));

	switch(*(char*)operator) {
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

// Applies single argument functions on a given operand.
double* applyFunction(FunctionType functionKey, void* operandPtr) {
	double operand = *(double*)operandPtr;
	double* result = malloc(sizeof(double));

	switch(functionKey) {
		case sqrt_:
			*result = sqrt(operand);
			break;
		case sin_:
			*result = sin(operand);
			break;
		case cos_:
			*result = cos(operand);
			break;
		case tan_:
			*result = tan(operand);
			break;
		default:
			result = NULL;
	}

	return result;
}

// Get the priority of operator1 relative to operator2.
int getPriority(void* operator1, void* operator2) {
	int priority1, priority2;

	switch(*(char*)operator1) {
		case '+':
		case '-':
			priority1 = 1;
			break;
		case '*':
		case '/':
			priority1 = 2;
			break;
		case '^':
			priority1 = 3;
			break;
	}

	switch(*(char*)operator2) {
		case '+':
		case '-':
			priority2 = 1;
			break;
		case '*':
		case '/':
			priority2 = 2;
			break;
		case '^':
			priority2 = 3;
			break;
	}

	return priority1 - priority2;
}

// Returns the associativity of a given token.
AssocType getAssoc(void* operator) {
	switch(*(char*)operator) {
		case '^':
		case '!':
			return right;
		default:
			return left;
	}
}

// Returns the classification of a given token.
TokenType tokenType(void* token) {
	if(token == NULL) {
		return unknown;
	}

	char* charToken = (char*)token;

	switch(*charToken) {
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

	if(strncmp(charToken, "pi", 2) == 0
			|| strncmp(charToken, "e", 1) == 0
			|| strncmp(charToken, "ans", 3) == 0) {

		return constant;
	}

	if(functionType(charToken)) {
		return function;
	}

	return unknown;
}

// Returns the function type of a given token.
FunctionType functionType(void* token) {
	char* charToken = (char*) token;

	if(strncmp(charToken, "sqrt", 4) == 0) {
		return sqrt_;
	}else if(strncmp(charToken, "sin", 3) == 0) {
		return sin_;
	} else if(strncmp(charToken, "cos", 3) == 0) {
		return cos_;
	} else if(strncmp(charToken, "tan", 3) == 0) {
		return tan_;
	}

	return none;
}

// Prints the corresponding message to the supplied status.
void printStatus(Status status) {
	switch(status) {
		case evalFail:
			fprintf(stderr, "Error: Failed to evaluate expression.\n");
			break;
		case divZero:
			fprintf(stderr, "Error: Division by zero.\n");
			break;
		case unpairedBracket:
			fprintf(stderr, "Error: Unpaired brackets.\n");
			break;
		case noDigit:
			fprintf(stderr, "Error: operands must contain at least one digit.\n");
			break;
		case noOperator:
			fprintf(stderr, "Error: expressions must contain at least one operator.\n");
			break;
		case extraDecimalSep:
			fprintf(stderr, "Error: Extra decimal point.\n");
			break;
		// Success or error handled elsewhere.
		default:
			break;
	}
}
