#include <string.h>
#include <stdlib.h>
#include "str_func.h"

void splitString(pathparts* ret, char* str, char* c) {
	char* temp = NULL;

	//initialize the return array
	ret->parts = (char**) malloc(sizeof(char*));
	ret->numParts = 0;

	temp = strtok(str, c);
	ret->parts = (char**) realloc(ret->parts, (ret->numParts + 1) * sizeof(char*));
	(ret->parts)[ret->numParts] = (char*) malloc((strlen(temp) + 1) * sizeof(char));
	strcpy((ret->parts)[ret->numParts], temp);
	(ret->numParts)++;

	//keep looping until we've gone through every part of the path
	while(temp != NULL) {
		temp = strtok(NULL, c);
		if (temp != NULL) {
			ret->parts = (char**) realloc(ret->parts, (ret->numParts + 1) * sizeof(char*));
			(ret->parts)[ret->numParts] = (char*) malloc((strlen(temp) + 1) * sizeof(char));
			strcpy((ret->parts)[ret->numParts], temp);
			(ret->numParts)++;
		}
	}

	free(temp);
}
