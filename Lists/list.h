#ifndef LIST_H
#define LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ListElement_{
	void* data;
	struct ListElement_* next;
} ListElement;

typedef struct List_{
	int size;

	ListElement* head;
	ListElement* tail;

	void (*destroyFunction)(void* data);
} List;

#define getElementData(element) element->data
#define getListSize(list) list->size
#define getListHead(list) list->head
#define getListTail(list) list->tail
#define getNextElement(element) element->next
#define isListHead(list, element) list->head == element ? 1 : 0
#define isListTail(list, element) list->tail == element ? 1 : 0

//void listInit(List* list, void (*destroyFunction)(void* data));
List* listCreate(void (*destroyFunction)(void* data));
int listAddNext(List* list, ListElement* element, const void* data);
int listDelNext(List* list, ListElement* element, void** data);
int listAddIndex(List* list, int index, const void* data);
int listDelIndex(List* list, int index, void** data);
int listCat(List* list1, List* list2);
void listDestroy(List* list);


// Function definitions:

List* listCreate(void (*destroyFunction)(void* data)){
	List* list = malloc(sizeof(List));

	list->size = 0;

	list->head = NULL;
	list->tail = NULL;

	// destroyFunction is the function used to deallocated data.
	// Use NULL for static allocation and free() for malloc, calloc, etc.
	list->destroyFunction = destroyFunction;

	return list;
}

int listAddNext(List* list, ListElement* element, const void* data){
	ListElement* newElement = malloc(sizeof(ListElement));

	if(newElement == NULL){
		fprintf(stderr, "Allocation of list element failed.\n");
		exit(1);
	}

	newElement->data = (void*) data;

	// Insert at head.
	if(element == NULL){
		newElement->next = list->head;
		list->head = newElement;

		// If this is the only element then the head is the tail.
		if(getListSize(list) == 0){
			list->tail = newElement;
		}
	// Insert element elsewhere.
	} else{
		// Insert at tail.
		if(isListTail(list, element)){
			list->tail = newElement;
		}

		newElement->next = element->next;
		element->next = newElement;
	}

	++(list->size);
	return 0;
}

// void** data will hold the data from the deleted element.
int listDelNext(List* list, ListElement* element, void** data){
	ListElement* oldElement;

	// Attempting removal of element from an empty list.
	if(getListSize(list) == 0){
		return -1;
	}

	if(element == NULL){
		oldElement = list->head;
		*data = oldElement->data;
		list->head = oldElement->next;

		if(list->size == 1){
			list->tail = list->head;
		}
	} else{
		// Attempting removal after tail.
		if(element == list->tail){
			return -1;
		}

		oldElement = element->next;
		*data = oldElement->data;
		element->next = oldElement->next;

		if(element->next == NULL){
			list->tail = element;
		}
	}

	--(list->size);
	free(oldElement);
	return 0;
}

// Adds a new element at the specified index of the list.
int listAddIndex(List* list, int index, const void* data){
	ListElement* current = getListHead(list);

	if(index > getListSize(list)){
		fprintf(stderr, "Attempting to add element at index %d, ", index);
		fprintf(stderr, "but list is only of size %d.\n", getListSize(list));
		exit(1);
	// Add at head.
	} else if(index == 0){
		listAddNext(list, NULL, data);
	// Add at tail.
	} else if(index == getListSize(list)){
		listAddNext(list, list->tail, data);
	// Add elsewhere.
	} else{
		for(int i=0; i<index-1; ++i){
			current = getNextElement(current);
		}

		listAddNext(list, current, data);
	}

	return 0;
}

int listDelIndex(List* list, int index, void** data){
	ListElement* current = getListHead(list);

	if(index > getListSize(list) - 1){
		fprintf(stderr, "There is no element to delete at the specified index.\n");
		exit(1);
	// Delete at head.
	} else if(index == 0){
		listDelNext(list, NULL, data);
	// Add elsewhere.
	} else{
		for(int i=0; i<index-1; ++i){
			current = getNextElement(current);
		}

		listDelNext(list, current, data);
	}

	return 0;
}

// Appends list2 to list1.
int listCat(List* list1, List* list2){
	if(getListSize(list1) == 0 || getListSize(list2) == 0){
		return -1;
	}

	list1->tail->next = list2->head;
	list1->tail = list2->tail;
	list1->size += list2->size;

	return 0;
}

// Safely delete list.
void listDestroy(List* list){
	void* data;

	while(getListSize(list) > 0){
		if(listDelNext(list, NULL, (void**) &data) == 0 \
				&& list->destroyFunction != NULL){

				list->destroyFunction(data);
		}
	}

	free(list);
}

#endif
