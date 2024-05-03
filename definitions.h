#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <stdint.h>
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>

#define PANIC(msg) assert(0 && msg)
#define ABS(x) ((x) < 0 ? -(x) : (x))
// Random int in range [min, max)
#define RRANGE(min, max) ((min) + (rand() % ((max - min))))

typedef uint8_t PitchClass;

#define PCLS_WRAP(x) ((x + 12) % 12)

// Harmony tags
typedef enum {
  TAG_MAJOR,
  TAG_MINOR,
  // ...  
} HarmTag;

// Pitchclasses
#define PCLS_C  ((PitchClass) 0)
#define PCLS_CS ((PitchClass) 1)
#define PCLS_DB ((PitchClass) 1)
#define PCLS_D  ((PitchClass) 2)
#define PCLS_DS ((PitchClass) 3)
#define PCLS_EB ((PitchClass) 3)
#define PCLS_E  ((PitchClass) 4)
#define PCLS_ES ((PitchClass) 5)
#define PCLS_FB ((PitchClass) 4)
#define PCLS_F  ((PitchClass) 5)
#define PCLS_FS ((PitchClass) 6)
#define PCLS_GB ((PitchClass) 6)
#define PCLS_G  ((PitchClass) 7)
#define PCLS_GS ((PitchClass) 8)
#define PCLS_AB ((PitchClass) 8)
#define PCLS_A  ((PitchClass) 9)
#define PCLS_AS ((PitchClass) 10)
#define PCLS_BB ((PitchClass) 10)
#define PCLS_B  ((PitchClass) 11)
#define PCLS_BS ((PitchClass) 12)
#define PCLS_CB ((PitchClass) 11)

char pcls_buf[32];
const char *pcls_str(PitchClass pc)
{
  switch (pc) {
    case PCLS_C: return "C";
    case PCLS_DB: return "DB";
    case PCLS_D: return "D";
    case PCLS_EB: return "EB";
    case PCLS_E: return "E";
    case PCLS_F: return "F";
    case PCLS_GB: return "GB";
    case PCLS_G: return "G";
    case PCLS_AB: return "AB";
    case PCLS_A: return "A";
    case PCLS_BB: return "BB";
    case PCLS_B: return "B";
    default:
      sprintf(pcls_buf, "<err: %d>", pc);
      return pcls_buf;
  }
}

// Chord state
typedef struct {
  HarmTag tag;
  PitchClass chord[3];
  PitchClass real_chord[3];
  // int meta;
} ChordState;

void print_chordstate(ChordState *const chst, FILE *f)
{
  fprintf(f, "%s %s %s (%s %s %s)\n",
          pcls_str(chst->chord[0]),
          pcls_str(chst->chord[1]),
          pcls_str(chst->chord[2]),
          pcls_str(chst->real_chord[0]),
          pcls_str(chst->real_chord[1]),
          pcls_str(chst->real_chord[2]));
}

void chst_copy(ChordState *dst, ChordState *const src)
{
  memcpy(dst, src, sizeof (ChordState));
}

// Permutations
typedef enum {
  PERM_ABC,
  PERM_ACB,
  PERM_BAC,
  PERM_BCA,
  PERM_CAB,
  PERM_CBA,
  PERM_COUNT,
} Permutation;

// Write the permutation `perm` of `src` to `dst`
static inline void permute(PitchClass *dst, PitchClass *const src, int perm)
{
  switch (perm)
  {
    case PERM_ABC:
      dst[0] = src[0];
      dst[1] = src[1];
      dst[2] = src[2];
      return;
    case PERM_ACB:
      dst[0] = src[0];
      dst[1] = src[2];
      dst[2] = src[1];
      return;
    case PERM_BAC:
      dst[0] = src[1];
      dst[1] = src[0];
      dst[2] = src[2];
      return;
    case PERM_BCA:
      dst[0] = src[1];
      dst[1] = src[2];
      dst[2] = src[0];
      return;
    case PERM_CAB:
      dst[0] = src[2];
      dst[1] = src[0];
      dst[2] = src[1];
      return;
    case PERM_CBA:
      dst[0] = src[2];
      dst[1] = src[1];
      dst[2] = src[0];
      return;
    default:
      assert(0 && "illegal permutation");
  }
}

// Copy a three note chord from `src` to `dst`
static inline void copy(PitchClass *dst, PitchClass *const src)
{
  permute(dst, src, PERM_ABC);
}

// Computes the permutation with least square difference between two chords
static inline int lsd(PitchClass *const from, PitchClass *const to)
{
  int min = INT_MAX;
  int min_perm = 0;

  PitchClass buf[3];

  for (int perm = 0; perm < PERM_COUNT; perm++)
  {
    permute(buf, to, perm);

    int d0 = ABS(buf[0] - from[0]);
    int d1 = ABS(buf[1] - from[1]);
    int d2 = ABS(buf[2] - from[2]);

    int diff = d0 * d0 + d1 * d1 + d2 * d2;

    if (diff < min)
    {
      min = diff;
      min_perm = perm;
    }
  }

  return min_perm;
}

#endif
