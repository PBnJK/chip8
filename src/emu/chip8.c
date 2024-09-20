#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "chip8.h"
#include "util.h"

#define FONT_START_ADDR 0x50
#define FONT_SIZE 0x50

#define PROGRAM_START_ADDR 0x200

static const uint8_t CHIP8_FONT[FONT_SIZE] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0, /* 0 */
	0x20, 0x60, 0x20, 0x20, 0x70, /* 1 */
	0xF0, 0x10, 0xF0, 0x80, 0xF0, /* 2 */
	0xF0, 0x10, 0xF0, 0x10, 0xF0, /* 3 */
	0x90, 0x90, 0xF0, 0x10, 0x10, /* 4 */
	0xF0, 0x80, 0xF0, 0x10, 0xF0, /* 5 */
	0xF0, 0x80, 0xF0, 0x90, 0xF0, /* 6 */
	0xF0, 0x10, 0x20, 0x40, 0x40, /* 7 */
	0xF0, 0x90, 0xF0, 0x90, 0xF0, /* 8 */
	0xF0, 0x90, 0xF0, 0x10, 0xF0, /* 9 */
	0xF0, 0x90, 0xF0, 0x90, 0x90, /* A */
	0xE0, 0x90, 0xE0, 0x90, 0xE0, /* B */
	0xF0, 0x80, 0x80, 0x80, 0xF0, /* C */
	0xE0, 0x90, 0x90, 0x90, 0xE0, /* D */
	0xF0, 0x80, 0xF0, 0x80, 0xF0, /* E */
	0xF0, 0x80, 0xF0, 0x80, 0x80, /* F */
};

Chip8 c8New(void) {
	srand(time(NULL));

	Chip8 c8 = { 0 };

	c8.pc = PROGRAM_START_ADDR;
	memcpy(&c8.mem[FONT_START_ADDR], CHIP8_FONT, FONT_SIZE);

	return c8;
}

int c8Load(Chip8 *c8, uint8_t *program, size_t size) {
	if( size > 0xFFF ) {
		fprintf(stderr,
			"ERR: Tried to load program that's too big\n"
			"     (is 0x%zu, more than maximum of 4095)\n",
			size);
		return EXIT_FAILURE;
	}

	/* Copy program to memory */
	memcpy(&c8->mem[PROGRAM_START_ADDR], program, size);
	free(program);

	return EXIT_SUCCESS;
}

int c8LoadFile(Chip8 *c8, const char *PATH) {
	uint8_t *buffer = NULL;
	const size_t BYTES_READ = utilLoadBinaryFile(PATH, &buffer);

	return c8Load(c8, buffer, BYTES_READ);
}

static void _advance(Chip8 *c8) {
	c8->pc += 2;
}

static void _backtrack(Chip8 *c8) {
	c8->pc -= 2;
}

static void _push(Chip8 *c8, uint16_t value) {
	c8->stack[c8->sp++] = value;
}

static uint16_t _pop(Chip8 *c8) {
	return c8->stack[--c8->sp];
}

static void _setflag(Chip8 *c8, bool value) {
	c8->v[0xF] = value ? 1 : 0;
}

static void _jump(Chip8 *c8, uint16_t addr) {
	c8->pc = addr - 2;
}

/* 0??? opcodes */
static void op0(Chip8 *c8, Instr op) {
	switch( op.nn ) {
	/* 00E0 -> Clears the screen */
	case 0xE0:
		for( int y = 0; y < SCR_HEIGHT; ++y ) {
			for( int x = 0; x < SCR_WIDTH; ++x ) {
				c8->display[y][x] = 0;
			}
		}

		break;
	/* 00EE -> Returns from subroutine */
	case 0xEE:
		c8->pc = _pop(c8);
		break;
	}
}

/* 1NNN -> Jump to address NNN */
static void op1(Chip8 *c8, Instr op) {
	_jump(c8, op.nnn);
}

/* 2NNN -> Call subroutine NNN */
static void op2(Chip8 *c8, Instr op) {
	_push(c8, c8->pc);
	_jump(c8, op.nnn);
}

/* 3XNN -> Skip next if VX == NN */
static void op3(Chip8 *c8, Instr op) {
	if( c8->v[op.x] == op.nn ) {
		_advance(c8);
	}
}

/* 4XNN -> Skip next if VX != NN */
static void op4(Chip8 *c8, Instr op) {
	if( c8->v[op.x] != op.nn ) {
		_advance(c8);
	}
}

/* 5XY0 -> Skip next if VX == VY */
static void op5(Chip8 *c8, Instr op) {
	if( c8->v[op.x] == c8->v[op.y] ) {
		_advance(c8);
	}
}

/* 6XNN -> Set VX to NN */
static void op6(Chip8 *c8, Instr op) {
	c8->v[op.x] = op.nn;
}

/* 7XNN -> Add NN to VX */
static void op7(Chip8 *c8, Instr op) {
	c8->v[op.x] += op.nn;
}

/* 8??? opcodes */
static void op8(Chip8 *c8, Instr op) {
	switch( op.n ) {
	/* 8XY0 -> Set VX to VY */
	case 0x0:
		c8->v[op.x] = c8->v[op.y];
		break;
	/* 8XY1 -> OR VX with VY, store in VX */
	case 0x1:
		c8->v[op.x] |= c8->v[op.y];
		break;
	/* 8XY2 -> AND VX with VY, store in VX */
	case 0x2:
		c8->v[op.x] &= c8->v[op.y];
		break;
	/* 8XY3 -> XOR VX with VY, store in VX */
	case 0x3:
		c8->v[op.x] ^= c8->v[op.y];
		break;
	/* 8XY4 -> Add VY to VX. VF indicates if a carry occured */
	case 0x4: {
		const uint16_t VALUE = c8->v[op.x] + c8->v[op.y];
		c8->v[op.x] = VALUE;

		_setflag(c8, VALUE > UINT8_MAX);
	} break;
	/* 8XY5 -> Subtract VY from VX. VF indicates if a borrow occured */
	case 0x5: {
		const bool LARGER = c8->v[op.x] >= c8->v[op.y];
		c8->v[op.x] -= c8->v[op.y];

		_setflag(c8, LARGER);
	} break;
	/* 8XY6 -> Shift VY right by 1 bit, store in VX. VF holds VX's least
	 * significant bit
	 */
	case 0x6: {
		const uint8_t LSB = c8->v[op.x] & 1;

		c8->v[op.x] = c8->v[op.y] >> 1;
		c8->v[0xF] = LSB;
	} break;
	/* 8XY7 -> Set VX = VY - VX. VF indicates if a borrow occured */
	case 0x7: {
		const bool LARGER = c8->v[op.y] >= c8->v[op.x];
		c8->v[op.x] = c8->v[op.y] - c8->v[op.x];

		_setflag(c8, LARGER);
	} break;
	/* 8XYE -> Shift VY left by 1 bit, store in VX. VF holds VX's most
	 * significant bit
	 */
	case 0xE: {
		const uint8_t MSB = c8->v[op.x] >> 7;

		c8->v[op.x] = c8->v[op.y] << 1;
		c8->v[0xF] = MSB;
	} break;
	}
}

/* 9XY0 -> Skip next if VX != VY */
static void op9(Chip8 *c8, Instr op) {
	if( c8->v[op.x] != c8->v[op.y] ) {
		_advance(c8);
	}
}

/* ANNN -> Set I to NNN */
static void opA(Chip8 *c8, Instr op) {
	c8->i = op.nnn;
}

/* BNNN -> Jump to address NNN + V0 */
static void opB(Chip8 *c8, Instr op) {
	c8->pc = op.nnn + c8->v[0x0];
}

/* CXNN -> Set VX to a random number ANDed with NN */
static void opC(Chip8 *c8, Instr op) {
	uint8_t randbyte = rand() % 256;
	c8->v[op.x] = randbyte & op.nn;
}

/* DXYN -> Draw a sprite at VX, VY
 *
 * The sprite is fetched starting from the address stored in I, with N
 * representing the size of the sprite data
 *
 * VF represents if any set pixels were changed to unset
 */
static void opD(Chip8 *c8, Instr op) {
	c8->v[0xF] = 0;

	uint8_t px = c8->v[op.x] % SCR_WIDTH;
	uint8_t py = c8->v[op.y] % SCR_HEIGHT;

	for( int row = 0; row < op.n; ++row ) {
		int spriteByte = c8->mem[c8->i + row];

		for( int col = 0; col < 8; ++col ) {
			int ix = (px + col) % SCR_WIDTH;
			int iy = (py + row) % SCR_HEIGHT;
			uint8_t *scrPixel = &c8->display[iy][ix];

			int sprPixel = (spriteByte >> (7 - col)) & 1;
			if( sprPixel == 1 ) {
				if( *scrPixel == 0xFF ) {
					c8->v[0xF] = 1;
				}

				*scrPixel ^= 0xFF;
			}
		}
	}

	c8->dirty = true;
}

/* E??? opcodes */
static void opE(Chip8 *c8, Instr op) {
	switch( op.nn ) {
	/* EX9E -> Skip next if the key VX is pressed */
	case 0x9E:
		if( c8->keypad[op.x] ) {
			_advance(c8);
		}
		break;
	/* EXA1 -> Skip next if the key VX is not pressed */
	case 0xA1:
		if( !c8->keypad[op.x] ) {
			_advance(c8);
		}
		break;
	}
}

/* F??? opcodes */
static void opF(Chip8 *c8, Instr op) {
	switch( op.nn ) {
	/* FX07 -> Set VX to the delay timer */
	case 0x07:
		c8->v[op.x] = c8->timers.dt;
		break;
	/* FX0A -> Wait for a keypress, store the key in VX */
	case 0x0A:
		for( int i = 0; i < 16; ++i ) {
			if( c8->keypad[op.x] ) {
				c8->v[op.x] = i;
				return;
			}
		}

		_backtrack(c8);
		break;
	/* FX15 -> Set the delay timer to VX */
	case 0x15:
		c8->timers.dt = c8->v[op.x];
		break;
	/* FX18 -> Set the sound timer to VX */
	case 0x18:
		c8->timers.st = c8->v[op.x];
		break;
	/* FX1E -> Add the value in VX to I */
	case 0x1E:
		c8->i += c8->v[op.x];
		break;
	/* FX29 -> Set I to the address of the sprite for the digit VX */
	case 0x29:
		c8->i = FONT_START_ADDR + (c8->v[op.x] * 5);
		break;
	/* FX33 -> Store the BCD representation of VX at I..=I + 2 */
	case 0x33: {
		uint8_t value = c8->v[op.x];

		c8->mem[c8->i + 2] = value % 10;
		value /= 10;

		c8->mem[c8->i + 1] = value % 10;
		value /= 10;

		c8->mem[c8->i] = value % 10;
	} break;
	/* FX55 -> Store V0..=VX in memory starting at I. Adds X + 1 to I */
	case 0x55:
		for( int i = 0; i <= op.x; ++i ) {
			c8->mem[c8->i + i] = c8->v[i];
		}

		c8->i += op.x + 1;
		break;
	/* FX65 -> Set V0..=VX to values in memory starting at I. Adds X + 1
	 * to I
	 */
	case 0x65:
		for( int i = 0; i <= op.x; ++i ) {
			c8->v[i] = c8->mem[c8->i + i];
		}

		c8->i += op.x + 1;
	}
}

typedef void (*opFunc)(Chip8 *, Instr);

static const opFunc opTable[] = {
	op0,
	op1,
	op2,
	op3,
	op4,
	op5,
	op6,
	op7,
	op8,
	op9,
	opA,
	opB,
	opC,
	opD,
	opE,
	opF,
};

static Instr _fetch(Chip8 *c8) {
	const uint16_t OPCODE = (c8->mem[c8->pc] << 8) | (c8->mem[c8->pc + 1]);
	return c8ParseInstruction(OPCODE);
}

Instr c8ParseInstruction(const uint16_t INSTR) {
	return (Instr) {
		.op = INSTR >> 12,
		.x = (INSTR >> 8) & 0xF,
		.y = (INSTR >> 4) & 0xF,
		.n = INSTR & 0xF,
		.nn = INSTR & 0xFF,
		.nnn = INSTR & 0xFFF,
	};
}

void c8Cycle(Chip8 *c8) {
	Instr instruction = _fetch(c8);
	opTable[instruction.op](c8, instruction);

	_advance(c8);
}
