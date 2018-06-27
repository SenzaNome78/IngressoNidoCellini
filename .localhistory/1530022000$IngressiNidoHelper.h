#pragma once
#include <LiquidCrystal_I2C.h>

void LcdPrintCentered(String testo, uint8_t riga, bool spcBegin, LiquidCrystal_I2C& lcd);

void PlayBuzzer();

void ResetLettore();

// Estrai il valore di un parametro da una stringa contenente
// uno o più parametri (tuttiParametri).
// tuttiParametri deve essere così formata:
// ?primoParamChiave=primoParamValore&secondoParamChiave=secondoParamValore&...!
// punto interrogativo iniziale, = divide chiave da valore, & divide parametri, ! chiude stringa
// tuttiParametri: Contiene tutti i parametri da cui estrarne uno
// paramChiave: nome della chiave da cui trarre il valore
// restituisce il valore richiesto o "" se non trovato
String EstraiParametro(String tuttiParametri, String pararChiave);