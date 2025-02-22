#include <stdlib.h>

#define JAY_IMPLEMENTATION
#include "jay.h"

#define BUFFER_SIZE 1024

int main() {
	char buffer[BUFFER_SIZE];
	size_t length = 0;

	while (fgets(buffer + length, BUFFER_SIZE - length, stdin)) {
		length += strlen(buffer + length);
		if (length >= BUFFER_SIZE - 1) {
			fprintf(stderr, "Input too long\n");
			return EXIT_FAILURE;
		}
	}

	buffer[length] = '\0';

	json_value *jsonValue = jay_parse(buffer);
	jay_print(jsonValue, 0);
	printf("\n");

	return 0;
}
