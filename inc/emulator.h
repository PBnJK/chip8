#ifndef GUARD_EMULATOR_H_
#define GUARD_EMULATOR_H_

#include "chip8.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <SDL2/SDL.h>

typedef struct _Emulator {
	Chip8 c8;

	int delay; /* Delay, in nanoseconds */

	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *tex;
} Emulator;

/* Creates a new emulator */
int emuNew(Emulator *emu);

/* Runs the emulator from an array of bytes */
int emuRun(Emulator *emu, uint8_t *program, size_t size);

/* Runs the emulator from a file */
int emuRunFile(Emulator *emu, const char *PATH);

/* Set emulator delay */
void emuSetDelay(Emulator *emu, const int delayms);

/* Set emulator scaling factor. May fail */
int emuSetScaleFactor(Emulator *emu, const float SCALE);

/* Frees the emulator and quits SDL */
void emuQuit(Emulator *emu);

#endif // !GUARD_EMULATOR_H_
