/* Chip-8 decompiler -- Code analyser
 *
 * Analyses and builds a model to help in the decompilation
 */

#include <stdbool.h>
#include <stdio.h>

#include "chip8.h"
#include "analyser.h"

bool lastWasSkip = false;

static void _addToVec16(Vec16 *vec16, uint16_t value) {
	vec16->list[vec16->size++] = value;
}

static void _setVec16(Vec16 *vec16, uint16_t value) {
	vec16->list[vec16->size] = value;
}

static void _addJmp(Analyser *anl, uint16_t from, uint16_t to) {
	_addToVec16(&anl->jumps, 0x0200 + from);
	_addToVec16(&anl->jumps, to);
}

static void _addSub(Analyser *anl, uint16_t from, uint16_t to) {
	_addToVec16(&anl->subroutines, 0x0200 + from);
	_addToVec16(&anl->subroutines, to);
}

static void _addSkip(Analyser *anl, uint16_t addr) {
	_addToVec16(&anl->skips, 0x0200 + addr);
}

static void _setUnreachableCodeStart(Analyser *anl, uint16_t addr) {
	_setVec16(&anl->unreachable, 0x0200 + addr);
}

static void _setUnreachableCodeEndIfNotSkipped(Analyser *anl, uint16_t addr) {
	if( lastWasSkip ) {
		_setUnreachableCodeStart(anl, addr);
	} else {
		_addToVec16(&anl->unreachable, addr);
		++anl->unreachable.size;
	}
}

static void _analyse(Analyser *anl, size_t addr, const Instr INSTR) {
	switch( INSTR.op ) {
	case 0x1:
		_addJmp(anl, addr, INSTR.nnn);
		_setUnreachableCodeEndIfNotSkipped(anl, addr);
		break;
	case 0xB:
		_addJmp(anl, addr, INSTR.nnn | (1 << 15));
		_setUnreachableCodeEndIfNotSkipped(anl, addr);
		break;
	case 0x2:
		_addSub(anl, addr, INSTR.nnn);
		break;
	case 0x3:
	case 0x4:
	case 0x5:
	case 0x9:
	case 0xE:
		_addSkip(anl, addr);
		break;
	}
}

Analyser anlInit(uint8_t *buffer, size_t size) {
	return (Analyser) {
		.buffer = buffer,
		.size = size,
		.subroutines = { 0 },
		.jumps = { 0 },
		.unreachable = { 0 },
		.skips = { 0 },
	};
}

void anlAnalyse(Analyser *anl) {
	for( size_t i = 0; i < anl->size; i += 2 ) {
		const uint16_t INSTR = (anl->buffer[i] << 8) | (anl->buffer[i + 1]);
		_analyse(anl, i, c8ParseInstruction(INSTR));
	}
}

static void _printVec16Pairs(Vec16 *vec16) {
	puts("[");

	for( size_t i = 0; i < vec16->size; i += 2 ) {
		printf("    %04X -> %04X\n", vec16->list[i], vec16->list[i + 1]);
	}

	puts("]");
}

static void _printVec16(Vec16 *vec16) {
	puts("[");

	printf("   ");
	for( size_t i = 0; i < vec16->size; ++i ) {
		printf(" %04X,", vec16->list[i]);
		if( i % 8 == 7 ) {
			printf("\n   ");
		}
	}

	puts("\n]");
}

void anlPrint(Analyser *anl) {
	printf("subs ");
	_printVec16Pairs(&anl->subroutines);

	printf("\njmps ");
	_printVec16Pairs(&anl->jumps);

	printf("\nunreach ");
	_printVec16Pairs(&anl->unreachable);

	printf("\nskips ");
	_printVec16(&anl->skips);
}
