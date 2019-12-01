#include "display.h"

#include "lib/systick.h"
#include <stdio.h>
#include <string.h>

static ucg_t *ucg;

void display_setup(void) {
  ucg = display_setup_base();
}

void display_update(void) {
  static uint8_t first=1;

  if(!first)
    return;

  ucg_SetColor(ucg, 0, 255, 0, 0);
  ucg_SetColor(ucg, 2, 0, 255, 0);
  ucg_SetColor(ucg, 1, 0, 0, 255);
  ucg_SetColor(ucg, 3, 255, 255, 255);
  ucg_DrawGradientBox(ucg, 0, 0, ucg_GetWidth(ucg), ucg_GetHeight(ucg));
  ucg_SendBuffer(ucg);

  first = 0;
  // while(1) {
  //   // Blue > Red > Green > White
  //   for(int16_t d=255, i=0 ; d >= 0 && i <= 255 ; d-=5, i+=5) {
  //     ucg_SetColor(ucg, 2, i, 0, d); // Blue > Red
  //     ucg_SetColor(ucg, 0, d, i, 0);  // Red > Green
  //     ucg_SetColor(ucg, 1, i, 255, i); // Green > White
  //     ucg_SetColor(ucg, 3, d, d, 255);  // White > Blue
  //     ucg_DrawGradientBox(ucg, 0, 0, ucg_GetWidth(ucg), ucg_GetHeight(ucg));
  //   }
  //   // Red > Green > White > Blue
  //   for(int16_t d=255, i=0 ; d >= 0 && i <= 255 ; d-=5, i+=5) {
  //     ucg_SetColor(ucg, 2, d, i, 0); // Red > Green
  //     ucg_SetColor(ucg, 0, i, 255, i);  // Green > White
  //     ucg_SetColor(ucg, 1, d, d, 255); // White > Blue
  //     ucg_SetColor(ucg, 3, i, 0, d);  // Blue > Red
  //     ucg_DrawGradientBox(ucg, 0, 0, ucg_GetWidth(ucg), ucg_GetHeight(ucg));
  //   }
  //   // Green > White > Blue > Red
  //   for(int16_t d=255, i=0 ; d >= 0 && i <= 255 ; d-=5, i+=5) {
  //     ucg_SetColor(ucg, 2, i, 255, i); // Green > White
  //     ucg_SetColor(ucg, 0, d, d, 255);  // White > Blue
  //     ucg_SetColor(ucg, 1, i, 0, d); // Blue > Red
  //     ucg_SetColor(ucg, 3, d, i, 0);  // Red > Green
  //     ucg_DrawGradientBox(ucg, 0, 0, ucg_GetWidth(ucg), ucg_GetHeight(ucg));
  //   }
  //   // White > Blue > Red > Green
  //   for(int16_t d=255, i=0 ; d >= 0 && i <= 255 ; d-=5, i+=5) {
  //     ucg_SetColor(ucg, 2, d, d, 255); // White > Blue
  //     ucg_SetColor(ucg, 0, i, 0, d);  // Blue > Red
  //     ucg_SetColor(ucg, 1, d, i, 0); // Red > Green
  //     ucg_SetColor(ucg, 3, i, 255, i);  // Green > White
  //     ucg_DrawGradientBox(ucg, 0, 0, ucg_GetWidth(ucg), ucg_GetHeight(ucg));
  //   }
  // }
}