/*
 * Copyright(c) 2021 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>  // for puts
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <time.h>

#include <string.h>
#include <stdbool.h>

typedef struct params_s {
  uint32_t max_value;
  uint32_t nibble_size;
  const char *name;
} params_t;

enum { TWO_BIT, FOUR_BIT };

params_t params[] = {
    {.max_value = (1 << 8), .nibble_size = 2, .name = "Two Bit"},

    {.max_value = (1 << 16), .nibble_size = 4, .name = "Four bit"}};

/* Count the number of trailing zeroes. */
static inline uint32_t ctz(uint32_t x) {
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
  return x ? __builtin_ctz(x) : 32;
#else
  // not implemented
  assert(0);
  return 0;
#endif
}

void print(uint32_t start,
           uint32_t end,
           uint32_t *start_values,
           uint32_t *end_values,
           uint32_t nibble_size) {
  int i;
  uint32_t range_start = 0;
  uint32_t range_end = 0;
  printf("\n%60.60s\n",
         "-------------------------------------------------------------"
         "-------------------------------------------------------------");
  for (i = 3; i >= 0; i--) {
    range_start <<= nibble_size;
    range_start |= start_values[i];
    range_end <<= nibble_size;
    range_end |= end_values[i];
  }
  printf("%5d-%-5d (0x%04x - 0x%04x)   | ",
         range_start,
         range_end,
         range_start,
         range_end);
  for (i = 3; i >= 0; i--) {
    if (start_values[i] == end_values[i]) {
      printf("%5d ", start_values[i]);
    } else {
      printf("%2d-%-2d ", start_values[i], end_values[i]);
    }
  }
  printf("\n%60.60s\n",
         "-------------------------------------------------------------"
         "-------------------------------------------------------------");

  assert(range_start >= start);
  assert(range_end <= end);
  assert(range_end >= range_start);
}

void check_range(uint32_t start,
                 uint32_t end,
                 uint32_t start_values[][4],
                 uint32_t end_values[][4],
                 uint32_t count,
                 uint32_t max_value,
                 uint32_t nibble_size) {
  bool range_covered[max_value];
  memset(range_covered, 0, sizeof(range_covered));
  int j;
  for (j = 0; j < count; j++) {
    int i;
    uint32_t range_start = 0;
    uint32_t range_end = 0;
    for (i = 3; i >= 0; i--) {
      range_start <<= nibble_size;
      range_start |= start_values[j][i];
      range_end <<= nibble_size;
      range_end |= end_values[j][i];
    }

    assert(range_start >= start);
    assert(range_end <= end);
    assert(range_end >= range_start);

    for (i = range_start; i <= range_end; i++) {
      assert(range_covered[i] == 0);
      range_covered[i] = 1;
    }
  }

  for (j = 0; j < max_value; j++) {
    if (j < start) {
      assert(range_covered[j] == 0);
    } else if (j <= end) {
      assert(range_covered[j] == 1);
    } else {
      assert(range_covered[j] == 0);
    }
  }
}

void expand(uint32_t start, uint32_t end, bool debug, int mode) {
  uint32_t max_val = params[mode].max_value;
  uint32_t nibble_size = params[mode].nibble_size;

  assert(start < max_val);
  assert(end < max_val);
  assert(start <= end);

  if (debug) {
    printf("Mode %s start %d end %d\n", params[mode].name, start, end);
  }

  uint32_t count = 0;
  uint32_t start_values[7][4];
  uint32_t end_values[7][4];

  uint32_t range_start = 0, range_end = 0;
  range_start = start;

  int i;
  int start_nibble;

  do {
    if (range_start == 0) {
      start_nibble = 3;
    } else {
      start_nibble = ctz(range_start) / nibble_size;
    }

    assert(start_nibble <= 3);
    for (i = start_nibble; i >= 0; i--) {
      range_end = range_start | ((1 << (nibble_size * (i + 1))) - 1);

      for (; (range_end >= range_start) && (range_end > end) &&
                 (range_end >= 1 << (nibble_size * i));
           range_end -= 1 << (nibble_size * i)) {
      }

      if ((range_end >= range_start) && (range_end <= end)) {
        break;
      }
    }

    for (i = 0; i < 4; i++) {
      start_values[count][i] =
          (range_start >> (i * nibble_size)) & ((1 << nibble_size) - 1);
      end_values[count][i] =
          (range_end >> (i * nibble_size)) & ((1 << nibble_size) - 1);
    }
    if (debug) {
      print(start, end, start_values[count], end_values[count], nibble_size);
    }

    range_start = range_end + 1;
    count++;
    assert(count <= 7);
  } while (range_end < end);

  /* Now do an assert for the valid range */
  check_range(
      start, end, start_values, end_values, count, max_val, nibble_size);
}

int main(int argc, char **argv) {
  uint32_t start = 18885;
  uint32_t end = 44440;
  if (argc == 3) {
    start = strtoul(argv[1], NULL, 0);
    end = strtoul(argv[2], NULL, 0);

    expand(start, end, true, FOUR_BIT);

    if ((start < params[TWO_BIT].max_value) &&
        (end < params[TWO_BIT].max_value)) {
      expand(start, end, true, TWO_BIT);
    }

  } else {
    uint32_t i, j;

    printf("TWO BIT\n");
    for (i = 0; i < params[TWO_BIT].max_value; i++) {
      for (j = i; j < params[TWO_BIT].max_value; j++) {
        expand(i, j, false, TWO_BIT);
      }
      if ((i % 100) == 0) {
        printf("%d\n", i);
      }
    }
    printf("TWO BIT passed\n");

    uint32_t seed = 0;
    if (argc == 2) {
      seed = strtoul(argv[1], NULL, 0);
    } else {
      seed = time(NULL);
    }

    printf("Seed %d\n", seed);
    srand(seed);
    for (i = 0; i < 500000; i++) {
      uint32_t start = rand() % params[FOUR_BIT].max_value;
      uint32_t end = start + (rand() % (params[FOUR_BIT].max_value - start));
      expand(start, end, false, FOUR_BIT);
      if ((i % 5000) == 0) {
        printf("%d\n", i);
      }
    }
    printf("Seed %d\n", seed);
  }
  return 0;
}
