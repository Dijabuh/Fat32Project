
typedef struct
{
	char** parts;
	int numParts;
} pathparts;

void splitString(pathparts* ret, char* str, char* c);
