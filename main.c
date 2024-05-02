#include <stdio.h>
#include "definitions.h"

#define MIDI_IMPLEMENTATION
#include "midi.h"

#define VEL    96
#define DIV    2048
#define LEN    (DIV << 1)
#define NCHRDS 24

const uint8_t base_oct = 3;
const uint8_t oct[3] = { 4, 5, 6 };

#define PITCH(oct, cls) (12 * (oct) + (cls))

void play_chord(ChordState *chd, midi_track_t *trk, midi_track_t *base, int len)
{
  midi_message_t msg;

  msg = midi_message_note_on(0, PITCH(base_oct, chd->chord[0]), VEL);
  midi_track_add_midi_message(base, 0, msg);
    for (size_t i = 0; i < 3; i++)
  {
    msg = midi_message_note_on(0, PITCH(oct[i], chd->chord[i]), VEL);
    midi_track_add_midi_message(trk, 0, msg);
  }

  msg = midi_message_note_off(0, PITCH(base_oct, chd->chord[0]), VEL);
  midi_track_add_midi_message(base, len, msg);
    for (size_t i = 0; i < 3; i++)
  {
    msg = midi_message_note_off(0, PITCH(oct[i], chd->chord[i]), VEL);
    midi_track_add_midi_message(trk, i ? 0 : len, msg);
  }
}

void pick_next_chord(ChordState *current);

int main(int argc, char *argv[])
{
  (void) argc;
  (void) argv;

  srand(0);

  // Create Midi context
  int div = midi_division_ticks_per_quarter_note(DIV);
  int fmt = MIDI_FORMAT_SIMULTANEOUS;
  midi_t mid = midi_create(fmt, div);

  // Create tracks
  midi_track_t *main_trk = midi_track_create();
  midi_track_t *base_trk = midi_track_create();

  // Create C major chord
  ChordState curr, prev;
  curr.tag = TAG_MAJOR;
  curr.chord[0] = PCLS_C;
  curr.chord[1] = PCLS_E;
  curr.chord[2] = PCLS_G;
  copy(curr.real_chord, curr.chord);

  // Build tracks
  for (size_t i = 0; i < NCHRDS; i++)
  {
    print_chordstate(&curr, stderr);
    play_chord(&curr, main_trk, base_trk, LEN);
    pick_next_chord(&curr);
    chst_copy(&prev, &curr);
  }

  // Finish and output Midi
  midi_track_add_end_of_track_event(main_trk, 0);
  midi_track_add_end_of_track_event(base_trk, 0);
  midi_add_track(&mid, main_trk);
  midi_add_track(&mid, base_trk);
  midi_write(&mid, stdout);
  midi_destroy(&mid);
}

void pick_next_chord(ChordState *curr)
{
  ChordState next;

  switch (curr->tag)
  {
    case TAG_MAJOR:
    {
      int case_ = RRANGE(0, 4);
      fprintf(stderr, "minor, case %d\n", case_);
      switch (case_)
      {
        case 0:
          // Transpose major chord a whole step down
          next.real_chord[0] = PCLS_WRAP(curr->real_chord[0] - 2);
          next.real_chord[1] = PCLS_WRAP(curr->real_chord[1] - 2);
          next.real_chord[2] = PCLS_WRAP(curr->real_chord[2] - 2);
          next.tag = TAG_MAJOR;
          // ===================================================
          break;
        case 1:
          // Transpose major chord a whole step down, make minor
          next.real_chord[0] = PCLS_WRAP(curr->real_chord[0] - 2);
          next.real_chord[1] = PCLS_WRAP(curr->real_chord[1] - 3);
          next.real_chord[2] = PCLS_WRAP(curr->real_chord[2] - 2);
          next.tag = TAG_MINOR;
          // ===================================================
        case 2:
          // Transpose a minor third up
          next.real_chord[0] = PCLS_WRAP(curr->real_chord[0] + 3);
          next.real_chord[1] = PCLS_WRAP(curr->real_chord[1] + 3);
          next.real_chord[2] = PCLS_WRAP(curr->real_chord[2] + 3);
          next.tag = TAG_MAJOR;
          // ===================================================
          break;
        case 3:
          // Transpose a major third up, make minor
          next.real_chord[0] = PCLS_WRAP(curr->real_chord[0] + 4);
          next.real_chord[1] = PCLS_WRAP(curr->real_chord[1] + 3);
          next.real_chord[2] = PCLS_WRAP(curr->real_chord[2] + 4);
          next.tag = TAG_MINOR;
          // ===================================================
          break;
        default:
          PANIC();
      }
      break;
    }
    case TAG_MINOR:
    {
      int case_ = RRANGE(0, 4);
      fprintf(stderr, "minor, case %d\n", case_);
      switch (case_)
      {
        case 0:
          // Transpose minor chord a major third down
          next.real_chord[0] = PCLS_WRAP(curr->real_chord[0] - 4);
          next.real_chord[1] = PCLS_WRAP(curr->real_chord[1] - 4);
          next.real_chord[2] = PCLS_WRAP(curr->real_chord[2] - 4);
          next.tag = TAG_MINOR;
          // ===================================================
          break;
        case 1:
          // Transpose minor chord a fourth, make major
          next.real_chord[0] = PCLS_WRAP(curr->real_chord[0] + 5);
          next.real_chord[1] = PCLS_WRAP(curr->real_chord[1] + 6);
          next.real_chord[2] = PCLS_WRAP(curr->real_chord[2] + 5);
          next.tag = TAG_MAJOR;
          // ===================================================
        case 2:
          // Transpose a fourth down
          next.real_chord[0] = PCLS_WRAP(curr->real_chord[0] - 4);
          next.real_chord[1] = PCLS_WRAP(curr->real_chord[1] - 4);
          next.real_chord[2] = PCLS_WRAP(curr->real_chord[2] - 4);
          next.tag = TAG_MINOR;
          // ===================================================
          break;
        case 3:
          // Transpose a whole step down, make major
          next.real_chord[0] = PCLS_WRAP(curr->real_chord[0] - 2);
          next.real_chord[1] = PCLS_WRAP(curr->real_chord[1] - 1);
          next.real_chord[2] = PCLS_WRAP(curr->real_chord[2] - 2);
          next.tag = TAG_MAJOR;
          // ===================================================
          break;
        default:
          PANIC();
      }
    }
    break;
  }

  // Make the chord "travel the least distance"
  int perm = lsd(curr->real_chord, next.real_chord);
  permute(next.chord, next.real_chord, perm);
  chst_copy(curr, &next);
}
