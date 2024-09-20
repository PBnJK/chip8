/* Entry-point for the chip8 suite
 *
 * The program work much the same as the 'go' tool, where different functions
 * are executed by the same program instead of separated into different
 * executables.
 */

#include "decompile.h"
#include "run.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *HELP_STRING
	= "usage: chip8 [program] [options]\n\n"
	  "program:\n"
	  "    run......... runs a program\n"
	  "    compile..... compiles cc8 source code into a program\n"
	  "    decompile... decompiles a program\n\n"
	  "use 'chip8 [program] help' to get the possible options\n";

static int _usage(void) {
	fprintf(stderr, "%s", HELP_STRING);
	return EXIT_FAILURE;
}

int main(int argc, char *argv[]) {
	if( argc < 2 ) {
		return _usage();
	}

	++argv;
	--argc;
	if( strcmp(*argv, "run") == 0 ) {
		return runMain(--argc, ++argv);
	} else if( strcmp(*argv, "compile") == 0 ) {
	} else if( strcmp(*argv, "decompile") == 0 ) {
		return decompMain(--argc, ++argv);
	}

	fprintf(stderr, "ERR: Unknown program '%s'!\n\n", *argv);
	return _usage();
}
