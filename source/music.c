#include "music.h"

/*
  Heavily based off https://github.com/blocksds/sdk/blob/master/examples/maxmod/streaming/source/main.c
*/

static mm_stream mus_stream;
static bool mus_playing = false;

FILE *song;
u8 mus_buf[MUS_BUFFER_LENGTH];
int streamIn;
int streamOut;

// Streaming callback (runs in IRQ)
static mm_word streamingCallback(mm_word length, mm_addr dest, mm_stream_formats format) {
    size_t size = length; // 8-bit mono → 1 byte per sample
    size_t bytesUntilEnd = MUS_BUFFER_LENGTH - streamOut;

    if (bytesUntilEnd >= size) {
        memcpy(dest, &mus_buf[streamOut], size);
        streamOut += size;
    } else {
        memcpy(dest, &mus_buf[streamOut], bytesUntilEnd);
        memcpy(dest + bytesUntilEnd, &mus_buf[0], size - bytesUntilEnd);
        streamOut = size - bytesUntilEnd;
    }

    return length;
}

// Internal function to refill buffer from file
static void refillBuffer(bool force) {
    if (!force && streamIn == streamOut) return;

    size_t remaining = (streamOut > streamIn) ? (streamOut - streamIn) : (MUS_BUFFER_LENGTH - streamIn);
    size_t readSize = remaining;

    size_t bytesRead = fread(&mus_buf[streamIn], 1, readSize, song);
    streamIn += bytesRead;

    if (feof(song)) {
        fseek(song, 0, SEEK_SET);  // loop
    }

    if (streamIn >= MUS_BUFFER_LENGTH) streamIn -= MUS_BUFFER_LENGTH;
}

// Public functions

void mus_init(void) {
    mm_ds_system mmSys = { .mod_count = 0, .samp_count = 0, .mem_bank = 0, .fifo_channel = FIFO_MAXMOD };
    mmInit(&mmSys);
    streamIn = 0;
    streamOut = 0;
    mus_playing = false;
    song = NULL;
}

void mus_play(const char *file, mm_word rate) {
    if (mus_playing) mus_stop();

    song = fopen(file, "rb");
    if (!song) return;

    // Fill initial buffer
    refillBuffer(true);

    mus_stream = (mm_stream){
        .sampling_rate = rate,
        .buffer_length = MUS_BUFFER_LENGTH,
        .callback      = streamingCallback,
        .format        = MM_STREAM_8BIT_MONO,
        .timer         = MM_TIMER0,
        .manual        = false
    };

    mmStreamOpen(&mus_stream);
    mus_playing = true;
}

void mus_update(void) {
    if (!mus_playing) return;
    refillBuffer(false);
}

void mus_stop(void) {
    if (!mus_playing) return;
    mmStreamClose();
    if (song) fclose(song);
    song = NULL;
    mus_playing = false;
}
