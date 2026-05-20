#include "shared.h"
#include "menu.h"

// Forward declarations of all game entry points
void runTetris();
void runSudoku();
void runWaterSort();
void runMemory();
void runBlockBlast();
void runFruitMerge();
void runHangman();
void runCheckers();
void runSnake();
void run2048();
void runMinesweeper();

// ---- Title screen ----
static void showTitle() {
  Input::Edge inp; inp.reset();
  for (int frame = 0; frame < 80; frame++) {
    Draw::fillScreen(Col::DarkBlue);
    // Star field
    for (int i = 0; i < 40; i++) {
      int sx = (i * 137 + 31) % SW;
      int sy = (i * 97  + 13) % SH;
      EADK::Color sc = ((frame + i) % 20 < 10) ? Col::White : Col::LightGray;
      Draw::pixel(sx, sy, sc);
    }
    // Pulsing border
    int pulse = (frame % 20 < 10) ? (frame % 10) : (10 - frame % 10);
    uint32_t br = 0x004000 + (uint32_t)pulse * 0x002000;
    Draw::fillRect(20, 48, 280, 82, Col::DarkGray);
    Draw::drawRect(20, 48, 280, 82, EADK::Color(br));
    Draw::drawRect(21, 49, 278, 80, EADK::Color(br / 2));
    Draw::textCentered("ARCADE HUB", 62, Col::Gold, Col::DarkGray, true);
    Draw::textCentered("11 Classic Games", 90, Col::LightGray, Col::DarkGray);
    Draw::textCentered("NumWorks Edition v1.0", 108, Col::MidGray, Col::DarkGray);
    Draw::textCentered("Press OK to Start", 165, Col::Yellow, Col::DarkBlue);
    Draw::textCentered("Tetris Sudoku Snake 2048", 185, Col::MidGray, Col::DarkBlue);
    Draw::textCentered("Checkers Minesweeper +more", 197, Col::MidGray, Col::DarkBlue);

    Input::Keys k = inp.update();
    if (k.ok || k.back) break;
    EADK::Timing::msleep(30);
  }
  Input::waitRelease();
  // Wait for OK press
  inp.reset();
  while (true) {
    Draw::fillScreen(Col::DarkBlue);
    for (int i = 0; i < 40; i++) {
      Draw::pixel((i*137+31)%SW, (i*97+13)%SH, Col::LightGray);
    }
    Draw::fillRect(20, 48, 280, 82, Col::DarkGray);
    Draw::drawRect(20, 48, 280, 82, Col::Gold);
    Draw::textCentered("ARCADE HUB", 62, Col::Gold, Col::DarkGray, true);
    Draw::textCentered("11 Classic Games", 90, Col::LightGray, Col::DarkGray);
    Draw::textCentered("NumWorks Edition v1.0", 108, Col::MidGray, Col::DarkGray);
    Draw::textCentered("Press OK to Start", 165, Col::Yellow, Col::DarkBlue);

    Input::Keys k = inp.update();
    if (k.ok) break;
    if (k.back) return;
    EADK::Timing::msleep(16);
  }
  Input::waitRelease();
  inp.reset();
}

// ---- Stats screen ----
static void showStats() {
  Input::Edge inp; inp.reset();
  while (true) {
    Draw::fillScreen(Col::DarkGray);
    drawHeader("STATISTICS");
    char buf[32];
    int y = 30;
    auto line = [&](const char* lbl, uint32_t val) {
      Draw::text(lbl, 10, y, Col::LightGray, Col::DarkGray);
      fmt(buf, 32, "%u", val);
      int w = Draw::strW(buf);
      Draw::text(buf, SW - w - 10, y, Col::Gold, Col::DarkGray);
      y += 17;
    };
    line("Tetris Best Score:", g_save.tetrisHigh);
    line("Tetris Total Lines:", g_save.tetrisLines);
    line("Sudoku Completed:", g_save.sudokuDone[0]+g_save.sudokuDone[1]+g_save.sudokuDone[2]);
    line("Water Sort Levels:", __builtin_popcount(g_save.wsCompleted));
    line("Memory Games Won:", g_save.memWins);
    line("Block Blast Best:", g_save.bbHigh);
    line("Fruit Merge Best:", g_save.fmHigh);
    line("Hangman Wins:", g_save.hmWins);
    line("Snake Best:", g_save.snakeHigh);
    line("2048 Best Score:", g_save.g2048High);
    line("Minesweeper Wins:", g_save.msWins[0]+g_save.msWins[1]+g_save.msWins[2]);
    Draw::textCentered("Back: Return", SH - 12, Col::MidGray, Col::DarkGray);

    Input::Keys k = inp.update();
    if (k.back || k.ok) break;
    EADK::Timing::msleep(16);
  }
  Input::waitRelease();
  inp.reset();
}

// ---- Settings screen ----
static void showSettings() {
  Input::Edge inp; inp.reset();
  int sel = 0, frame = 0;
  while (true) {
    Draw::fillScreen(Col::DarkGray);
    drawHeader("SETTINGS");
    const char* diffNames[] = {"Easy", "Medium", "Hard"};
    int y = 40;
    auto drawSetting = [&](int idx, const char* lbl, const char* val) {
      bool s = (idx == sel);
      Draw::fillRect(8, y, SW-16, 20, s ? Col::Blue : Col::DarkGray);
      if (s) Draw::drawRect(8, y, SW-16, 20, Col::Cyan);
      Draw::text(lbl, 16, y+4, s?Col::White:Col::LightGray, s?Col::Blue:Col::DarkGray);
      int vw = Draw::strW(val);
      Draw::text(val, SW-vw-16, y+4, Col::Gold, s?Col::Blue:Col::DarkGray);
      y += 24;
    };
    y = 40;
    drawSetting(0, "Default Difficulty", diffNames[g_save.defaultDiff]);
    Draw::textCentered("Left/Right: Change  Back: Save", SH-12, Col::MidGray, Col::DarkGray);

    frame++;
    Input::Keys k = inp.updateRepeat(frame);
    if (k.up   && sel > 0) sel--;
    if (k.down && sel < 0) sel++;
    if (k.left || k.right) {
      int d = k.right ? 1 : -1;
      if (sel == 0) g_save.defaultDiff = (g_save.defaultDiff + 3 + d) % 3;
    }
    if (k.back || k.ok) break;
    EADK::Timing::msleep(16);
  }
  savePersist();
  Input::waitRelease();
  inp.reset();
}

// ---- Credits screen ----
static void showCredits() {
  Input::Edge inp; inp.reset();
  Draw::fillScreen(Col::DarkBlue);
  drawHeader("CREDITS");
  const char* lines[] = {
    "ARCADE HUB v1.0",
    "for NumWorks Calculator",
    "",
    "Games included:",
    "Tetris  Sudoku  Snake",
    "2048  Minesweeper",
    "Water Sort  Memory",
    "Block Blast  Checkers",
    "Fruit Merge  Hangman",
    "",
    "Built with EADK SDK",
    "",
    "Press any key to return"
  };
  int y = 28;
  for (int i = 0; i < 13; i++) {
    Draw::textCentered(lines[i], y, (i==0)?Col::Gold:Col::LightGray, Col::DarkBlue);
    y += 14;
  }
  while (true) {
    Input::Keys k = inp.update();
    if (k.ok || k.back) break;
    EADK::Timing::msleep(16);
  }
  Input::waitRelease();
  inp.reset();
}

// ---- Game list ----
static const char* k_gameNames[] = {
  "1.  Tetris",
  "2.  Sudoku",
  "3.  Water Sort",
  "4.  Memory Match",
  "5.  Block Blast",
  "6.  Fruit Merge",
  "7.  Hangman",
  "8.  Checkers",
  "9.  Snake",
  "10. 2048",
  "11. Minesweeper"
};
static constexpr int NUM_GAMES = 11;

static void launchGame(int idx) {
  Input::waitRelease();
  switch (idx) {
    case 0:  runTetris();      break;
    case 1:  runSudoku();      break;
    case 2:  runWaterSort();   break;
    case 3:  runMemory();      break;
    case 4:  runBlockBlast();  break;
    case 5:  runFruitMerge();  break;
    case 6:  runHangman();     break;
    case 7:  runCheckers();    break;
    case 8:  runSnake();       break;
    case 9:  run2048();        break;
    case 10: runMinesweeper(); break;
  }
  savePersist();
  Input::waitRelease();
}

// ---- Main menu ----
static const char* k_mainItems[] = {
  "Play Games",
  "Statistics",
  "Settings",
  "Credits",
  "Quit"
};
static constexpr int NUM_MAIN = 5;

// ============================================================
// EADK entry point
// ============================================================
void eadk_main() {
  saveLoad();
  showTitle();

  static int lastMainSel = 0;
  static int lastGameSel = 0;

  bool running = true;
  while (running) {
    int sel = runMenu(k_mainItems, NUM_MAIN, lastMainSel, "ARCADE HUB");
    if (sel >= 0) lastMainSel = sel;

    switch (sel) {
      case 0: {
        int g = runMenu(k_gameNames, NUM_GAMES, lastGameSel, "SELECT GAME");
        if (g >= 0) { lastGameSel = g; launchGame(g); }
        break;
      }
      case 1: showStats();    break;
      case 2: showSettings(); break;
      case 3: showCredits();  break;
      case 4: running = false; break;
      case -1: /* back from main = do nothing */ break;
    }
  }

  // Goodbye screen
  Draw::fillScreen(Col::DarkBlue);
  Draw::textCentered("Thanks for playing!", SH/2 - 10, Col::Gold, Col::DarkBlue, true);
  Draw::textCentered("ARCADE HUB", SH/2 + 10, Col::LightGray, Col::DarkBlue);
  EADK::Timing::msleep(1500);
}
