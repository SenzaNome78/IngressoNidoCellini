#include "IngressiNidoHelper.h"
#include "Tone.cpp"
#include <Arduino.h>

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



// Estrai il valore di un parametro da una stringa contenente
// uno o più parametri (tuttiParametri).
// tuttiParametri deve essere così formata:
// ?primoParamChiave=primoParamValore&secondoParamChiave=secondoParamValore&...&
// punto interrogativo iniziale, = divide chiave da valore, & divide parametri, & chiude stringa
// tuttiParametri: Contiene tutti i parametri da cui estrarne uno
// paramChiave: nome della chiave da cui trarre il valore
// restituisce il valore richiesto, STRINGA_MAL_FORMATA se la stringa tutti i paramentri
// era mal formata (vedere in alto) oppure NESSUNA_CHIAVE se non abbiamo trovato nessuna chiave
String EstraiValoreParametro(String tuttiParametri, String paramChiave)
{
	Serial.print("Inizio EstraiValore: ");
	Serial.println(tuttiParametri);

	int posChiave = 0; // posizione della chiave del nostro parametro
	int posValue = 0; // la posizione del nostro valore
	String valore = ""; // Il valore che stavamo cercando

	// Analizziamo la stringa tuttiParametri per vedere se è ben formata
	if ((tuttiParametri.startsWith("?")) && (tuttiParametri.endsWith("&")) && (tuttiParametri.indexOf("=") != -1))
	{
		// Rimuovo il punto interrogativo iniziale
		tuttiParametri.remove(0, 1);
		// Rimuovo il punto esclamativo finale
		//tuttiParametri.remove(tuttiParametri.length()-1);

		posChiave = tuttiParametri.indexOf(paramChiave); // posizione della chiave
		
		if (posChiave != -1) // abbiamo trovato la nostra chiave
		{
			posValue = posChiave + paramChiave.length() + 1;
			valore = paramChiave.substring(posValue), tuttiParametri.indexOf("=", posValue);
		}
		else // La chiave che cercavamo non era presente
		{
			return "NESSUNA_CHIAVE";
		}
		return valore;
	}

	return "STRINGA_MAL_FORMATA";
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


// Riavviamo il microcontrollore ESP8266.
// Per un bug è necessario fare un power cycle
// (togliere e rimettere la corrente)
// per far funzionare a dovere il comando
void ResetLettore()
{
	ESP.restart();
}

