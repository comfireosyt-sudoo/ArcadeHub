#include "shared.h"

SaveData g_save;

static uint32_t calcChecksum() {
  const uint8_t* b = (const uint8_t*)&g_save;
  uint32_t c = 0xDEADBEEFU;
  size_t len = sizeof(SaveData) - 4; // exclude checksum field
  for (size_t i = 0; i < len; i++) {
    c ^= ((uint32_t)b[i]) << ((i%4)*8);
    c  = (c << 13) | (c >> 19);
  }
  return c;
}

void saveReset() {
  memset(&g_save, 0, sizeof(g_save));
  g_save.magic          = SAVE_MAGIC;
  g_save.memBest[0]     = 0xFFFFFFFF;
  g_save.memBest[1]     = 0xFFFFFFFF;
  g_save.memBest[2]     = 0xFFFFFFFF;
  g_save.msBest[0]      = 0xFFFFFFFF;
  g_save.msBest[1]      = 0xFFFFFFFF;
  g_save.msBest[2]      = 0xFFFFFFFF;
  g_save.defaultDiff    = 1;
  g_save.checksum       = calcChecksum();
}

void saveLoad() {
  // EADK external data: read raw bytes
  size_t sz = sizeof(SaveData);
  const uint8_t* ext = (const uint8_t*)EADK::ExternalData::address();
  if (ext && EADK::ExternalData::size() >= sz) {
    memcpy(&g_save, ext, sz);
    if (g_save.magic != SAVE_MAGIC || g_save.checksum != calcChecksum()) {
      saveReset();
    }
  } else {
    saveReset();
  }
}

void savePersist() {
  g_save.magic     = SAVE_MAGIC;
  g_save.checksum  = calcChecksum();
  // Write back
  uint8_t* ext = (uint8_t*)EADK::ExternalData::address();
  if (ext && EADK::ExternalData::size() >= sizeof(SaveData)) {
    memcpy(ext, &g_save, sizeof(SaveData));
  }
}
