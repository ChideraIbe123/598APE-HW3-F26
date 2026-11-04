#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static void die(const char *s) {
  fprintf(stderr, "%s\n", s);
  exit(1);
}

int N = 801;
int **grid;

// you can also use this function to debug print your sandpile
static void render(char *filename) {
  int chan = 3;
  unsigned char *pixels = (unsigned char *)malloc(N * N * chan * sizeof(char));
  int stride = N * chan;
  if (!pixels)
    die("malloc failed for PNG pixel buffer");
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      unsigned char r = 0, g = 0, b = 0;
      switch (grid[i][j]) {
      case 0: // obsidian
        r = 15;
        g = 15;
        b = 25;
        break;
      case 1: // electric cyan
        r = 0;
        g = 210;
        b = 255;
        break;
      case 2: // vibrant magenta
        r = 230;
        g = 30;
        b = 130;
        break;
      case 3: // bright gold
        r = 255;
        g = 200;
        b = 0;
        break;
      default: // white
        r = 0;
        g = 0;
        b = 0;
      }

      int pid = (i * N + j) * chan;
      pixels[pid] = r;
      pixels[pid + 1] = g;
      pixels[pid + 2] = b;
    }
  }
  if (stbi_write_png(filename, N, N, chan, pixels, stride)) {
    printf("Successfully saved sandpile identity to: %s\n", filename);
  } else {
    die("Error: stb_image_write failed to save png file\n");
  }
  free(pixels);
}

static void shape_rows(int *lo, int *hi) {
  long long c = N / 2;
  for (int i = 0; i < N; ++i) {
    long long d = c * c - (i - c) * (i - c);
    lo[i] = 0;
    hi[i] = -1;
    if (d < 0)
      continue;
    long long r = (long long)sqrt((double)d);
    while (r * r > d)
      r--;
    while ((r + 1) * (r + 1) <= d)
      r++;
    lo[i] = (int)(c - r);
    hi[i] = (int)MIN(MIN(c + r, (long long)MAX(i, N - 1 - i)), N - 1);
  }
}

static int topple_row(int *row, int *up, int *dn, int a, int b, int dm) {
  if (a > b)
    return 0;
  int changed = 0;
  int left = row[a - 1];
  int cur = row[a];
  for (int j = a; j <= b; ++j) {
    int t = cur >> 2;
    changed = changed | t;
    up[j] = up[j] + t;
    dn[j] = dn[j] + dm * t;
    row[j - 1] = left + t;
    left = cur & 3;
    cur = row[j + 1] + t;
  }
  row[b] = left;
  row[b + 1] = cur;
  return changed;
}

static int topple_pair(int *top, int *bot, int *up, int *dn, int start,
                       int end) {
  if (start > end)
    return 0;
  int changed = 0;
  int top_left = top[start - 1];
  int top_cur = top[start];
  int bot_left = bot[start - 1];
  int bot_cur = bot[start];
  for (int j = start; j <= end; ++j) {
    int top_t = top_cur >> 2;
    bot_cur = bot_cur + top_t;
    int bot_t = bot_cur >> 2;
    up[j] = up[j] + top_t;
    dn[j] = dn[j] + bot_t;
    top[j - 1] = top_left + top_t;
    bot[j - 1] = bot_left + bot_t;
    top_left = (top_cur & 3) + bot_t;
    bot_left = bot_cur & 3;
    top_cur = top[j + 1] + top_t;
    bot_cur = bot[j + 1] + bot_t;
    changed = changed | top_t | bot_t;
  }
  top[end] = top_left;
  bot[end] = bot_left;
  top[end + 1] = top_cur;
  bot[end + 1] = bot_cur;
  return changed;
}

static void stabilize() {
  int *lo = (int *)malloc(N * sizeof(int));
  int *hi = (int *)malloc(N * sizeof(int));
  if (!lo || !hi)
    die("malloc failed for shape rows");
  shape_rows(lo, hi);

  const int W = N + 2;
  int *g = (int *)calloc((size_t)W * W, sizeof(int));
  if (!g)
    die("malloc failed for padded grid");
  for (int i = 0; i < N; ++i)
    memcpy(g + (size_t)(i + 1) * W + 1, grid[i], N * sizeof(int));

  const int c = N / 2;
  const int rows = N % 2 ? c + 1 : N;
  int *junk = (int *)calloc(W, sizeof(int));
  if (!junk)
    die("malloc failed for junk row");

  int changed = 0;
  do {
    changed = 0;
    for (int i = 0; i < rows; ++i) {
      int *row = g + (size_t)(i + 1) * W + 1;
      int *up = row - W;
      int *dn = row + W;
      int a = lo[i];
      int b = hi[i];

      int use_pair = i + 1 < rows;
      if (N % 2 && i + 1 >= c - 1)
        use_pair = 0;

      if (use_pair) {
        int *bot = row + W;
        int *bot_dn = bot + W;
        int a2 = lo[i + 1];
        int b2 = hi[i + 1];
        int start = MAX(a, a2);
        int end = MIN(b, b2);
        if (start <= end) {
          changed |= topple_row(row, up, bot, a, start - 1, 1);
          changed |= topple_row(bot, row, bot_dn, a2, start - 1, 1);
          changed |= topple_pair(row, bot, up, bot_dn, start, end);
          changed |= topple_row(row, up, bot, end + 1, b, 1);
          changed |= topple_row(bot, row, bot_dn, end + 1, b2, 1);
        } else {
          changed |= topple_row(row, up, bot, a, b, 1);
          changed |= topple_row(bot, row, bot_dn, a2, b2, 1);
        }
        i = i + 1;
        continue;
      }

      int dm = 1;
      if (N % 2 && i == c - 1)
        dm = 2;
      if (N % 2 && i == c)
        dn = junk + 1;
      changed |= topple_row(row, up, dn, a, b, dm);
    }
  } while (changed);

  for (int i = 0; i < rows; ++i)
    memcpy(grid[i], g + (size_t)(i + 1) * W + 1, N * sizeof(int));
  for (int i = rows; i < N; ++i)
    memcpy(grid[i], grid[N - 1 - i], N * sizeof(int));
  free(junk);
  free(g);
  free(lo);
  free(hi);
}  


void find_identity() {
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      grid[i][j] = 6;
    }
  }
  stabilize();
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      grid[i][j] = 6 - grid[i][j];
    }
  }
  stabilize();
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 2) {
    fprintf(stderr, "Usage: %s N\n", argv[0]);
    return EXIT_FAILURE;
  }
  char *end;
  uintmax_t parsed = strtoumax(argv[1], &end, 10);
  if (errno || argv[1][0] < '0' || argv[1][0] > '9' || *end || !parsed ||
      parsed > INT32_MAX)
    die("grid size must be a positive int");
  N = (int)parsed;

  grid = (int **)malloc(N * sizeof(int *));
  for (int i = 0; i < N; ++i) {
    grid[i] = (int *)malloc(N * sizeof(int));
  }

  find_identity();

  render("identity.png");
  for (int i = 0; i < N; ++i) {
    free(grid[i]);
  }
  free(grid);
  return EXIT_SUCCESS;
}
