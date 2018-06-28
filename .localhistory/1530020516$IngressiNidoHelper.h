#pragma once
#include <LiquidCrystal_I2C.h>

void LcdPrintCentered(String testo, uint8_t riga, bool spcBegin, LiquidCrystal_I2C& lcd);

void PlayBuzzer();

void ResetLettore();