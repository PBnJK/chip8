#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* OS-dependent code (clock_gettime replacement)
 *
 * Not checked on windows, but should work...?
 * Taken from: https://stackoverflow.com/a/31335254
 */
#if defined(_WIN32) || defined(WIN32)
#include <windows.h>
#include <sysinfoapi.h>

struct timespec {
	long tv_sec;
	long tv_nsec;
};

#define exp7 10000000i64 /* 1e+7*/
#define exp9 1000000000i64 /* 1e+9 */
#define w2ux 116444736000000000i64

void unix_time(struct timespec *spec) {
	__int64 wintime;
	GetSystemTimeAsFileTime((FILETIME *)&wintime);
	wintime -= w2ux;

	spec->tv_sec = wintime / exp7;
	spec->tv_nsec = wintime % exp7 * 100;
}

int clock_gettime(int, timespec *spec) {
	static struct timespec startspec;
	static double ticks2nano;
	static __int64 startticks;
	static __int64 tps = 0;

	__int64 tmp, curticks;

	QueryPerformanceFrequency((LARGE_INTEGER *)&tmp);
	if( tps != tmp ) {
		tps = tmp;
		QueryPerformanceCounter((LARGE_INTEGER *)&startticks);
		unix_time(&startspec);
		ticks2nano = (double)exp9 / tps;
	}

	QueryPerformanceCounter((LARGE_INTEGER *)&curticks);

	curticks -= startticks;

	spec->tv_sec = startspec.tv_sec + (curticks / tps);
	spec->tv_nsec = startspec.tv_nsec + (double)(curticks % tps) * ticks2nano;

	if( !(spec->tv_nsec < exp9) ) {
		spec->tv_sec++;
		spec->tv_nsec -= exp9;
	}

	return 0;
}
#endif

#include "SDL_error.h"
#include "SDL_keyboard.h"
#include "SDL_render.h"
#include "SDL_video.h"
#include "chip8.h"
#include "emulator.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define DEFAULT_SCALE_FACTOR 10.0f

static void _draw(Emulator *emu) {
	int pitch = 0;
	void *pixels = NULL;

	if( SDL_LockTexture(emu->tex, NULL, &pixels, &pitch) != 0 ) {
		fprintf(stderr, "ERR: Couldn't lock texture: %s\n", SDL_GetError());
	} else {
		memcpy(pixels, emu->c8.display, SCR_HEIGHT * SCR_WIDTH);
	}

	SDL_UnlockTexture(emu->tex);

	SDL_RenderClear(emu->renderer);
	SDL_RenderCopy(emu->renderer, emu->tex, NULL, NULL);
	SDL_RenderPresent(emu->renderer);

	emu->c8.dirty = false;
}

/*
 * 1 2 3 4      1 2 3 C
 * Q W E R      4 5 6 D
 * A S D F  ->  7 8 9 E
 * Z X C V      A 0 B F
 */
static int _keyindex(int key) {
	switch( key ) {
	case SDLK_x:
		return 0x0;
	case SDLK_1:
		return 0x1;
	case SDLK_2:
		return 0x2;
	case SDLK_3:
		return 0x3;
	case SDLK_q:
		return 0x4;
	case SDLK_w:
		return 0x5;
	case SDLK_e:
		return 0x6;
	case SDLK_a:
		return 0x7;
	case SDLK_s:
		return 0x8;
	case SDLK_d:
		return 0x9;
	case SDLK_z:
		return 0xA;
	case SDLK_c:
		return 0xB;
	case SDLK_4:
		return 0xC;
	case SDLK_r:
		return 0xD;
	case SDLK_f:
		return 0xE;
	case SDLK_v:
		return 0xF;
	default:
		return 0x0;
	}
}

#define NS_PER_SECOND 1000000000

static struct timespec _getTimeDiff(struct timespec t1, struct timespec t2) {
	struct timespec td;

	td.tv_nsec = t2.tv_nsec - t1.tv_nsec;
	td.tv_sec = t2.tv_sec - t1.tv_sec;

	if( td.tv_sec > 0 && td.tv_nsec < 0 ) {
		td.tv_nsec += NS_PER_SECOND;
		td.tv_sec--;
	} else if( td.tv_sec < 0 && td.tv_nsec > 0 ) {
		td.tv_nsec -= NS_PER_SECOND;
		td.tv_sec++;
	}

	return td;
}

static Uint32 _updateTimers(Uint32 interval, void *emu) {
	Chip8 *c8 = &((Emulator *)emu)->c8;
	if( c8->timers.dt > 0 ) {
		--c8->timers.dt;
	}

	if( c8->timers.st > 0 ) {
		--c8->timers.st;
	}

	return interval;
}

static int _run(Emulator *emu) {
	bool run = true;
	SDL_Event e;

	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);

	SDL_AddTimer(16, _updateTimers, emu);

	while( run ) {
		while( SDL_PollEvent(&e) != 0 ) {
			switch( e.type ) {
			case SDL_QUIT:
				run = false;
				break;
			case SDL_KEYDOWN:
				emu->c8.keypad[_keyindex(e.key.keysym.sym)] = 1;
				break;
			case SDL_KEYUP:
				emu->c8.keypad[_keyindex(e.key.keysym.sym)] = 0;
				break;
			}
		}

		clock_gettime(CLOCK_MONOTONIC, &end);
		if( _getTimeDiff(start, end).tv_nsec < 1000000 ) {
			continue;
		}

		clock_gettime(CLOCK_MONOTONIC, &start);

		c8Cycle(&emu->c8);

		if( emu->c8.dirty ) {
			_draw(emu);
		}
	}

	return EXIT_SUCCESS;
}

int emuNew(Emulator *emu) {
	emu->window = NULL;
	emu->renderer = NULL;
	emu->tex = NULL;

	if( SDL_Init(SDL_INIT_EVERYTHING) != 0 ) {
		fprintf(stderr, "ERR: Failed to initialize SDL: %s\n", SDL_GetError());
		return EXIT_FAILURE;
	}

	emu->window = SDL_CreateWindow("Chip-8 Emulator", -1, -1, WINDOW_WIDTH,
		WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
	if( emu->window == NULL ) {
		fprintf(stderr, "ERR: Failed to create window: %s\n", SDL_GetError());
		return EXIT_FAILURE;
	}

	emu->renderer = SDL_CreateRenderer(
		emu->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if( emu->renderer == NULL ) {
		fprintf(stderr, "ERR: Failed to create renderer: %s\n", SDL_GetError());
		return EXIT_FAILURE;
	}

	if( SDL_RenderSetLogicalSize(emu->renderer, SCR_WIDTH, SCR_HEIGHT) != 0 ) {
		fprintf(stderr, "ERR: Failed to set size: %s\n", SDL_GetError());
		return EXIT_FAILURE;
	}

	if( SDL_RenderSetScale(
			emu->renderer, DEFAULT_SCALE_FACTOR, DEFAULT_SCALE_FACTOR)
		!= 0 ) {
		fprintf(stderr, "ERR: Failed to set scale: %s\n", SDL_GetError());
		return EXIT_FAILURE;
	}

	SDL_SetWindowTitle(emu->window, "Chip-8 emulator");

	emu->tex = SDL_CreateTexture(emu->renderer, SDL_PIXELFORMAT_RGB332,
		SDL_TEXTUREACCESS_STREAMING, SCR_WIDTH, SCR_HEIGHT);
	if( emu->tex == NULL ) {
		fprintf(stderr, "ERR: Failed to create texture: %s\n", SDL_GetError());
		return EXIT_FAILURE;
	}

	emu->c8 = c8New();

	return EXIT_SUCCESS;
}

int emuRun(Emulator *emu, uint8_t *program, size_t size) {
	if( c8Load(&emu->c8, program, size) == EXIT_FAILURE ) {
		fprintf(stderr, "ERR: c8Load failed!\n");
		return EXIT_FAILURE;
	}

	return _run(emu);
}

int emuRunFile(Emulator *emu, const char *PATH) {
	if( c8LoadFile(&emu->c8, PATH) > 0 ) {
		fprintf(stderr, "ERR: c8LoadFile failed!\n");
		return EXIT_FAILURE;
	}

	return _run(emu);
}

void emuSetDelay(Emulator *emu, const int delayms) {
	emu->delay = delayms * 1000000;
}

int emuSetScaleFactor(Emulator *emu, const float SCALE) {
	if( SDL_RenderSetScale(emu->renderer, SCALE, SCALE) != 0 ) {
		fprintf(stderr, "ERR: Failed to set scale: %s\n", SDL_GetError());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void emuQuit(Emulator *emu) {
	if( emu->tex ) {
		SDL_DestroyTexture(emu->tex);
	}

	if( emu->renderer ) {
		SDL_DestroyRenderer(emu->renderer);
	}

	if( emu->window ) {
		SDL_DestroyWindow(emu->window);
	}

	SDL_Quit();
}
