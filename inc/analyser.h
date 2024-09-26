#ifndef GUARD_PROGRAM_DECOMPILE_ANALYSER_H_
#define GUARD_PROGRAM_DECOMPILE_ANALYSER_H_

#include <stddef.h>
#include <stdint.h>

typedef struct _Vec16 {
	size_t size;
	uint16_t list[64];
} Vec16;

typedef struct _Analyser {
	/* Code to analyse */
	uint8_t *buffer;
	size_t size;

	/* Stores the addresses of subroutines
	 * Values are stored in (origin address, target address) pairs
	 */
	Vec16 subroutines;

	/* Stores all non-subroutine jumps
	 * Also stored in pairs
	 */
	Vec16 jumps;

	/* Stores addresses of skips
	 * Not stored in pairs, since we can easily get the target from the origin
	 */
	Vec16 skips;
} Analyser;

Analyser anlInit(uint8_t *buffer, size_t size);

void anlAnalyse(Analyser *anl);

#endif // !GUARD_PROGRAM_DECOMPILE_ANALYSER_H_
