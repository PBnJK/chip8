/* Chip-8 emulator
 *
 * This subprogram runs a compiled chip8 program.
 * It supports most known extensions, such as the SCHIP and XO-CHIP.
 */

#include <stdio.h>
#include <stdlib.h>

#include "emulator.h"
#include "run.h"

static const char *HELP_STRING
	= "usage: chip8 run [options] [program]\n\n"
	  "options:\n"
	  "    -d, --delay [num]... Sets the cycle delay, in milliseconds\n"
	  "    -s, --scale [num]... Sets the scaling factor of the window\n";

static int _usage() {
	fprintf(stderr, "%s", HELP_STRING);
	return EXIT_FAILURE;
}

int runMain(int argc, char *argv[]) {
	if( argc == 0 ) {
		return _usage();
	}

	Emulator emu;
	if( emuNew(&emu) == EXIT_FAILURE ) {
		fprintf(stderr, "emuNew() failed! Exiting...\n");
		return EXIT_FAILURE;
	}

	char *file = NULL;
	while( *argv ) {
		if( strcmp(*argv, "help") == 0 || strcmp(*argv, "--help") == 0 ) {
			_usage();
			return EXIT_SUCCESS;
		} else if( strcmp(*argv, "-d") == 0 || strcmp(*argv, "--delay") == 0 ) {
			++argv;
			emuSetDelay(&emu, atoi(*argv));
		} else if( strcmp(*argv, "-s") == 0 || strcmp(*argv, "--scale") == 0 ) {
			++argv;
			if( emuSetScaleFactor(&emu, atof(*argv)) == EXIT_FAILURE ) {
				emuQuit(&emu);
				return EXIT_FAILURE;
			}
		} else if( strcmp(*argv, "-d") == 0 || strcmp(*argv, "--delay") == 0 ) {
		} else if( strcmp(*argv, "-d") == 0 || strcmp(*argv, "--delay") == 0 ) {
		} else if( strcmp(*argv, "-d") == 0 || strcmp(*argv, "--delay") == 0 ) {
		} else if( *(argv + 1) ) {
			fprintf(stderr, "ERR: Unknown option '%s'!\n\n", *argv);
			return _usage();
		} else {
			file = *argv;
			break;
		}

		++argv;
	}

	if( !file ) {
		fprintf(stderr, "ERR: An input file must be provided!\n\n");
		emuQuit(&emu);
		return _usage();
	}

	if( emuRunFile(&emu, file) == EXIT_FAILURE ) {
		fprintf(stderr, "emuRunFile() failed! Exiting...\n");
		emuQuit(&emu);
		return EXIT_FAILURE;
	}

	emuQuit(&emu);
	return EXIT_SUCCESS;
}
