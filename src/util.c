/* Utility functions
 *
 */

#include <stdio.h>

#include "util.h"

size_t utilLoadBinaryFile(const char *PATH, uint8_t **buffer) {
	FILE *file;

	file = fopen(PATH, "rb");
	if( file == NULL ) {
		fprintf(stderr, "ERR: Couldn't open file '%s'\n", PATH);
		return -1;
	}

	fseek(file, 0L, SEEK_END);
	const size_t FILESIZE = ftell(file);
	rewind(file);

	*buffer = malloc(FILESIZE + 1);
	if( *buffer == NULL ) {
		fprintf(stderr,
			"ERR: Couldn't allocate memory (%zu bytes) for program\n",
			FILESIZE + 1);
		return -1;
	}

	const size_t BYTES_READ = fread(*buffer, 1, FILESIZE, file);
	if( BYTES_READ < FILESIZE ) {
		fprintf(stderr,
			"ERR: Couldn't fully read file (read %zu bytes, file is %zu bytes "
			"long)\n",
			BYTES_READ, FILESIZE);
		return -1;
	}

	(*buffer)[BYTES_READ] = '\0';

	return BYTES_READ;
}
