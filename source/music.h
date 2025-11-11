#pragma once
#include <stdio.h>
#include <nds.h>
#include <maxmod9.h>

#define MUS_BUFFER_LENGTH 1024

extern FILE *song;
extern u8 mus_buf[MUS_BUFFER_LENGTH];
extern int streamIn;
extern int streamOut;

void mus_init(void); // Inits the music system
void mus_play(const char *file, mm_word rate); // Loads and begins playing a song
void mus_update(void); // Call each frame! Updates the music system
void mus_stop(void); // Stops a running song