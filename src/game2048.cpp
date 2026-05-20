#include "shared.h"

static uint32_t g48[4][4];
static uint32_t sc48, hi48, tile48;
static uint32_t rng48;
static bool     won48;

static uint32_t r48(){rng48^=rng48<<13;rng48^=rng48>>17;rng48^=rng48<<5;return rng48;}

static void addTile(){
  int e[16],n=0;
  for(int r=0;r<4;r++) for(int c=0;c<4;c++) if(!g48[r][c]) e[n++]=r*4+c;
  if(!n) return;
  int idx=e[r48()%n];
  g48[idx/4][idx%4]=(r48()%10<9)?2:4;
}

static EADK::Color tileCol(uint32_t v){
  switch(v){
    case 0:    return EADK::Color(0x303030);
    case 2:    return EADK::Color(0xEEE4DA);
    case 4:    return EADK::Color(0xEDE0C8);
    case 8:    return EADK::Color(0xF2B179);
    case 16:   return EADK::Color(0xF59563);
    case 32:   return EADK::Color(0xF67C5F);
    case 64:   return EADK::Color(0xF65E3B);
    case 128:  return EADK::Color(0xEDCF72);
    case 256:  return EADK::Color(0xEDCC61);
    case 512:  return EADK::Color(0xEDC850);
    case 1024: return EADK::Color(0xEDC53F);
    case 2048: return EADK::Color(0xEDC22E);
    default:   return EADK::Color(0x3C3A32);
  }
}

static bool mergeRow(uint32_t row[4]){
  bool changed=false;
  uint32_t tmp[4]={};int ti=0;
  for(int i=0;i<4;i++) if(row[i]) tmp[ti++]=row[i];
  for(int i=0;i<3;i++) if(tmp[i]&&tmp[i]==tmp[i+1]){
    tmp[i]*=2; sc48+=tmp[i];
    if(tmp[i]>tile48) tile48=tmp[i];
    if(tmp[i]==2048) won48=true;
    for(int j=i+1;j<3;j++) tmp[j]=tmp[j+1]; tmp[3]=0;
  }
  for(int i=0;i<4;i++) if(tmp[i]!=row[i]) changed=true;
  for(int i=0;i<4;i++) row[i]=tmp[i];
  return changed;
}

static bool slideLeft() {bool c=false;for(int r=0;r<4;r++) if(mergeRow(g48[r]))c=true;return c;}
static bool slideRight(){bool c=false;for(int r=0;r<4;r++){uint32_t rev[4];for(int i=0;i<4;i++)rev[i]=g48[r][3-i];if(mergeRow(rev)){for(int i=0;i<4;i++)g48[r][3-i]=rev[i];c=true;}}return c;}
static bool slideUp()  {bool c=false;for(int col=0;col<4;col++){uint32_t v[4];for(int r=0;r<4;r++)v[r]=g48[r][col];if(mergeRow(v)){for(int r=0;r<4;r++)g48[r][col]=v[r];c=true;}}return c;}
static bool slideDown(){bool c=false;for(int col=0;col<4;col++){uint32_t v[4];for(int r=0;r<4;r++)v[r]=g48[3-r][col];if(mergeRow(v)){for(int r=0;r<4;r++)g48[3-r][col]=v[r];c=true;}}return c;}

static bool canMove(){
  for(int r=0;r<4;r++) for(int c=0;c<4;c++){
    if(!g48[r][c]) return true;
    if(c<3&&g48[r][c]==g48[r][c+1]) return true;
    if(r<3&&g48[r][c]==g48[r+1][c]) return true;
  } return false;
}

static constexpr int TS=54,TG=5,TBX=(SW-(4*TS+3*TG))/2,TBY=30;

static void render48(){
  Draw::fillScreen(Col::Black);
  Draw::fillRect(0,0,SW,28,Col::DarkBlue);
  Draw::text("2048",4,5,Col::Gold,Col::DarkBlue);
  char buf[24];
  fmt(buf,24,"Score:%u",sc48); Draw::text(buf,100,8,Col::White,Col::DarkBlue);
  fmt(buf,24,"Best:%u",hi48);  Draw::text(buf,220,8,Col::Silver,Col::DarkBlue);
  Draw::fillRect(TBX-TG,TBY-TG,4*(TS+TG)+TG,4*(TS+TG)+TG,EADK::Color(0x776E65));
  for(int r=0;r<4;r++) for(int c=0;c<4;c++){
    int px=TBX+c*(TS+TG), py=TBY+r*(TS+TG);
    uint32_t v=g48[r][c];
    EADK::Color bg=tileCol(v);
    Draw::fillRect(px,py,TS,TS,bg);
    if(v){
      EADK::Color fg=(v<=4)?Col::DarkGray:Col::White;
      char s[8]; fmt(s,8,"%u",v);
      int tw=Draw::strW(s,v<100);
      Draw::text(s,px+(TS-tw)/2,py+(TS-11)/2,fg,bg);
    }
  }
  Draw::text("Arrow:Slide  Back:Quit",4,SH-10,Col::MidGray,Col::Black);
}

void run2048(){
  rng48=(uint32_t)EADK::Timing::millis()^0x20480248U;
  hi48=g_save.g2048High; tile48=g_save.g2048Tile; sc48=0; won48=false;
  memset(g48,0,sizeof(g48)); addTile(); addTile();

  Input::Edge inp; inp.reset();
  bool contAfterWin=false;

  while(true){
    Input::Keys k=inp.update();
    if(k.back) break;
    bool moved=false;
    if(k.up)    moved=slideUp();
    if(k.down)  moved=slideDown();
    if(k.left)  moved=slideLeft();
    if(k.right) moved=slideRight();
    if(moved) addTile();
    if(sc48>hi48) hi48=sc48;
    render48();
    if(won48&&!contAfterWin){
      Draw::fillRect(50,90,220,60,Col::DarkBlue); Draw::drawRect(50,90,220,60,Col::Gold);
      Draw::textCentered("YOU REACHED 2048!",100,Col::Gold,Col::DarkBlue);
      Draw::textCentered("OK:Continue Back:Quit",118,Col::LightGray,Col::DarkBlue);
      Input::waitRelease(); inp.reset();
      while(true){Input::Keys k2=inp.update();if(k2.ok){contAfterWin=true;break;}if(k2.back)goto done48;EADK::Timing::msleep(16);}
    }
    if(!canMove()) break;
    EADK::Timing::msleep(16);
  }
done48:
  if(sc48>g_save.g2048High) g_save.g2048High=sc48;
  if(tile48>g_save.g2048Tile) g_save.g2048Tile=tile48;
  savePersist();
  render48();
  Draw::fillRect(70,90,180,55,Col::DarkBlue); Draw::drawRect(70,90,180,55,Col::Red);
  Draw::textCentered("GAME OVER",100,Col::White,Col::DarkBlue,true);
  char buf[24]; fmt(buf,24,"Score:%u",sc48); Draw::textCentered(buf,124,Col::Gold,Col::DarkBlue);
  Draw::textCentered("OK to exit",138,Col::LightGray,Col::DarkBlue);
  Input::waitRelease(); inp.reset();
  while(true){Input::Keys k2=inp.update();if(k2.ok||k2.back)break;EADK::Timing::msleep(16);}
}
