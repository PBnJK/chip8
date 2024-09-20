/* Chip-8 code decompiler
 *
 * This subprogram decompiles chip8 code into human-readable assembly language,
 * which is useful for debugging/reverse-engineered compiled programs.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chip8.h"
#include "util.h"
#include "decompile.h"

static bool _verbose = false;

static uint16_t _getraw(const Instr OP) {
	return (OP.op << 12) | OP.nnn;
}

#define PRINTOP(S, ...)                                                        \
	(fprintf(                                                                  \
		file, "0x%04X: %04X " S "\n", 0x0200 + i, _getraw(OP), ##__VA_ARGS__))

#define VPRINT(Y, N) (_verbose ? Y : N)

static void _op0(int i, const Instr OP, FILE *file) {
	switch( OP.nn ) {
	case 0xE0:
		VPRINT(PRINTOP("Clear the screen"), PRINTOP("CLS"));
		break;
	case 0xEE:
		VPRINT(PRINTOP("Return from subroutine"), PRINTOP("RET"));
		break;
	default:
		VPRINT(PRINTOP("Run assembly @ %04X (might be data)", OP.nnn),
			PRINTOP("SYS %04X", OP.nnn));
	}
}

static void _op1(int i, const Instr OP, FILE *file) {
	VPRINT(PRINTOP("Jump to %04X", OP.nnn), PRINTOP("JP %04X", OP.nnn));
}

static void _op2(int i, const Instr OP, FILE *file) {
	VPRINT(PRINTOP("Call subroutine @ %04X", OP.nnn),
		PRINTOP("CALL %04X", OP.nnn));
}

static void _op3(int i, const Instr OP, FILE *file) {
	VPRINT(PRINTOP("Skip next if V%X == %X", OP.x, OP.nn),
		PRINTOP("SE V%X, %X", OP.x, OP.nn));
}

static void _op4(int i, const Instr OP, FILE *file) {
	VPRINT(PRINTOP("Skip next if V%X != %X", OP.x, OP.nn),
		PRINTOP("SNE V%X, %X", OP.x, OP.nn));
}

static void _op5(int i, const Instr OP, FILE *file) {
	VPRINT(PRINTOP("Skip next if V%X == V%X", OP.x, OP.y),
		PRINTOP("SE V%X, V%X", OP.x, OP.y));
}

static void _op6(int i, const Instr OP, FILE *file) {
	VPRINT(PRINTOP("Set V%X to %X", OP.x, OP.nn),
		PRINTOP("LD V%X, %X", OP.x, OP.nn));
}

static void _op7(int i, const Instr OP, FILE *file) {
	VPRINT(PRINTOP("Add %X to V%x", OP.nn, OP.x),
		PRINTOP("ADD V%X, %X", OP.x, OP.nn));
}

static void _op8(int i, const Instr OP, FILE *file) {
	switch( OP.n ) {
	case 0x0:
		VPRINT(PRINTOP("Set V%X to V%X", OP.x, OP.y),
			PRINTOP("LD V%X, V%X", OP.x, OP.y));
		break;
	case 0x1:
		VPRINT(PRINTOP("OR V%X with V%X", OP.x, OP.y),
			PRINTOP("OR V%X, V%X", OP.x, OP.y));
		break;
	case 0x2:
		VPRINT(PRINTOP("AND V%X with V%X", OP.x, OP.y),
			PRINTOP("AND V%X, V%X", OP.x, OP.y));
		break;
	case 0x3:
		VPRINT(PRINTOP("XOR V%X with V%X", OP.x, OP.y),
			PRINTOP("XOR V%X, V%X", OP.x, OP.y));
		break;
	case 0x4:
		VPRINT(PRINTOP("Add V%X to V%X", OP.y, OP.x),
			PRINTOP("ADD V%X, V%X", OP.x, OP.y));
		break;
	case 0x5:
		VPRINT(PRINTOP("Subtract V%X from V%X", OP.y, OP.x),
			PRINTOP("SUB V%X, V%X", OP.x, OP.y));
		break;
	case 0x6:
		VPRINT(PRINTOP("Set V%X to V%X >> 1", OP.x, OP.y),
			PRINTOP("SHR V%X, V%X", OP.x, OP.y));
		break;
	case 0x7:
		VPRINT(PRINTOP("Subtract V%X from V%X, store in V%X", OP.x, OP.y, OP.x),
			PRINTOP("SUBN V%X, V%X", OP.x, OP.y));
		break;
	case 0xE:
		VPRINT(PRINTOP("Set V%X to V%X << 1", OP.x, OP.y),
			PRINTOP("SHL V%X, V%X", OP.x, OP.y));
		break;
	default:
		VPRINT(PRINTOP("Unknown instruction (might be data)"), PRINTOP("???"));
	}
}

static void _op9(int i, const Instr OP, FILE *file) {
	VPRINT(PRINTOP("Skip next if V%X != V%X", OP.x, OP.y),
		PRINTOP("SNE V%X, V%X", OP.x, OP.y));
}

static void _opA(int i, const Instr OP, FILE *file) {
	VPRINT(PRINTOP("Set I to %04X", OP.nnn), PRINTOP("LD I, %04X", OP.nnn));
}

static void _opB(int i, const Instr OP, FILE *file) {
	VPRINT(
		PRINTOP("Jump to V0 + %04X", OP.nnn), PRINTOP("JP V0, %04X", OP.nnn));
}

static void _opC(int i, const Instr OP, FILE *file) {
	VPRINT(PRINTOP("Set V%X to random byte w/ mask %X", OP.x, OP.nn),
		PRINTOP("RND V%X, %X", OP.x, OP.nn));
}

static void _opD(int i, const Instr OP, FILE *file) {
	VPRINT(PRINTOP("Draw %X-byte long sprite to (V%X, V%X)", OP.n, OP.x, OP.y),
		PRINTOP("DRAW V%X, V%X, %X", OP.x, OP.y, OP.n));
}

static void _opE(int i, const Instr OP, FILE *file) {
	switch( OP.nn ) {
	case 0x9E:
		VPRINT(PRINTOP("Skip next if key %X is pressed", OP.x),
			PRINTOP("SKP V%X", OP.x));
		break;
	case 0xA1:
		VPRINT(PRINTOP("Skip next if key %X is not pressed", OP.x),
			PRINTOP("SKNP V%X", OP.x));
		break;
	default:
		VPRINT(PRINTOP("Unknown instruction (might be data)"), PRINTOP("???"));
	}
}

static void _opF(int i, const Instr OP, FILE *file) {
	switch( OP.nn ) {
	case 0x07:
		VPRINT(PRINTOP("Load delay timer into V%X", OP.x),
			PRINTOP("LD V%X, DT", OP.x));
		break;
	case 0x0A:
		VPRINT(PRINTOP("Wait for key, store in V%X", OP.x),
			PRINTOP("LD V%X, K", OP.x));
		break;
	case 0x15:
		VPRINT(PRINTOP("Set delay timer to V%X", OP.x),
			PRINTOP("LD DT, V%X", OP.x));
		break;
	case 0x18:
		VPRINT(PRINTOP("Set sound timer to V%X", OP.x),
			PRINTOP("LD ST, V%X", OP.x));
		break;
	case 0x1E:
		VPRINT(PRINTOP("Add V%X to I", OP.x), PRINTOP("ADD I, V%X", OP.x));
		break;
	case 0x29:
		VPRINT(PRINTOP("Load digit V%X address into I", OP.x),
			PRINTOP("LD F, V%X", OP.x));
		break;
	case 0x33:
		VPRINT(PRINTOP("Store BCD of V%X into I...I+2", OP.x),
			PRINTOP("LD B, V%X", OP.x));
		break;
	case 0x55:
		VPRINT(PRINTOP("Store V0...V%X starting at I", OP.x),
			PRINTOP("LD [I], V%X", OP.x));
		break;
	case 0x65:
		VPRINT(PRINTOP("Read V0...V%X starting at I", OP.x),
			PRINTOP("LD V%X, [I]", OP.x));
		break;
	default:
		VPRINT(PRINTOP("Unknown instruction (might be data)"), PRINTOP("???"));
	}
}

typedef void (*decompFunc)(int, Instr, FILE *);

static const decompFunc opTable[] = {
	_op0,
	_op1,
	_op2,
	_op3,
	_op4,
	_op5,
	_op6,
	_op7,
	_op8,
	_op9,
	_opA,
	_opB,
	_opC,
	_opD,
	_opE,
	_opF,
};

static int _getInfile(const char *PATH, FILE **out) {
	*out = fopen(PATH, "w");
	if( *out == NULL ) {
		fprintf(stderr, "ERR: Couldn't open file '%s'\n", PATH);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static void _output(int i, const Instr INSTR, FILE *file) {
	opTable[INSTR.op](i, INSTR, file);
}

static void _decompile(uint8_t *buffer, const size_t SIZE, FILE *file) {
	for( size_t i = 0; i < SIZE; i += 2 ) {
		const uint16_t INSTR = (buffer[i] << 8) | (buffer[i + 1]);
		_output(i, c8ParseInstruction(INSTR), file);
	}
}

static const char *HELP_STRING
	= "usage: chip8 decompile [options] [program]\n\n"
	  "options:\n"
	  "    -o, --out [file]... Outputs the result to a file\n"
	  "    -v, --verbose...... Outputs plain english instead of assembly\n";

static int _usage() {
	fprintf(stderr, "%s", HELP_STRING);
	return EXIT_FAILURE;
}

int decompMain(int argc, char *argv[]) {
	if( argc == 0 ) {
		return _usage();
	}

	char *file = NULL;
	FILE *output = NULL;
	while( *argv ) {
		if( strcmp(*argv, "help") == 0 || strcmp(*argv, "--help") == 0 ) {
			_usage();
			return EXIT_SUCCESS;
		} else if( strcmp(*argv, "-o") == 0 || strcmp(*argv, "--out") == 0 ) {
			++argv;
			if( _getInfile(*argv, &output) == EXIT_FAILURE ) {
				return EXIT_FAILURE;
			}
		} else if( strcmp(*argv, "-v") == 0
			|| strcmp(*argv, "--verbose") == 0 ) {
			_verbose = true;
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
		if( output ) {
			fclose(output);
		}

		return _usage();
	}
	uint8_t *buffer = NULL;
	const size_t BYTES_READ = utilLoadBinaryFile(file, &buffer);

	if( BYTES_READ == -1 ) {
		return EXIT_FAILURE;
	}

	if( output ) {
		fprintf(output, "%s, %zu bytes long\n\n", file, BYTES_READ);
		_decompile(buffer, BYTES_READ, output);
	} else {
		printf("%s, %zu bytes long\n\n", file, BYTES_READ);
		_decompile(buffer, BYTES_READ, stdout);
	}

	if( output ) {
		fclose(output);
	}

	return EXIT_SUCCESS;
}
