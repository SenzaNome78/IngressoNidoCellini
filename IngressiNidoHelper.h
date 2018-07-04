#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

/*
Scrive sull'lcd la stringa testo.
@param testo: riga di testo da visualizzare
@param testo: numero di riga dove visualizzare la stringa
@param testo: dove mettere il primo spazio, true se all'inizio
@param testo: riferimento all'oggetto lcd da usare
*/
void LcdPrintCentered(String testo, uint8_t riga, bool spcBegin, LiquidCrystal_I2C& lcd);

// Funzione che fa emettere un suono al nostro buzzer
void PlayBuzzer();

// Riavviamo il microcontrollore ESP8266.
// Per un bug dello stesso è necessario fare un power cycle
// (togliere e rimettere l'alimentazione)
// per far funzionare a dovere il comando
void ResetLettore();

// Estrai il valore di un parametro da una stringa contenente
// uno o più parametri (tuttiParametri).
// tuttiParametri deve essere così formata:
// ?primoParamChiave=primoParamValore&secondoParamChiave=secondoParamValore&...!
// punto interrogativo iniziale, = divide chiave da valore, & divide parametri, ! chiude stringa
// tuttiParametri: Contiene tutti i parametri da cui estrarne uno
// paramChiave: nome della chiave da cui trarre il valore
// restituisce il valore richiesto o "" se non trovato
String EstraiValoreParametro(String tuttiParametri, String pararChiave);