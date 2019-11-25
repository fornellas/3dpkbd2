#include "display.h"

uint8_t first=1;

void display_update(void) {
  char buff[30];

  if(!first)
    return;

  ucg_SetColor(&ucg, 0, 255, 255, 255);
  ucg_SetMaxClipRange(&ucg);
  ucg_DrawBox(&ucg, 0, 0, ucg_GetWidth(&ucg), ucg_GetHeight(&ucg));

  first = 0;
}