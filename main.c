#include <stdio.h>
#include "definitions.h"

#define MIDI_IMPLEMENTATION
#include "midi.h"

#define VEL    96
#define DIV    2048
#define LEN    (DIV << 1)
#define NCHRDS 32

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
#define TRP(_0, _1, _2)\
  next.real_chord[0] = PCLS_WRAP(curr->real_chord[0] + _0);\
  next.real_chord[1] = PCLS_WRAP(curr->real_chord[1] + _1);\
  next.real_chord[2] = PCLS_WRAP(curr->real_chord[2] + _2)

  ChordState next;

  fprintf(stderr, "tag: %d\n", curr->tag);
  switch (curr->tag)
  {
    case TAG_MAJOR:
    {
      int case_ = RRANGE(0, 9);
      fprintf(stderr, "major, case %d\n", case_);
      switch (case_)
      {
        case 0:
          // Transpose major chord a whole step down
          TRP(-2, -2, -2);
          next.tag = TAG_MAJOR;
          break;
        case 1:
          // Transpose major chord a whole step down, make minor
          TRP(-2, -3, -2);
          next.tag = TAG_MINOR;
          break;
        case 2:
          // Transpose a minor third up
          TRP(+3, +3, +3);
          next.tag = TAG_MAJOR;
          break;
        case 3:
          // Transpose a major third up, make minor
          TRP(+4, +3, +4);
          next.tag = TAG_MINOR;
          break;
        case 4:
          // Transpose to dominant, suspended
          TRP(-5, -4, -5);
          next.tag = TAG_SUSPENDED;
          break;
        case 5:
          // Transpose a whole step up, suspended
          TRP(+2, +3, +2);
          next.tag = TAG_SUSPENDED;
          break;
        case 6:
          // Make suspended
          TRP(+0, +1, +0);
          next.tag = TAG_SUSPENDED;
          break;
        case 7:
          // Make dominant without one
          TRP(+4, +3, +3);
          next.tag = TAG_DIMINISHED;
          break;
        case 8:
          // To dominant with low nine
          TRP(+5, +4, +4);
          next.tag = TAG_DIMINISHED;
          break;
        default:
          PANIC("case out of range");
      }
      break;
    }
    case TAG_MINOR:
    {
      int case_ = RRANGE(0, 9);
      fprintf(stderr, "minor, case %d\n", case_);
      switch (case_)
      {
        case 0:
          // Transpose minor chord a major third down
          TRP(-4, -4, -4);
          next.tag = TAG_MINOR;
          break;
        case 1:
          // Transpose minor chord a fourth, make major
          TRP(+5, +6, +5);
          next.tag = TAG_MAJOR;
          break;
        case 2:
          // Transpose a fourth down
          TRP(-5, -5, -5);
          next.tag = TAG_MINOR;
          break;
        case 3:
          // Transpose a whole step down, make major
          TRP(-2, -1, -2);
          next.tag = TAG_MAJOR;
          break;
        case 4:
          // Transpose to dominant, suspended
          TRP(-5, -3, -5);
          next.tag = TAG_SUSPENDED;
          break;
        case 5:
          // Transpose to subdominant, suspended
          TRP(+5, +7, +5);
          next.tag = TAG_SUSPENDED;
          break;
        case 6:
          // Make suspended
          TRP(+0, +2, +0);
          next.tag = TAG_SUSPENDED;
          break;
        case 7:
          // Lower fifth
          TRP(+0, +0, -1);
          next.tag = TAG_DIMINISHED;
          break;
        case 8:
          // Add high sixth
          TRP(-3, -3, -4);
          next.tag = TAG_DIMINISHED;
          break;
        default:
          PANIC("case out of range");
      }
      break;
    }
    case TAG_SUSPENDED:
    {
      int case_ = RRANGE(0, 4);
      fprintf(stderr, "suspended, case %d\n", case_);
      switch (case_)
      {
        case 0:
          // Release, fourth goes to major third
          TRP(+0, -1, +0);
          next.tag = TAG_MAJOR;
          break;
        case 1:
          // Release, fourth goes to minor third
          TRP(+0, -2, +0);
          next.tag = TAG_MINOR;
          break;
        case 2:
          // Release, fifth goes to major third
          TRP(+5, +4, +5);
          next.tag = TAG_MAJOR;
          break;
        case 3:
          // Release, fifth goes to minor third
          TRP(+5, +3, +5);
          next.tag = TAG_MINOR;
          break;
        default:
          PANIC("case out of range");
      }
      break;
    }
    case TAG_DIMINISHED:
    {
      int case_ = RRANGE(0, 4);
      fprintf(stderr, "diminished, case %d\n", case_);
      switch (case_)
      {
        case 0:
          // Make major on same base note
          TRP(+0, +1, +1);
          next.tag = TAG_MAJOR;
          break;
        case 1:
          // Treat as D7<9 resolve to minor
          TRP(-5, -5, -4);
          next.tag = TAG_MINOR;
          break;
        case 2:
          // Treat as D7 resolve to major
          TRP(+1, +2, +2);
          next.tag = TAG_MAJOR;
          break;
        case 3:
          // Resolve low one down
          TRP(-1, +0, +0);
          next.tag = TAG_MAJOR;
          break;
        default:
          PANIC("case out of range");
      }
      break;
    }
    default:
      PANIC("tag out of range");
  }

  // Make the chord "travel the least distance"
  int perm = lsd(curr->real_chord, next.real_chord);
  fprintf(stderr, "optimal permutation: %d\n", perm);
  permute(next.chord, next.real_chord, perm);
  chst_copy(curr, &next);
}
