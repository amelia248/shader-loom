#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errors.h>
#include <memory.h>

error_t fsReadFile(c_string_t path, char** buffer, size_t* len) {
	FILE *fptr = fopen(path, "r");
	if (fptr == NULL) return PH_ERR_FAILURE;

	size_t bufferSize = 0x1'0000;
	(*buffer) = (char*)memalloc(bufferSize);
	(*len) = 0;

	while (true) {
		int c = fgetc(fptr);
		if (c == EOF) break;

		if ((*len) >= bufferSize) {
			bufferSize += 0x1'0000;
			(*buffer) = realloc(*buffer, bufferSize);
		}
		(*buffer)[*len] = (char)c;
		(*len)++;
	};
	(*buffer)[*len] = 0;
	
	buffer = realloc(*buffer, (*len) + 1);

	fclose(fptr);
	return PH_ERR_SUCCESS;
}
