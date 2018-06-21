#include "IngressiNidoHelper.h"
//#include "Tone.cpp"

/*
Scrive sull'lcd la stringa testo.
@param testo: riga di testo da visualizzare
@param testo: numero di riga dove visualizzare la stringa
@param testo: dove mettere il primo spazio, true se all'inizio
@param testo: riferimento all'oggetto lcd da usare
*/
void LcdPrintCentered(String testo, uint8_t riga, bool spcBegin, LiquidCrystal_I2C& lcd)
{
	// Togliamo gli spazi esterni dalla stringa
	testo.trim();

	// Se la stringa è maggiore di 20, la tronchiamo a 20 caratteri
	if (testo.length() > 20)
		testo = testo.substring(0, 20);

	// Ci salviamo la lunghezza del testo originale
	uint8_t origStrLength = testo.length();
	
	// se il parametro spcBegin è true, iniziamo a mettere gli spazi esterni partendo dall'inizio
	if (spcBegin == true)
	{
		// il for durerà per il numero di caratteri da aggiungere
		for (uint8_t i = 0; i < 20 - origStrLength; i++)
		{
			// Aggiungiamo uno spazio all'inizio
			testo = " " + testo;

			// Se il testo è arrivato a 20 caratteri possiamo uscire
			if (testo.length() == 20)
				break;
			else // altrimenti aggiungiamo uno spazio alla fine
			{
				testo = testo + " ";
			}
			if (testo.length() == 20) // Ricontrolliamo se il testo ha raggiunto i venti caratteri
				break;
		}
	}
	else // spcBegin è false, iniziamo a mettere gli spazi dalla fine
	{
		for (uint8_t i = 0; i < 20 - origStrLength; i++)
		{
			// Aggiungiamo uno spazio alla fine
			testo = testo + " ";

			// Se il testo è arrivato a 20 caratteri possiamo uscire
			if (testo.length() == 20)
				break;
			else // altrimenti aggiungiamo uno spazio all'inizio
			{
				testo = " " + testo;
			}
			if (testo.length() == 20)
				break;
		}
	}

	// Arrivati qui il nostro testo sarà di venti caratteri, con spazi iniziali se necessario
	lcd.setCursor(0, riga);
	lcd.print(testo);
}

// Funzione che fa emettere un suono al nostro buzzer

void PlayBuzzer(){
	int piezoPin = 16;

	// Passiamo alla funzione tone il pin collegato al buzzer,
	// la frequenza del segnale PWM (50% duty cycle) e la durata del suono
	tone(piezoPin, 800, 30);
	delay(60);
	tone(piezoPin, 1200, 30);
	delay(60);
	tone(piezoPin, 1400, 40);
	delay(60);
	tone(piezoPin, 1600, 40);
	delay(60);

	// Interrompiamo l'emissione del tono
	noTone(piezoPin);
}

void ResetLettore()
{
	// Riavviamo il microcontrollore ESP8266
	// Per un bug è necessario fare un power cycle
	// (togliere e rimettere la corrente)
	// per far funzionare a dovere il comando
	ESP.restart();
}
