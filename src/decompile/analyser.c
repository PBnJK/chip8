/* Chip-8 decompiler -- Code analyser
 *
 * Analyses and builds a model to help in the decompilation
 */

#include <stdbool.h>

#include "chip8.h"
#include "analyser.h"

static void _addToVec16(Vec16 *vec16, uint16_t value) {
	vec16->list[vec16->size++] = value;
}

static void _addJmp(Analyser *anl, uint16_t from, uint16_t to) {
	_addToVec16(&anl->jumps, from);
	_addToVec16(&anl->jumps, to);
}

static void _addSub(Analyser *anl, uint16_t from, uint16_t to) {
	_addToVec16(&anl->subroutines, from);
	_addToVec16(&anl->subroutines, to);
}

static void _addSkip(Analyser *anl, uint16_t from) {
	_addToVec16(&anl->subroutines, from);
}

static void _analyse(Analyser *anl, size_t addr, const Instr INSTR) {
	switch( INSTR.op ) {
	case 0x1:
		_addJmp(anl, addr, INSTR.nnn);
		break;
	case 0xB:
		_addJmp(anl, addr, INSTR.nnn | (1 << 15));
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
	};
}

void anlAnalyse(Analyser *anl) {
	for( size_t i = 0; i < anl->size; i += 2 ) {
		const uint16_t INSTR = (anl->buffer[i] << 8) | (anl->buffer[i + 1]);
		_analyse(anl, i, c8ParseInstruction(INSTR));
	}
}
