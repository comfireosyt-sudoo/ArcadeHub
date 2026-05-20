#include "shared.h"

// Draw the top header bar
void drawHeader(const char* title) {
  Draw::fillRect(0, 0, SW, 22, Col::DarkBlue);
  Draw::drawRect(0, 0, SW, 22, Col::Blue);
  Draw::textCentered(title, 4, Col::Gold, Col::DarkBlue, true);
}

// Generic scrollable list menu.
// Returns selected index, or -1 if Back pressed.
int runMenu(const char** items, int count, int startSel, const char* title) {
  int sel      = startSel;
  int scrollTop = 0;
  constexpr int IH = 20;
  constexpr int MY = 24;
  int visible = (SH - MY - 14) / IH;

  Input::Edge inp;
  inp.reset();
  int frame = 0;

  while (true) {
    Draw::fillRect(0, MY, SW, SH - MY, Col::DarkGray);
    drawHeader(title);

    if (sel < scrollTop) scrollTop = sel;
    if (sel >= scrollTop + visible) scrollTop = sel - visible + 1;

    for (int i = 0; i < visible && scrollTop+i < count; i++) {
      int idx = scrollTop + i;
      int y   = MY + i * IH + 2;
      bool s  = (idx == sel);
      EADK::Color bg  = s ? Col::Blue    : Col::DarkGray;
      EADK::Color fg  = s ? Col::White   : Col::LightGray;
      Draw::fillRect(6, y, SW-12, IH-2, bg);
      if (s) Draw::drawRect(6, y, SW-12, IH-2, Col::Cyan);
      Draw::text(items[idx], 14, y+4, fg, bg);
    }

    if (scrollTop > 0)
      Draw::textCentered("^", MY+2, Col::Yellow, Col::DarkGray);
    if (scrollTop + visible < count)
      Draw::textCentered("v", SH-12, Col::Yellow, Col::DarkGray);

    Draw::fillRect(0, SH-12, SW, 12, Col::Black);
    Draw::text("OK:Select  Back:Return", 4, SH-11, Col::MidGray, Col::Black);

    frame++;
    Input::Keys k = inp.updateRepeat(frame);
    if (k.up   && sel > 0)       sel--;
    if (k.down && sel < count-1) sel++;
    if (k.ok)   return sel;
    if (k.back) return -1;

    EADK::Timing::msleep(16);
  }
}
