# chip8: Not Another Chip-8 Emulator, Man!

Yup, it's another one. Except that this one:
- Runs Chip-8 programs;
- Decompiles Chip-8 programs;
- Compiles assembly into Chip-8 programs (not yet... I'm working on it);

## Features
### `chip8 run`
This program runs Chip-8 programs. It is fairly functional, though it still has
some [problems](#problems).

Use `chip8 run help` to get usage info.

### `chip8 decompile`
This program does a best-effort decompilation of Chip-8 programs. It scans the
bytes and outputs them as assembly instructions. A verbose mode can also output
plain English instructions.

Use `chip8 decompile help` to get usage info.

### `chip8 compile`
Doesn't actually do anything yet... :(

## Problems
- Keypad doesn't work??? It looks fine, but only the 4 key does something. I'm
  looking into it;
- The decompiler is very bare-bones. Will try to add some fancy stuff, like
  subroutine labels. That will require a two-pass scanner;

## TODO list

### Runner
- [ ] Fix keypad;
- [ ] Add step-by-step execution;
- [ ] Fully-fledged debugger (breakpoints, step-in, etc.)

### Decompiler
- [x] Start work on analyser;
- [ ] Add basic subroutine labels;
- [ ] Add jump arrows (or do labels again?);
- [ ] Add skip arrows;

### Compiler
- [ ] Start work on;
