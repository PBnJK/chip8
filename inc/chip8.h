#ifndef GUARD_CHIP8_H_
#define GUARD_CHIP8_H_

#define SCR_WIDTH 64
#define SCR_HEIGHT 32

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct _Chip8 {
	uint8_t mem[4 * 1024]; /* 4KB memory */

	uint16_t stack[16]; /* Address stack */
	uint8_t sp; /* Stack pointer */

	uint16_t pc; /* Program counter */
	uint16_t i; /* Index register */

	uint8_t v[16]; /* General-purpose registers */

	/* Internal timers */
	struct {
		uint8_t dt; /* Delay timer */
		uint8_t st; /* Sound timer */
	} timers;

	uint8_t display[SCR_HEIGHT][SCR_WIDTH]; /* VRAM */
	bool dirty; /* Signals that the screen needs to be refreshed */

	uint8_t keypad[16]; /* Keypad data */
} Chip8;

/* Represents a Chip-8 instruction */
typedef struct _Instr {
	uint8_t op; /* First nibble */

	uint8_t x; /* Second nibble */
	uint8_t y; /* Third nibble */

	uint8_t n; /* Fourth nibble */
	uint8_t nn; /* Second byte */
	uint16_t nnn; /* Second, third and fourth nibbles */
} Instr;

/* Creates a new Chip-8 interpreter */
Chip8 c8New(void);

/* Loads an array of bytes into program memory
 *
 * Returns EXIT_FAILURE if it fails
 */
int c8Load(Chip8 *c8, uint8_t *program, size_t size);

/* Loads a program into memory from a file
 *
 * Returns EXIT_FAILURE if it fails
 */
int c8LoadFile(Chip8 *c8, const char *PATH);

/* Parses a raw opcode into an Instruction */
Instr c8ParseInstruction(const uint16_t INSTR);

/* Executes one Chip-8 cycle */
void c8Cycle(Chip8 *c8);

#endif // !GUARD_CHIP8_H_
