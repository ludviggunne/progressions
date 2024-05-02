/*
 * midi.h - single header C library for creating midi files
 * Copyright (C) 2024 Ludvig Gunne Lindström
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
*/

#ifndef MIDI_H
#define MIDI_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MIDI_ERROR -1
#define MIDI_OK     0

#define MIDI_MESSAGE_NOTE_OFF_EVENT      8
#define MIDI_MESSAGE_NOTE_ON_EVENT       9
#define MIDI_MESSAGE_PITCH_WHEEL_CHANGE 14
/* TODO: More messages */

#define MIDI_PITCH_WHEEL_CENTRE 0x2000

#define MIDI_TEXT_TEXT_EVENT             1
#define MIDI_TEXT_COPYRIGHT_NOTICE       2
#define MIDI_TEXT_SEQUENCE_OR_TRACK_NAME 3
#define MIDI_TEXT_INSTRUMENT_NAME        4
#define MIDI_TEXT_LYRIC                  5
#define MIDI_TEXT_MARKER                 6
#define MIDI_TEXT_CUE_POINT              7

#define MIDI_FORMAT_SINGLE       0
#define MIDI_FORMAT_SIMULTANEOUS 1
#define MIDI_FORMAT_INDEPENDENT  2

typedef struct {
    uint8_t status;
    uint8_t data[2];
} midi_message_t;

typedef struct midi_track midi_track_t;

typedef struct {
    uint16_t format;
    uint16_t ntrks;
    uint16_t division;
    midi_track_t *tracks;
} midi_t;

midi_message_t midi_message_note_on(int channel, int key, int velocity);
midi_message_t midi_message_note_off(int channel, int key, int velocity);
midi_message_t midi_message_pitch_wheel_change(int channel, int value);

uint16_t midi_division_ticks_per_quarter_note(uint16_t ticks);

midi_track_t *midi_track_create(void);
void midi_track_destroy(midi_track_t * track);
int midi_track_add_midi_message(midi_track_t * track, uint32_t dt,
                                midi_message_t msg);
int midi_track_add_end_of_track_event(midi_track_t * track, uint32_t dt);
int midi_track_add_meta_event_text(midi_track_t * track, uint32_t dt,
                                   uint8_t kind, const char *text);

midi_t midi_create(uint16_t format, uint16_t division);
void midi_destroy(midi_t * midi);
void midi_add_track(midi_t * midi, midi_track_t * track);
int midi_write(midi_t * midi, FILE * f);

#endif                          /* MIDI_H */

#ifdef MIDI_IMPLEMENTATION
#undef MIDI_IMPLEMENTATION

#define MIDI_TRACK_INITIAL_CAPACITY 16

struct midi_track {
    struct midi_track *next;
    uint8_t *data;
    size_t size;
    size_t cap;
};

static int _fwrite_u32_be(FILE *f, uint32_t value)
{
    uint8_t buf[4];
    buf[0] = (value >> 24) & 0xff;
    buf[1] = (value >> 16) & 0xff;
    buf[2] = (value >> 8) & 0xff;
    buf[3] = value & 0xff;

    if (fwrite(buf, 1, 4, f) < 4) {
        return MIDI_ERROR;
    }

    return MIDI_OK;
}

static int _fwrite_u16_be(FILE *f, uint32_t value)
{
    uint8_t buf[2];
    buf[0] = (value >> 8) & 0xff;
    buf[1] = value & 0xff;

    if (fwrite(buf, 1, 2, f) < 2) {
        return MIDI_ERROR;
    }

    return MIDI_OK;
}

midi_message_t midi_message_note_on(int channel, int key, int velocity)
{
    midi_message_t msg = { 0 };
    msg.status = channel & 0xf;
    msg.status |= MIDI_MESSAGE_NOTE_ON_EVENT << 4;
    msg.data[0] = key & 0x7f;
    msg.data[1] = velocity & 0x7f;
    return msg;
}

midi_message_t midi_message_note_off(int channel, int key, int velocity)
{
    midi_message_t msg = { 0 };
    msg.status = channel & 0xf;
    msg.status |= MIDI_MESSAGE_NOTE_OFF_EVENT << 4;
    msg.data[0] = key & 0x7f;
    msg.data[1] = velocity & 0x7f;
    return msg;
}

midi_message_t midi_message_pitch_wheel_change(int channel, int value)
{
    midi_message_t msg = { 0 };
    msg.status = channel & 0xf;
    msg.status |= MIDI_MESSAGE_PITCH_WHEEL_CHANGE << 4;
    msg.data[0] = value & 0x7f;
    msg.data[1] = (value >> 7) & 0x7f;
    return msg;
}

uint16_t midi_division_ticks_per_quarter_note(uint16_t ticks)
{
    return ticks & 0x7fff;
}

midi_track_t *midi_track_create(void)
{
    midi_track_t *track = malloc(sizeof(*track));
    if (track == NULL) {
        return NULL;
    }

    track->next = NULL;
    track->cap = MIDI_TRACK_INITIAL_CAPACITY;
    track->data = malloc(track->cap);
    track->size = 0;

    if (track->data == NULL) {
        free(track);
        return NULL;
    }

    return track;
}

void midi_track_destroy(midi_track_t *track)
{
    free(track->data);
    free(track);
}

static uint8_t *_midi_track_alloc(midi_track_t *track, size_t size)
{
    if (track->size + size > track->cap) {
        size_t cap = track->cap * 2;
        while (track->size + size > cap) {
            cap *= 2;
        }
        uint8_t *data = realloc(track->data, sizeof(*data) * cap);
        if (data == NULL) {
            return NULL;
        }
        track->data = data;
        track->cap = cap;
    }

    uint8_t *ptr = &track->data[track->size];
    track->size += size;
    return ptr;
}

static int _midi_track_write_vlq(midi_track_t *track, uint32_t x)
{
    uint8_t buf[4];
    size_t size = 1;

    buf[3] = x & 0x7f;
    x >>= 7;

    while (x) {
        buf[3 - size] = (x & 0x7f) | 0x80;
        x >>= 7;
        size++;
    }

    uint8_t *ptr = _midi_track_alloc(track, size);
    if (ptr == NULL) {
        return MIDI_ERROR;
    }

    memcpy(ptr, &buf[4 - size], size);
    return MIDI_OK;
}

static int _midi_track_write_to_file(midi_track_t *track, FILE *f)
{
    if (fwrite("MTrk", 1, 4, f) < 4) {
        return MIDI_ERROR;
    }

    if (_fwrite_u32_be(f, track->size)) {
        return MIDI_ERROR;
    }

    if (fwrite(track->data, 1, track->size, f) < track->size) {
        return MIDI_ERROR;
    }

    /* TODO: Should there be padding? */

    return MIDI_OK;
}

int midi_track_add_midi_message(midi_track_t *track, uint32_t dt,
                                midi_message_t msg)
{
    if (_midi_track_write_vlq(track, dt)) {
        return MIDI_ERROR;
    }

    uint8_t *ptr = _midi_track_alloc(track, 3);
    if (ptr == NULL) {
        return MIDI_ERROR;
    }

    ptr[0] = msg.status;
    ptr[1] = msg.data[0];
    ptr[2] = msg.data[1];

    return MIDI_OK;
}

int midi_track_add_end_of_track_event(midi_track_t *track, uint32_t dt)
{
    const uint8_t event[] = { 0xff, 0x2f, 0x00 };
    const size_t event_size = sizeof(event) / sizeof(*event);

    if (_midi_track_write_vlq(track, dt)) {
        return MIDI_ERROR;
    }

    uint8_t *ptr = _midi_track_alloc(track, event_size);
    if (ptr == NULL) {
        return MIDI_ERROR;
    }

    memcpy(ptr, event, event_size);

    return MIDI_OK;
}

int midi_track_add_meta_event_text(midi_track_t *track, uint32_t dt,
                                   uint8_t kind, const char *text)
{
    if (_midi_track_write_vlq(track, dt)) {
        return MIDI_ERROR;
    }

    uint8_t *hdr = _midi_track_alloc(track, 2);
    if (hdr == NULL) {
        return MIDI_ERROR;
    }

    hdr[0] = 0xff;
    hdr[1] = kind;

    uint32_t len = 0x0fffffff & strlen(text);

    if (_midi_track_write_vlq(track, len)) {
        return MIDI_ERROR;
    }

    char *ptr = (char *) _midi_track_alloc(track, len);
    if (ptr == NULL) {
        return MIDI_ERROR;
    }

    memcpy(ptr, text, len);
    return MIDI_OK;
}

midi_t midi_create(uint16_t format, uint16_t division)
{
    midi_t f = { 0 };
    f.format = format;
    f.division = division;
    f.ntrks = 0;
    f.tracks = NULL;
    return f;
}

void midi_destroy(midi_t *midi)
{
    midi_track_t *track = midi->tracks;
    while (track) {
        midi_track_t *tmp = track;
        track = track->next;
        midi_track_destroy(tmp);
    }
    midi->tracks = NULL;
}

void midi_add_track(midi_t *midi, midi_track_t *track)
{
    midi_track_t *ptr = midi->tracks;

    if (ptr == NULL) {
        midi->tracks = track;
        midi->ntrks++;
        return;
    }

    while (ptr->next) {
        ptr = ptr->next;
    }

    ptr->next = track;
    midi->ntrks++;
}

int midi_write(midi_t *midi, FILE *f)
{
    if (fwrite("MThd", 1, 4, f) < 4) {
        return MIDI_ERROR;
    }

    if (_fwrite_u32_be(f, 6)) {
        return MIDI_ERROR;
    }

    if (_fwrite_u16_be(f, midi->format)) {
        return MIDI_ERROR;
    }

    if (_fwrite_u16_be(f, midi->ntrks)) {
        return MIDI_ERROR;
    }

    if (_fwrite_u16_be(f, midi->division)) {
        return MIDI_ERROR;
    }

    midi_track_t *track = midi->tracks;

    while (track) {
        if (_midi_track_write_to_file(track, f)) {
            return MIDI_ERROR;
        }
        track = track->next;
    }

    return MIDI_OK;
}

#endif                          /* MIDI_IMPLEMENTATION */
