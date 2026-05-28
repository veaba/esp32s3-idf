#include "qrcode.h"
#include "esp_task_wdt.h"
#include "spilcd.h"
#include <stdlib.h>
#include <string.h>

#define QR_RS_EC_MAX 68
#define QR_DATA_MAX_WORDS 120
#define QR_BIT_BUFFER_MAX 2000

static uint8_t qr_modules[QRCODE_SIZE_MAX][QRCODE_SIZE_MAX];
static int8_t qr_is_function[QRCODE_SIZE_MAX][QRCODE_SIZE_MAX];
static int qr_module_count = 0;

static uint8_t bit_buffer[QR_BIT_BUFFER_MAX / 8];
static int bit_buffer_len = 0;

static const uint8_t GF_EXP[256] = {
    1,   2,   4,   8,   16,  32,  64,  128, 29,  58,  116, 232, 205, 135, 19,  38,  76,  152, 45,  90,  180, 117,
    234, 201, 143, 3,   6,   12,  24,  48,  96,  192, 157, 39,  78,  156, 37,  74,  148, 53,  106, 212, 181, 119,
    238, 193, 159, 35,  70,  140, 5,   10,  20,  40,  80,  160, 93,  186, 105, 210, 185, 111, 222, 161, 95,  190,
    97,  194, 153, 47,  94,  188, 101, 202, 137, 15,  30,  60,  120, 240, 253, 231, 211, 187, 107, 214, 177, 127,
    254, 225, 223, 163, 91,  182, 113, 226, 217, 175, 67,  134, 17,  34,  68,  136, 13,  26,  52,  104, 208, 189,
    103, 206, 129, 31,  62,  124, 248, 237, 199, 147, 59,  118, 236, 197, 151, 51,  102, 204, 133, 23,  46,  92,
    184, 109, 218, 169, 79,  158, 33,  66,  132, 21,  42,  84,  168, 77,  154, 41,  82,  164, 85,  170, 73,  146,
    57,  114, 228, 213, 183, 115, 230, 209, 191, 99,  198, 145, 63,  126, 252, 229, 215, 179, 123, 246, 241, 255,
    227, 219, 171, 75,  150, 49,  98,  196, 149, 55,  110, 220, 165, 87,  174, 65,  130, 25,  50,  100, 200, 141,
    7,   14,  28,  56,  112, 224, 221, 167, 83,  166, 81,  162, 89,  178, 121, 242, 249, 239, 195, 155, 43,  86,
    172, 69,  138, 9,   18,  36,  72,  144, 61,  122, 244, 245, 247, 243, 251, 235, 203, 139, 11,  22,  44,  88,
    176, 125, 250, 233, 207, 131, 27,  54,  108, 216, 173, 71,  142, 1};

static const uint8_t GF_LOG[256] = {
    0,   0,   1,   25,  2,   50,  26,  198, 3,   223, 51,  238, 27,  104, 199, 75,  4,   100, 224, 14,  52,  141,
    239, 129, 28,  193, 105, 248, 200, 8,   76,  113, 5,   138, 101, 47,  225, 36,  15,  33,  53,  147, 142, 218,
    240, 18,  130, 69,  29,  181, 194, 125, 106, 39,  249, 185, 201, 154, 9,   120, 77,  228, 114, 166, 6,   191,
    139, 98,  102, 221, 48,  253, 226, 152, 37,  179, 16,  145, 34,  136, 54,  208, 148, 206, 143, 150, 219, 189,
    241, 210, 19,  92,  131, 56,  70,  64,  30,  66,  182, 163, 195, 72,  126, 110, 107, 58,  40,  84,  250, 133,
    186, 61,  202, 94,  155, 159, 10,  21,  121, 43,  78,  212, 229, 172, 115, 243, 167, 87,  7,   112, 192, 247,
    140, 128, 99,  13,  103, 74,  222, 237, 49,  197, 254, 24,  227, 165, 153, 119, 38,  184, 180, 124, 17,  68,
    146, 217, 35,  32,  137, 46,  55,  63,  209, 91,  149, 188, 207, 205, 144, 135, 151, 178, 220, 252, 190, 97,
    242, 86,  211, 171, 20,  42,  93,  158, 132, 60,  57,  83,  71,  109, 65,  162, 31,  45,  67,  216, 183, 123,
    164, 118, 196, 23,  73,  236, 127, 12,  111, 246, 108, 161, 59,  82,  41,  157, 85,  170, 251, 96,  134, 177,
    187, 204, 62,  90,  203, 89,  95,  176, 156, 169, 160, 81,  11,  245, 22,  235, 122, 117, 44,  215, 79,  174,
    213, 233, 230, 231, 173, 232, 116, 214, 244, 234, 168, 80,  88,  175};

static uint8_t gf_mul(uint8_t a, uint8_t b) {
  if (a == 0 || b == 0)
    return 0;
  return GF_EXP[(GF_LOG[a] + GF_LOG[b]) % 255];
}

static void rs_encode(const uint8_t *data, int data_len, int ec_count, uint8_t *ec) {
  uint8_t gen[QR_RS_EC_MAX + 1];
  uint8_t buffer[QR_DATA_MAX_WORDS + QR_RS_EC_MAX];
  int i, j;
  memset(buffer, 0, data_len + ec_count);
  memcpy(buffer, data, data_len);
  gen[0] = 1;
  for (i = 0; i < ec_count; i++) {
    gen[i + 1] = 1;
    for (j = i; j > 0; j--) {
      gen[j] = gf_mul(gen[j], GF_EXP[i]) ^ gen[j - 1];
    }
    gen[0] = gf_mul(gen[0], GF_EXP[i]);
  }
  for (i = 0; i < data_len; i++) {
    uint8_t feedback = buffer[i];
    if (feedback == 0)
      continue;
    for (j = 0; j < ec_count; j++) {
      buffer[i + j + 1] ^= gf_mul(feedback, gen[ec_count - 1 - j]);
    }
  }
  memcpy(ec, buffer + data_len, ec_count);
}

typedef struct {
  int version;
  int total_codewords;
  int data_codewords;
  int ec_per_block;
  int num_blocks;
} RS_Spec;

static const RS_Spec rs_table[4][40] = {
    // EC Level L
    {{1, 26, 19, 7, 1},        {2, 44, 34, 10, 1},       {3, 70, 55, 15, 1},       {4, 100, 80, 20, 1},
     {5, 134, 108, 26, 1},     {6, 172, 136, 18, 2},     {7, 196, 156, 20, 2},     {8, 242, 194, 24, 2},
     {9, 292, 232, 30, 2},     {10, 346, 274, 18, 4},    {11, 404, 324, 20, 4},    {12, 466, 370, 24, 4},
     {13, 532, 428, 26, 4},    {14, 581, 461, 30, 4},    {15, 655, 523, 22, 6},    {16, 733, 589, 24, 6},
     {17, 815, 647, 28, 6},    {18, 901, 721, 30, 6},    {19, 991, 795, 28, 8},    {20, 1085, 861, 28, 8},
     {21, 1156, 932, 28, 8},   {22, 1258, 1006, 28, 10}, {23, 1364, 1094, 30, 10}, {24, 1474, 1174, 30, 10},
     {25, 1568, 1276, 26, 12}, {26, 1788, 1372, 28, 12}, {27, 1896, 1468, 30, 12}, {28, 2024, 1594, 28, 14},
     {29, 2192, 1700, 30, 14}, {30, 2360, 1822, 24, 16}, {31, 2522, 1930, 28, 16}, {32, 2666, 2072, 30, 16},
     {33, 2744, 2124, 30, 17}, {34, 2854, 2194, 30, 18}, {35, 2972, 2294, 30, 18}, {36, 3028, 2334, 30, 19},
     {37, 3170, 2486, 26, 20}, {38, 3312, 2588, 28, 20}, {39, 3462, 2698, 30, 21}, {40, 3626, 2808, 30, 22}},
    // EC Level M
    {{1, 26, 16, 10, 1},       {2, 44, 28, 16, 1},       {3, 70, 44, 26, 1},       {4, 100, 64, 36, 1},
     {5, 134, 86, 48, 1},      {6, 172, 108, 32, 2},     {7, 196, 124, 40, 2},     {8, 242, 154, 36, 3},
     {9, 292, 182, 44, 3},     {10, 346, 216, 26, 4},    {11, 404, 254, 36, 4},    {12, 466, 290, 42, 4},
     {13, 532, 334, 46, 4},    {14, 581, 460, 36, 2},    {15, 655, 412, 40, 6},    {16, 733, 464, 44, 6},
     {17, 815, 516, 48, 6},    {18, 901, 568, 54, 6},    {19, 991, 614, 46, 8},    {20, 1085, 664, 48, 8},
     {21, 1156, 718, 48, 8},   {22, 1258, 782, 52, 8},   {23, 1364, 854, 54, 8},   {24, 1474, 916, 54, 10},
     {25, 1568, 996, 50, 12},  {26, 1788, 1072, 54, 12}, {27, 1896, 1148, 66, 12}, {28, 2024, 1254, 56, 14},
     {29, 2192, 1346, 56, 14}, {30, 2360, 1442, 52, 16}, {31, 2522, 1536, 56, 16}, {32, 2666, 1624, 60, 16},
     {33, 2744, 1692, 60, 16}, {34, 2854, 1768, 64, 16}, {35, 2972, 1852, 70, 16}, {36, 3028, 1906, 74, 17},
     {37, 3170, 1996, 70, 18}, {38, 3312, 2100, 72, 18}, {39, 3462, 2200, 76, 19}, {40, 3626, 2296, 76, 20}},
    // EC Level Q
    {{1, 26, 13, 13, 1},       {2, 44, 22, 22, 1},       {3, 70, 34, 36, 1},       {4, 100, 48, 52, 1},
     {5, 134, 62, 72, 1},      {6, 172, 84, 48, 2},      {7, 196, 98, 60, 2},      {8, 242, 138, 44, 3},
     {9, 292, 178, 52, 4},     {10, 346, 216, 36, 6},    {11, 404, 262, 48, 6},    {12, 466, 310, 56, 6},
     {13, 532, 352, 60, 6},    {14, 581, 418, 44, 8},    {15, 655, 478, 52, 8},    {16, 733, 538, 60, 8},
     {17, 815, 596, 64, 8},    {18, 901, 658, 72, 8},    {19, 991, 716, 56, 10},   {20, 1085, 780, 56, 10},
     {21, 1156, 854, 68, 10},  {22, 1258, 938, 76, 10},  {23, 1364, 1022, 76, 10}, {24, 1474, 1100, 78, 12},
     {25, 1568, 1184, 68, 12}, {26, 1788, 1280, 72, 12}, {27, 1896, 1372, 76, 12}, {28, 2024, 1464, 60, 14},
     {29, 2192, 1580, 74, 14}, {30, 2360, 1684, 78, 14}, {31, 2522, 1798, 78, 14}, {32, 2666, 1898, 80, 15},
     {33, 2744, 1978, 72, 16}, {34, 2854, 2060, 80, 16}, {35, 2972, 2156, 76, 17}, {36, 3028, 2208, 80, 17},
     {37, 3170, 2302, 72, 18}, {38, 3312, 2442, 72, 19}, {39, 3462, 2546, 76, 20}, {40, 3626, 2640, 80, 21}},
    // EC Level H
    {{1, 26, 9, 17, 1},         {2, 44, 16, 28, 1},        {3, 70, 26, 44, 1},        {4, 100, 36, 64, 1},
     {5, 134, 46, 88, 1},       {6, 172, 60, 64, 2},       {7, 196, 72, 80, 2},       {8, 242, 96, 52, 3},
     {9, 292, 130, 68, 4},      {10, 346, 168, 52, 6},     {11, 404, 206, 68, 6},     {12, 466, 244, 74, 6},
     {13, 532, 280, 88, 6},     {14, 581, 340, 52, 8},     {15, 655, 406, 62, 8},     {16, 733, 458, 82, 8},
     {17, 815, 520, 80, 8},     {18, 901, 578, 102, 8},    {19, 991, 638, 66, 10},    {20, 1085, 690, 66, 10},
     {21, 1156, 750, 80, 10},   {22, 1258, 818, 98, 10},   {23, 1364, 890, 106, 10},  {24, 1474, 956, 62, 12},
     {25, 1568, 1034, 86, 12},  {26, 1788, 1104, 94, 12},  {27, 1896, 1184, 106, 12}, {28, 2024, 1284, 70, 14},
     {29, 2192, 1388, 90, 14},  {30, 2360, 1496, 98, 14},  {31, 2522, 1594, 118, 14}, {32, 2666, 1700, 106, 15},
     {33, 2744, 1786, 110, 16}, {34, 2854, 1862, 130, 16}, {35, 2972, 1954, 76, 17},  {36, 3028, 1998, 86, 17},
     {37, 3170, 2102, 104, 18}, {38, 3312, 2192, 106, 19}, {39, 3462, 2298, 124, 19}, {40, 3626, 2396, 126, 20}}};

static int get_version_for_data(int data_len, QRErrorCorrectLevel ecl) {
  for (int v = 1; v <= QRCODE_VERSION_MAX; v++) {
    if (rs_table[ecl][v - 1].data_codewords >= data_len + 1) {
      return v;
    }
  }
  return QRCODE_VERSION_MAX;
}

static void bit_buffer_append(int bit, int count) {
  for (int i = 0; i < count; i++) {
    bit_buffer[bit_buffer_len / 8] = (bit_buffer[bit_buffer_len / 8] << 1) | (bit & 1);
    bit_buffer_len++;
  }
}

static void bit_buffer_append_byte(uint8_t byte, int count) {
  for (int i = count - 1; i >= 0; i--) {
    bit_buffer_append((byte >> i) & 1, 1);
  }
}

static void encode_byte_mode(const char *data, int len) {
  bit_buffer_append_byte(0x04, 4);
  int char_count_bits = 8;
  bit_buffer_append_byte(len, char_count_bits);
  for (int i = 0; i < len; i++) {
    bit_buffer_append_byte(data[i], 8);
  }
}

static int get_penalty_score(uint8_t matrix[QRCODE_SIZE_MAX][QRCODE_SIZE_MAX], int size) {
  int score = 0, r, c, count, dark, total, pct, prev, next, step;
  for (r = 0; r < size; r++) {
    count = 1;
    for (c = 1; c < size; c++) {
      if (matrix[r][c] == matrix[r][c - 1]) {
        count++;
      } else {
        if (count >= 5)
          score += count + 2;
        count = 1;
      }
    }
    if (count >= 5)
      score += count + 2;
  }
  for (c = 0; c < size; c++) {
    count = 1;
    for (r = 1; r < size; r++) {
      if (matrix[r][c] == matrix[r - 1][c]) {
        count++;
      } else {
        if (count >= 5)
          score += count + 2;
        count = 1;
      }
    }
    if (count >= 5)
      score += count + 2;
  }
  for (r = 0; r < size - 1; r++) {
    for (c = 0; c < size - 1; c++) {
      uint8_t v = matrix[r][c];
      if (matrix[r][c + 1] == v && matrix[r + 1][c] == v && matrix[r + 1][c + 1] == v)
        score += 3;
    }
  }
  for (r = 0; r < size; r++) {
    for (c = 0; c < size - 6; c++) {
      if (matrix[r][c] == 1 && matrix[r][c + 1] == 0 && matrix[r][c + 2] == 1 && matrix[r][c + 3] == 1 &&
          matrix[r][c + 4] == 1 && matrix[r][c + 5] == 0 && matrix[r][c + 6] == 1)
        score += 40;
    }
  }
  for (c = 0; c < size; c++) {
    for (r = 0; r < size - 6; r++) {
      if (matrix[r][c] == 1 && matrix[r + 1][c] == 0 && matrix[r + 2][c] == 1 && matrix[r + 3][c] == 1 &&
          matrix[r + 4][c] == 1 && matrix[r + 5][c] == 0 && matrix[r + 6][c] == 1)
        score += 40;
    }
  }
  dark = 0;
  for (r = 0; r < size; r++)
    for (c = 0; c < size; c++)
      if (matrix[r][c])
        dark++;
  total = size * size;
  pct = (dark * 100) / total;
  prev = (pct / 5) * 5;
  next = prev + 5;
  step = (abs(prev - 50) < abs(next - 50)) ? abs(prev - 50) : abs(next - 50);
  score += (step / 5) * 10;
  return score;
}

static void place_function_patterns(uint8_t matrix[QRCODE_SIZE_MAX][QRCODE_SIZE_MAX], int size) {
  int r, c, p, i, j;
  for (r = 0; r < size; r++)
    for (c = 0; c < size; c++)
      qr_is_function[r][c] = 0;
  for (p = 0; p < 3; p++) {
    int sr = (p == 2) ? size - 7 : 0;
    int sc = (p == 1) ? size - 7 : 0;
    for (r = -1; r <= 7; r++) {
      for (c = -1; c <= 7; c++) {
        int mr = sr + r;
        int mc = sc + c;
        if (mr < 0 || mr >= size || mc < 0 || mc >= size)
          continue;
        qr_is_function[mr][mc] = 1;
        if (r == -1 || r == 7 || c == -1 || c == 7) {
          matrix[mr][mc] = 0;
        } else if ((r == 0 || r == 6) && (c >= 0 && c <= 6)) {
          matrix[mr][mc] = 1;
        } else if ((c == 0 || c == 6) && (r >= 0 && r <= 6)) {
          matrix[mr][mc] = 1;
        } else if ((r >= 2 && r <= 4) && (c >= 2 && c <= 4)) {
          matrix[mr][mc] = 1;
        } else {
          matrix[mr][mc] = 0;
        }
      }
    }
  }
  for (i = 8; i < size - 8; i++) {
    qr_is_function[6][i] = 1;
    qr_is_function[i][6] = 1;
    matrix[6][i] = (i % 2 == 0) ? 1 : 0;
    matrix[i][6] = (i % 2 == 0) ? 1 : 0;
  }
  matrix[size - 8][8] = 1;
  qr_is_function[size - 8][8] = 1;
  for (i = 0; i <= 8; i++) {
    qr_is_function[i][8] = 1;
    qr_is_function[8][i] = 1;
  }
  for (i = 0; i < 7; i++) {
    qr_is_function[size - 1 - i][8] = 1;
    qr_is_function[8][size - 1 - i] = 1;
  }
  qr_is_function[8][size - 8] = 1; /* format info horizontal: last bit on right side */

  /* Alignment patterns (versions 2-6) */
  static const uint8_t align_pos[][2] = {
      {0, 0},  /* version 1: none */
      {6, 18}, /* version 2 */
      {6, 22}, /* version 3 */
      {6, 26}, /* version 4 */
      {6, 30}, /* version 5 */
      {6, 34}, /* version 6 */
  };
  if (size > 21) {
    int version = (size - 17) / 4;
    for (i = 0; i < 2; i++) {
      for (j = 0; j < 2; j++) {
        int ar = align_pos[version - 1][i];
        int ac = align_pos[version - 1][j];
        if (qr_is_function[ar][ac])
          continue; /* skip if overlaps with finder/timing */
        for (r = -2; r <= 2; r++) {
          for (c = -2; c <= 2; c++) {
            int mr = ar + r, mc = ac + c;
            qr_is_function[mr][mc] = 1;
            matrix[mr][mc] = (r == -2 || r == 2 || c == -2 || c == 2 || (r == 0 && c == 0)) ? 1 : 0;
          }
        }
      }
    }
  }
}

static int apply_mask(int mask, int r, int c) {
  switch (mask) {
    case 0:
      return (r + c) % 2 == 0;
    case 1:
      return r % 2 == 0;
    case 2:
      return c % 3 == 0;
    case 3:
      return (r + c) % 3 == 0;
    case 4:
      return ((r / 2) + (c / 3)) % 2 == 0;
    case 5:
      return ((r * c) % 2) + ((r * c) % 3) == 0;
    case 6:
      return (((r * c) % 2) + ((r * c) % 3)) % 2 == 0;
    case 7:
      return (((r + c) % 2) + ((r * c) % 3)) % 2 == 0;
    default:
      return 0;
  }
}

static void place_format_info(uint8_t matrix[QRCODE_SIZE_MAX][QRCODE_SIZE_MAX], int size, int mask,
                              QRErrorCorrectLevel ecl) {
  static const uint16_t fmt_table[] = {0x5412, 0x5125, 0x5E7C, 0x5B4B, 0x45F9, 0x40CE, 0x4F97, 0x4AA0,
                                       0x77C4, 0x72F3, 0x7DAA, 0x789D, 0x662F, 0x6318, 0x6C41, 0x6976,
                                       0x1689, 0x13BE, 0x1CE7, 0x19D0, 0x0762, 0x0255, 0x0D0C, 0x083B,
                                       0x355F, 0x3068, 0x3F31, 0x3A06, 0x24B4, 0x2183, 0x2EDA, 0x2BED};
  // C enum: L=0, M=1, Q=2, H=3; fmt_table groups: [0]=L, [1]=M, [2]=Q, [3]=H
  static const uint8_t ec_fmt_map[] = {0, 1, 2, 3};
  int fmt_idx = ec_fmt_map[ecl] * 8 + mask;
  uint16_t fmt_bits = fmt_table[fmt_idx % 32];
  int i;
  for (i = 0; i < 15; i++) {
    int bit = (fmt_bits >> i) & 1;
    int r, c;
    if (i < 6) {
      r = i;
      c = 8;
    } else if (i < 8) {
      r = i + 1;
      c = 8;
    } else {
      r = size - 15 + i;
      c = 8;
    }
    if (r >= 0 && r < size && c >= 0 && c < size)
      matrix[r][c] = bit;
    if (i < 8) {
      r = 8;
      c = size - i - 1;
    } else if (i < 9) {
      r = 8;
      c = 7;
    } else {
      r = 8;
      c = 14 - i;
    }
    if (r >= 0 && r < size && c >= 0 && c < size)
      matrix[r][c] = bit;
  }
}

static void place_data_modules(uint8_t matrix[QRCODE_SIZE_MAX][QRCODE_SIZE_MAX], int size, const uint8_t *data,
                               int total_bits) {
  int dir_up = 1;
  int col = size - 1;
  int bit_idx = 0;
  int i, sub;
  while (col > 0) {
    if (col == 6)
      col--;
    for (i = 0; i < size; i++) {
      int rr = dir_up ? (size - 1 - i) : i;
      for (sub = 0; sub < 2; sub++) {
        int cc = col - sub;
        if (cc >= 0 && !qr_is_function[rr][cc]) {
          if (bit_idx >= total_bits)
            return;
          int byte_idx = bit_idx / 8;
          int bit_pos = 7 - (bit_idx % 8);
          matrix[rr][cc] = (data[byte_idx] >> bit_pos) & 1;
          bit_idx++;
        }
      }
    }
    col -= 2;
    dir_up = !dir_up;
  }
}

static uint8_t work_mat1[QRCODE_SIZE_MAX][QRCODE_SIZE_MAX];
static uint8_t work_mat2[QRCODE_SIZE_MAX][QRCODE_SIZE_MAX];
static uint8_t best_masked[QRCODE_SIZE_MAX][QRCODE_SIZE_MAX];

static int select_best_mask(uint8_t matrix[QRCODE_SIZE_MAX][QRCODE_SIZE_MAX],
                            uint8_t masked[QRCODE_SIZE_MAX][QRCODE_SIZE_MAX], int size) {
  int best_score = 999999;
  int best_mask = 0;
  int m, r, c;
  memcpy(work_mat1, matrix, QRCODE_SIZE_MAX * QRCODE_SIZE_MAX);
  for (m = 0; m < 8; m++) {
    memcpy(work_mat2, work_mat1, QRCODE_SIZE_MAX * QRCODE_SIZE_MAX);
    for (r = 0; r < size; r++)
      for (c = 0; c < size; c++)
        if (!qr_is_function[r][c] && apply_mask(m, r, c))
          work_mat2[r][c] = !work_mat2[r][c];
    int score = get_penalty_score(work_mat2, size);
    if (score < best_score) {
      best_score = score;
      best_mask = m;
      memcpy(best_masked, work_mat2, QRCODE_SIZE_MAX * QRCODE_SIZE_MAX);
    }
  }
  if (masked != best_masked)
    memcpy(masked, best_masked, QRCODE_SIZE_MAX * QRCODE_SIZE_MAX);
  return best_mask;
}

void qrcode_init(void) {
  memset(qr_modules, 0, sizeof(qr_modules));
  memset(qr_is_function, 0, sizeof(qr_is_function));
  qr_module_count = 0;
}

int qrcode_encode_string(const char *data, QRErrorCorrectLevel ecl, QRCode *outQr) {
  if (!data || !outQr)
    return -1;
  int data_len = strlen(data);
  int version = get_version_for_data(data_len, ecl);
  const RS_Spec *spec = &rs_table[ecl][version - 1];
  int size = 17 + 4 * version;
  if (size > QRCODE_SIZE_MAX)
    return -2;
  qr_module_count = size;
  memset(qr_modules, 0, sizeof(qr_modules));
  memset(qr_is_function, 0, sizeof(qr_is_function));
  bit_buffer_len = 0;
  encode_byte_mode(data, data_len);
  int remaining_data_bits = spec->data_codewords * 8;
  int term_bits = (bit_buffer_len <= remaining_data_bits - 4) ? 4 : (remaining_data_bits - bit_buffer_len);
  if (term_bits > 0)
    bit_buffer_append(0, term_bits);
  while (bit_buffer_len % 8 != 0)
    bit_buffer_append(0, 1);
  uint8_t pad_vals[] = {0xEC, 0x11};
  int pad_idx = 0;
  while (bit_buffer_len < remaining_data_bits) {
    for (int i = 7; i >= 0; i--)
      bit_buffer_append((pad_vals[pad_idx] >> i) & 1, 1);
    pad_idx ^= 1;
  }
  uint8_t data_words[QR_DATA_MAX_WORDS];
  int data_bytes = bit_buffer_len / 8;
  for (int i = 0; i < data_bytes; i++)
    data_words[i] = bit_buffer[i];
  uint8_t ec_words[QR_RS_EC_MAX];
  uint8_t all_codewords[200];
  int total_placed = 0;
  int block_size = spec->data_codewords / spec->num_blocks;

  ESP_LOGI("QR", "data_len=%d, version=%d, ecl=%d, data_cw=%d, ec_per_block=%d, blocks=%d, total=%d", data_len, version,
           ecl, spec->data_codewords, spec->ec_per_block, spec->num_blocks, spec->total_codewords);

  for (int b = 0; b < spec->num_blocks; b++) {
    memcpy(all_codewords + total_placed, data_words + b * block_size, block_size);
    total_placed += block_size;
    rs_encode(data_words + b * block_size, block_size, spec->ec_per_block, ec_words);
    memcpy(all_codewords + total_placed, ec_words, spec->ec_per_block);
    total_placed += spec->ec_per_block;
  }

  place_function_patterns(qr_modules, size);
  int total_words = spec->total_codewords;
  place_data_modules(qr_modules, size, all_codewords, total_words * 8);
  uint8_t(*final_matrix)[QRCODE_SIZE_MAX] = work_mat2;
  int best_mask = select_best_mask(qr_modules, final_matrix, size);
  place_format_info(final_matrix, size, best_mask, ecl);
  for (int r = 0; r < size; r++)
    for (int c = 0; c < size; c++)
      outQr->modules[r][c] = final_matrix[r][c];
  outQr->width = size;

  ESP_LOGI("QR", "QRCode v%d %dx%d mask=%d OK", version, size, size, best_mask);

  return 0;
}

int qrcode_get_module(int row, int col) {
  if (row < 0 || row >= qr_module_count || col < 0 || col >= qr_module_count)
    return 0;
  return qr_modules[row][col];
}

void draw_qrcode(QRCode *qr, uint16_t x, uint16_t y, uint8_t module_size) {
  int size = qr->width;
  int quiet = 4;
  int total = (size + 2 * quiet) * module_size;
  spilcd_fill(x, y, x + total - 1, y + total - 1, WHITE);
  int margin = quiet * module_size;
  for (int r = 0; r < size; r++) {
    for (int c = 0; c < size; c++) {
      if (qr->modules[r][c]) {
        uint16_t px = x + margin + c * module_size;
        uint16_t py = y + margin + r * module_size;
        for (uint8_t dy = 0; dy < module_size; dy++) {
          for (uint8_t dx = 0; dx < module_size; dx++) {
            spilcd_draw_point(px + dx, py + dy, BLACK);
          }
        }
      }
    }
  }
}


void qrcode_show_hello_world(void) {
  QRCode qr;
  const char *text = "WIFI:T:WPA;S:esp32-s3-wifi;P:123456789;;";
  // const char *text = "hello world";

  int ret = qrcode_encode_string(text, QR_ECLEVEL_M, &qr);
  if (ret != 0) {
    spilcd_show_string(10, 100, 240, 24, 16, "encode failed", RED);
    return;
  }

  uint16_t total_px = (qr.width + 8) * 4;
  uint16_t cx = (spilcddev.width - total_px) / 2;
  uint16_t cy = 30;
  draw_qrcode(&qr, cx, cy, 4);
}