#pragma once
#include <eadk.h>
#include <string.h>
#include <stdint.h>

// ---- Screen dimensions ----
static constexpr int SW = 320;
static constexpr int SH = 240;

// ---- Color helpers ----
namespace Col {
  static constexpr EADK::Color Black     = EADK::Color(0x000000);
  static constexpr EADK::Color White     = EADK::Color(0xFFFFFF);
  static constexpr EADK::Color DarkGray  = EADK::Color(0x222222);
  static constexpr EADK::Color MidGray   = EADK::Color(0x555555);
  static constexpr EADK::Color LightGray = EADK::Color(0xAAAAAA);
  static constexpr EADK::Color Red       = EADK::Color(0xE04040);
  static constexpr EADK::Color Green     = EADK::Color(0x40C040);
  static constexpr EADK::Color Blue      = EADK::Color(0x4080FF);
  static constexpr EADK::Color Yellow    = EADK::Color(0xFFD040);
  static constexpr EADK::Color Orange    = EADK::Color(0xFF8020);
  static constexpr EADK::Color Purple    = EADK::Color(0xA040E0);
  static constexpr EADK::Color Cyan      = EADK::Color(0x40E0E0);
  static constexpr EADK::Color Pink      = EADK::Color(0xFF60A0);
  static constexpr EADK::Color DarkBlue  = EADK::Color(0x001060);
  static constexpr EADK::Color DarkGreen = EADK::Color(0x006010);
  static constexpr EADK::Color Gold      = EADK::Color(0xFFCC00);
  static constexpr EADK::Color Silver    = EADK::Color(0xC0C0C0);
  static constexpr EADK::Color Brown     = EADK::Color(0x804020);
}

// ---- Drawing helpers ----
namespace Draw {

inline void fillRect(int x, int y, int w, int h, EADK::Color c) {
  if (w <= 0 || h <= 0) return;
  EADK::Display::pushRectUniform(EADK::Rect(x, y, w, h), c);
}

inline void fillScreen(EADK::Color c) {
  fillRect(0, 0, SW, SH, c);
}

inline void drawRect(int x, int y, int w, int h, EADK::Color c) {
  fillRect(x,       y,       w, 1, c);
  fillRect(x,       y+h-1,   w, 1, c);
  fillRect(x,       y,       1, h, c);
  fillRect(x+w-1,   y,       1, h, c);
}

inline void hLine(int x, int y, int w, EADK::Color c) { fillRect(x, y, w, 1, c); }
inline void vLine(int x, int y, int h, EADK::Color c) { fillRect(x, y, 1, h, c); }
inline void pixel(int x, int y, EADK::Color c)        { fillRect(x, y, 1, 1, c); }

inline void fillCircle(int cx, int cy, int r, EADK::Color c) {
  for (int dy = -r; dy <= r; dy++) {
    int dx = (int)__builtin_sqrt((float)(r*r - dy*dy));
    fillRect(cx - dx, cy + dy, 2*dx+1, 1, c);
  }
}

// ---- Text rendering via EADK ----
// EADK provides drawString with small (11px) and large (16px) fonts
inline void text(const char* s, int x, int y, EADK::Color fg, EADK::Color bg) {
  EADK::Display::drawString(s, EADK::Point(x, y), false, fg, bg);
}
inline void textLarge(const char* s, int x, int y, EADK::Color fg, EADK::Color bg) {
  EADK::Display::drawString(s, EADK::Point(x, y), true, fg, bg);
}

// Char width: small=7, large=10 (approximate for EADK font)
inline int strW(const char* s, bool large = false) {
  int n = 0; while (s[n]) n++;
  return n * (large ? 10 : 7);
}

inline void textCentered(const char* s, int y, EADK::Color fg, EADK::Color bg, bool large = false) {
  int x = (SW - strW(s, large)) / 2;
  if (large) textLarge(s, x, y, fg, bg);
  else        text(s, x, y, fg, bg);
}

inline void drawButton(int x, int y, int w, int h, const char* lbl, bool sel) {
  EADK::Color bg  = sel ? Col::Blue    : Col::DarkGray;
  EADK::Color fg  = sel ? Col::White   : Col::LightGray;
  EADK::Color brd = sel ? Col::Cyan    : Col::MidGray;
  fillRect(x, y, w, h, bg);
  drawRect(x, y, w, h, brd);
  int tx = x + (w - strW(lbl)) / 2;
  int ty = y + (h - 11) / 2;
  text(lbl, tx, ty, fg, bg);
}

} // namespace Draw

// ---- Minimal snprintf ----
inline int fmt(char* buf, int sz, const char* f, ...) {
  // va_list manual unpack (avoid full <cstdio> dependency)
  // Supports %d %u %s %c %%
  __builtin_va_list ap;
  __builtin_va_start(ap, f);
  int o = 0;
  for (; *f && o < sz-1; f++) {
    if (*f != '%') { buf[o++] = *f; continue; }
    f++;
    if (*f == '%') { buf[o++] = '%'; continue; }
    if (*f == 'c') { buf[o++] = (char)__builtin_va_arg(ap, int); continue; }
    if (*f == 's') {
      const char* sv = __builtin_va_arg(ap, const char*);
      while (*sv && o < sz-1) buf[o++] = *sv++;
      continue;
    }
    if (*f == 'd' || *f == 'i' || *f == 'u') {
      int v = __builtin_va_arg(ap, int);
      bool neg = (*f == 'd' || *f == 'i') && v < 0;
      unsigned uv = neg ? (unsigned)(-v) : (unsigned)v;
      char tmp[12]; int ti = 0;
      if (uv == 0) tmp[ti++] = '0';
      while (uv > 0) { tmp[ti++] = '0' + uv%10; uv /= 10; }
      if (neg) buf[o++] = '-';
      while (ti > 0 && o < sz-1) buf[o++] = tmp[--ti];
      continue;
    }
  }
  buf[o] = 0;
  __builtin_va_end(ap);
  return o;
}

// ---- Input helpers ----
namespace Input {

using Key = EADK::Keyboard::Key;

struct Keys {
  bool up, down, left, right, ok, back;
  bool num[10]; // 0-9
  bool any() const { return up||down||left||right||ok||back; }
};

inline Keys scan() {
  EADK::Keyboard::State s = EADK::Keyboard::scan();
  Keys k{};
  k.up    = s.keyDown(Key::Up);
  k.down  = s.keyDown(Key::Down);
  k.left  = s.keyDown(Key::Left);
  k.right = s.keyDown(Key::Right);
  k.ok    = s.keyDown(Key::OK);
  k.back  = s.keyDown(Key::Back);
  // Number keys
  static const Key nk[] = {
    Key::Zero,Key::One,Key::Two,Key::Three,Key::Four,
    Key::Five,Key::Six,Key::Seven,Key::Eight,Key::Nine
  };
  for (int i = 0; i < 10; i++) k.num[i] = s.keyDown(nk[i]);
  return k;
}

// Edge detector: only true on the frame the key was first pressed
struct Edge {
  Keys prev{};
  Keys update() {
    Keys cur = scan();
    Keys e{};
    e.up    = cur.up    && !prev.up;
    e.down  = cur.down  && !prev.down;
    e.left  = cur.left  && !prev.left;
    e.right = cur.right && !prev.right;
    e.ok    = cur.ok    && !prev.ok;
    e.back  = cur.back  && !prev.back;
    for (int i=0;i<10;i++) e.num[i] = cur.num[i] && !prev.num[i];
    prev = cur;
    return e;
  }
  // With auto-repeat for directional keys
  Keys updateRepeat(int frame) {
    Keys e = update();
    if (frame > 20 && frame % 6 == 0) {
      Keys cur = scan();
      e.up    = e.up    || cur.up;
      e.down  = e.down  || cur.down;
      e.left  = e.left  || cur.left;
      e.right = e.right || cur.right;
    }
    return e;
  }
  void reset() { prev = {}; }
};

inline void waitRelease() {
  while (scan().any()) EADK::Timing::msleep(10);
}

} // namespace Input

// ---- Save system (EADK storage) ----
// EADK::ExternalData provides a small persistent byte buffer
// We XOR-checksum our struct and store it there.

static constexpr uint32_t SAVE_MAGIC = 0xAC4D0001U;

struct SaveData {
  uint32_t magic;
  // Tetris
  uint32_t tetrisHigh, tetrisLines, tetrisGames;
  uint8_t  tetrisMaxLevel;
  // Sudoku
  uint32_t sudokuDone[3];
  bool     sudokuHasSave;
  uint8_t  sudokuDiff;
  uint8_t  sudokuPuzzle[81];
  uint8_t  sudokuSolution[81];
  // Water sort
  uint32_t wsLevel;
  uint32_t wsCompleted;
  // Memory
  uint32_t memBest[3]; // seconds, 0xFFFFFFFF=none
  uint32_t memWins;
  // Block blast
  uint32_t bbHigh, bbGames;
  // Fruit merge
  uint32_t fmHigh, fmMerges;
  // Hangman
  uint32_t hmWins, hmLosses;
  // Checkers
  uint32_t chkWins[3], chkLosses, chkDraws;
  // Snake
  uint32_t snakeHigh, snakeGames;
  // 2048
  uint32_t g2048High, g2048Tile;
  // Minesweeper
  uint32_t msWins[3], msLosses[3], msBest[3];
  // Settings
  uint8_t  defaultDiff;
  // Checksum (must be last)
  uint32_t checksum;
};

// Global save instance
extern SaveData g_save;

void saveLoad();
void savePersist();
void saveReset();
