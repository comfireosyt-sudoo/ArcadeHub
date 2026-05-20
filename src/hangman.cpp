#include "shared.h"

static const char* hmWords[]={
  "ELEPHANT","PENGUIN","DOLPHIN","KANGAROO","GIRAFFE","CROCODILE","FLAMINGO","CHEETAH",
  "PARIS","TOKYO","LONDON","BERLIN","SYDNEY","VENICE","PRAGUE","ATHENS",
  "ALGORITHM","COMPILER","DATABASE","FIRMWARE","NETWORK","KEYBOARD","PROCESSOR","PYTHON",
  "PIZZA","SUSHI","TACOS","RAMEN","PASTA","CROISSANT","MANGO","PRETZEL"
};
static constexpr int HM_WORDS=32;
static constexpr int HM_WRONG=7;

static char   hmWord[17];
static bool   hmGuessed[26];
static int    hmWrong,hmCur;
static uint32_t hmRng;

static uint32_t hmRand(){hmRng^=hmRng<<13;hmRng^=hmRng>>17;hmRng^=hmRng<<5;return hmRng;}

static bool hmComplete(){for(int i=0;hmWord[i];i++) if(!hmGuessed[hmWord[i]-'A']) return false;return true;}

static void hmDrawMan(int wrong){
  int gx=8,gy=24;
  Draw::vLine(gx+20,gy,80,Col::LightGray); Draw::hLine(gx+20,gy,55,Col::LightGray);
  Draw::vLine(gx+75,gy,18,Col::LightGray); Draw::hLine(gx+20,gy+80,35,Col::LightGray);
  if(wrong>=1) Draw::fillCircle(gx+75,gy+26,9,Col::White);
  if(wrong>=2) Draw::vLine(gx+75,gy+35,26,Col::White);
  if(wrong>=3) Draw::hLine(gx+58,gy+44,17,Col::White);
  if(wrong>=4) Draw::hLine(gx+75,gy+44,17,Col::White);
  if(wrong>=5) { Draw::vLine(gx+68,gy+61,18,Col::White); }
  if(wrong>=6) { Draw::vLine(gx+82,gy+61,18,Col::White); }
  if(wrong>=7) { Draw::pixel(gx+72,gy+25,Col::Red);Draw::pixel(gx+74,gy+27,Col::Red);
                 Draw::pixel(gx+74,gy+25,Col::Red);Draw::pixel(gx+72,gy+27,Col::Red);
                 Draw::pixel(gx+77,gy+25,Col::Red);Draw::pixel(gx+79,gy+27,Col::Red);
                 Draw::pixel(gx+79,gy+25,Col::Red);Draw::pixel(gx+77,gy+27,Col::Red); }
}

static void hmRender(){
  Draw::fillScreen(Col::Black);
  Draw::fillRect(0,0,SW,18,Col::DarkBlue);
  Draw::text("HANGMAN",4,2,Col::Gold,Col::DarkBlue);
  char buf[24]; fmt(buf,24,"Wrong:%d/%d",hmWrong,HM_WRONG);
  Draw::text(buf,200,4,Col::Red,Col::DarkBlue);
  hmDrawMan(hmWrong);
  // Word display
  int wlen=0; while(hmWord[wlen]) wlen++;
  int sw=wlen*17, startX=(SW-sw)/2, wy=118;
  for(int i=0;i<wlen;i++){
    int px=startX+i*17;
    Draw::hLine(px,wy+14,13,Col::White);
    if(hmGuessed[hmWord[i]-'A']){char s[2]={hmWord[i],0};Draw::textLarge(s,px+2,wy,Col::Cyan,Col::Black);}
  }
  // Keyboard
  const char* rows[3]={"ABCDEFGHI","JKLMNOPQR","STUVWXYZ"};
  int ky=140; int li=0;
  for(int row=0;row<3;row++){
    int rlen=row<2?9:8;
    int kx=(SW-rlen*20)/2;
    for(int col=0;col<rlen;col++){
      char ch=rows[row][col];
      bool g=hmGuessed[ch-'A'];
      bool sel=(li==hmCur);
      bool wrong=g&&[&](){for(int i=0;hmWord[i];i++) if(hmWord[i]==ch) return false;return true;}();
      EADK::Color bg=sel?Col::Gold:wrong?Col::DarkGray:g?Col::DarkGreen:EADK::Color(0x303060);
      EADK::Color fg=(sel||g)?Col::Black:Col::White;
      Draw::fillRect(kx,ky,18,15,bg);
      if(sel) Draw::drawRect(kx,ky,18,15,Col::White);
      char s[2]={ch,0}; Draw::text(s,kx+5,ky+2,fg,bg);
      kx+=20; li++;
    }
    ky+=19;
  }
  Draw::text("Arrow:Move OK:Guess Back:Quit",2,SH-10,Col::MidGray,Col::Black);
}

void runHangman(){
  hmRng=(uint32_t)EADK::Timing::millis()^0xBADC0DE1U;
  memset(hmGuessed,0,sizeof(hmGuessed)); hmWrong=0; hmCur=0;
  int idx=(int)(hmRand()%HM_WORDS);
  const char* w=hmWords[idx]; int i=0;
  while(w[i]&&i<16){hmWord[i]=w[i];i++;} hmWord[i]=0;

  Input::Edge inp; inp.reset(); int frame=0;

  while(true){
    Input::Keys k=inp.updateRepeat(frame++);
    if(k.back) return;
    if(k.right&&hmCur<25) hmCur++;
    if(k.left &&hmCur>0)  hmCur--;
    if(k.down &&hmCur+9<=25) hmCur+=9;
    if(k.up   &&hmCur-9>=0)  hmCur-=9;
    if(k.ok&&!hmGuessed[hmCur]){
      hmGuessed[hmCur]=true;
      bool found=false;
      for(int j=0;hmWord[j];j++) if(hmWord[j]=='A'+hmCur) found=true;
      if(!found) hmWrong++;
    }
    hmRender();
    bool won=hmComplete(), lost=(hmWrong>=HM_WRONG);
    if(won||lost){
      Draw::fillRect(50,88,220,60,Col::DarkBlue);
      Draw::drawRect(50,88,220,60,won?Col::Gold:Col::Red);
      Draw::textCentered(won?"YOU WIN!":"GAME OVER",98,won?Col::Gold:Col::White,Col::DarkBlue,true);
      char buf[32]; fmt(buf,32,"Word: %s",hmWord);
      Draw::textCentered(buf,122,Col::Cyan,Col::DarkBlue);
      Draw::textCentered("OK to exit",138,Col::LightGray,Col::DarkBlue);
      if(won) g_save.hmWins++; else g_save.hmLosses++;
      savePersist();
      Input::waitRelease(); inp.reset();
      while(true){Input::Keys k2=inp.update();if(k2.ok||k2.back)break;EADK::Timing::msleep(16);}
      return;
    }
    EADK::Timing::msleep(50);
  }
}
