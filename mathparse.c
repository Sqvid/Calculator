#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "Lists/Stacks/stack.h"
#include "Lists/Queues/queue.h"

#define BUFFER 128
#define CHUNK_SIZE 8

int isOperator(char token);
Queue* strToMathQueue(char* inputString, int maxStringSize);

int main(void){
	char inputString[BUFFER];
	void** data;

	printf("cal>> ");
	fgets(inputString, sizeof(inputString), stdin);

	Queue* mathQueue = strToMathQueue(inputString, BUFFER);

	if(mathQueue == NULL){
		return -1;
	}

	while(getQueueSize(mathQueue) > 0){
		dequeue(mathQueue, data);
		printf("%s\n", *(char**)data);
	}

	queueDestroy(mathQueue);
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
		} else if(isdigit(token)){
			int numLen = 1, tokenSize = CHUNK_SIZE;
			char* numToken = malloc(tokenSize);

			// Count the length of the number by iterating until
			// token is no longer a digit or decimal point.
			while(isdigit(inputString[i + numLen]) || (token == '.')){
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
		//Ignore spaces and tabs.
		} else if(token == ' ' || token == '\t'){
			continue;
		} else if(strncmp(inputString, "quit\n", 5) == 0){
			queueDestroy(mathQueue);
			return NULL;
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
