/*
 * LettoreRfid.cpp
 *
 *  Created on: 21 mag 2018
 *      Author: David
 */

#include "LettoreRfid.h"

 // Definisci la chiave per l'autenticazione del badge


// Variabile per il blocco di memoria contenente il nome
byte bloccoNome = 12;

// Variabile per il blocco di memoria contenente il ruolo (un solo byte: B o E)
byte bloccoRuolo = 13;

// Variabile per il blocco di memoria contenente il sesso (un solo byte: F o M)
byte bloccoSesso = 14;

MFRC522::StatusCode status;	// Status code dal lettore

// Lunghezza della stringa del nome
//byte len;

// Costruttore
LettoreRfid::LettoreRfid() :
	mfrc522(SS_PIN, RST_PIN)
{
	//	mfrc522 = new MFRC522(SS_PIN, RST_PIN);
	SPI.begin();
	mfrc522.PCD_Init();   // Init MFRC522
	//NuovaRilevazione = true;
}

/***********************************************************************
 * Rileva se un badge è stato avvicinato al lettore.
 * Se è così viene inizializzato con PICC_ReadCardSeriale e ritorna true
 * altrimenti ritorna false
 */
bool LettoreRfid::BadgeRilevato()
{
	resetMembers();
	if (mfrc522.PICC_IsNewCardPresent())
	{
		if (mfrc522.PICC_ReadCardSerial())
		{
			nomeUser = LeggiBlocco(bloccoNome);
			ruoloUser = LeggiBlocco(bloccoRuolo);
			sessoUser = LeggiBlocco(bloccoSesso);
			
			// Nella stringa tmpSerial mettiamo il seriale del badge,
			// lo inviamo a SetNuovaRilevazione per aggiungerlo alla lista
			// e per settare la variabile NuovaRilevazione
			String tmpSerial = "";
			for (int i = 0; i < mfrc522.uid.size; i++)
			{
				tmpSerial += String(mfrc522.uid.uidByte[i]);
			}

			// Funzione che si occupa della registrazione del seriale
			SetNuovaRilevazione(tmpSerial);

			mfrc522.PCD_StopCrypto1();
			mfrc522.PICC_HaltA();

			return true;
		}
		else
		{
			mfrc522.PCD_StopCrypto1();
			mfrc522.PICC_HaltA();

			return false;
		}
	}
	else
	{
		mfrc522.PCD_StopCrypto1();
		mfrc522.PICC_HaltA();

		return false;
	}
}

/*
 *  Legge un blocco e ritorna la stringa in esso contenuta
 */
String LettoreRfid::LeggiBlocco(byte numBlocco)
{
	MFRC522::MIFARE_Key key = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

	// Autenticazione per il blocco della PICC contenente il nome
	status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, numBlocco, &key, &(mfrc522.uid));
	if (status != MFRC522::STATUS_OK) // Non siamo riusciti ad identificarci
	{
		Serial.print("Autenticazione fallita: ");
		Serial.println(mfrc522.GetStatusCodeName(status));
		return "";
	}

	// Lunghezza del buffer (min 18)
	byte bufferLen = 18;
	// array per lavorare con i dati ottenuti dal badge (deve essere minimo 18 byte)
	byte buffer1[18];
	byte buffer2[18];

	// Legge il nome dello user dalla PICC e lo deposita in buffer1
	status = mfrc522.MIFARE_Read(numBlocco, buffer1, &bufferLen);
	if (status != MFRC522::STATUS_OK) // Non siamo riusciti a leggere il nome
	{
		Serial.print("lettura nome fallita: ");
		Serial.println(mfrc522.GetStatusCodeName(status));

		return "";
	}

	//Scrive il nome del badge da buffer1 al membro nomeUser
	String tmpString = "";
	for (uint8_t i = 0; i < 16; i++)
	{
		if (isAlphaNumeric(buffer1[i])) // Aggiunge solo caratteri e numeri alla stringa
		{
			tmpString += char(buffer1[i]);
		}
	}
	return tmpString;

}

void LettoreRfid::SetNuovaRilevazione(uint32 tmpSerial)
{
	// Il badge non conteneva uno dei campi, esci e setta la variabile
	// badgeDaRegistrare come true
	if ((nomeUser == "") || (ruoloUser == "") || (sessoUser == ""))
	{
		badgeDaRegistrare = true;
		return;
	}
	else
	{
		badgeDaRegistrare = false;
	}

	setSeriale(tmpSerial);

	// Se il seriale è già presente in questa sessione
	// Setta NuovaRilevazione a false ed esce dalla funzione
	for (byte i = 0; i < MAX_USERS; i++)
	{
		if (strArrayUid[i][0] == getSeriale())
		{
			NuovaRilevazione = false;
			return;
		}
	}

	// Se siamo qui il seriale è nuovo in questa sessione
	// Cerchiamo il primo spazio vuoto nella array strArrayUid
	// e lo inseriamo in quel posto
	for (byte i = 0; i < MAX_USERS; i++)
	{
		if (strArrayUid[i][0] == "")
		{
			strArrayUid[i][0] = getSeriale();
			NuovaRilevazione = true;
			return;
		}
	}
}

/*
 * Scriviamo un nuovo badge con il nome e il ruolo inviatoci dal server web
 */
bool LettoreRfid::ScriviNuovoBadge(String testoNome, String testoRuolo, String testoSesso)
{
	//resetMembers();

	//if (testoNome == "" || testoNome.length() > 16 || testoRuolo == ""
	//	|| testoRuolo.length() > 16)
	//{
	//	Serial.println("Esco da ScriviNuovoBadge");
	//	return false;
	//}

	if (mfrc522.PICC_IsNewCardPresent())
	{
		if (mfrc522.PICC_ReadCardSerial())
		{
			ScriviBlocco(bloccoNome, testoNome);
			ScriviBlocco(bloccoRuolo, testoRuolo);
			ScriviBlocco(bloccoSesso, testoSesso);
			mfrc522.PCD_StopCrypto1();
			mfrc522.PICC_HaltA();

			return true;
		}
		else
		{
			mfrc522.PCD_StopCrypto1();
			mfrc522.PICC_HaltA();
			return false;
		}
	}
	else
	{
		mfrc522.PCD_StopCrypto1();
		mfrc522.PICC_HaltA();
		return false;
	}
}

 bool LettoreRfid::SetIdPresenza(uint32 seriale, uint16 idPresenza)
 {
	 // Cerchiamo la presenza a cui dobbiamo aggiungere l'idPresenza
	 for (byte i = 0; i < MAX_USERS; i++)
	 {
		 if (strArrayUid[i][0] == seriale)
		 {
			 strArrayUid[i][1] = idPresenza;
			 return true;
		 }
	 }

	 return false;
 }

bool LettoreRfid::ScriviBlocco(byte block, String stringa)
{
	MFRC522::MIFARE_Key key = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

	// Autenticazione per il blocco della PICC che conterrà il nome
	status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));

	if (status != MFRC522::STATUS_OK) // Non siamo riusciti ad identificarci
	{
		Serial.print("Autenticazione fallita: ");
		Serial.println(mfrc522.GetStatusCodeName(status));
		Serial.print("Nome: "); Serial.println(stringa);
		Serial.print("Blocco: "); Serial.println(block);
		return false;
	}

	byte bufferLen = 16;
	byte buffer1[16];

	// scrivo nel buffer il testo stringa
	for (uint8_t i = 0; i < bufferLen; i++)
	{
		if (isAlphaNumeric(stringa[i]))
		{
			buffer1[i] = stringa[i];
		}
		else buffer1[i] = 0;
	}

	// Scrive il nome nella PICC
	status = mfrc522.MIFARE_Write(block, buffer1, bufferLen);
	if (status != MFRC522::STATUS_OK) // Non siamo riusciti a scrivere il nome
	{
		Serial.print("Scrittura fallita: ");
		Serial.print("stringa: "); Serial.println(stringa);
		Serial.print("Blocco: "); Serial.println(block);
		Serial.println(mfrc522.GetStatusCodeName(status));

		return false;
	}

	return true;
}

bool LettoreRfid::PulisciBlocco(byte block)
{
	Serial.println("IN pulisci blocco01");
	byte buffer1[16];
	// Ripulisci i blocchi di memoria usando un buffer vuoto
	for (uint8_t i = 0; i < 16; i++)
	{
		buffer1[i] = 0;
	}
	Serial.println("IN pulisci blocco02");
	status = mfrc522.MIFARE_Write(block, buffer1, 16);
	Serial.println("IN pulisci blocco03");
	if (status != MFRC522::STATUS_OK) // Non siamo riusciti a scrivere il nome
	{
		Serial.print("Pulizia fallita: ");
		Serial.println(mfrc522.GetStatusCodeName(status));

		return false;
	}
	Serial.println("IN pulisci blocco04");
	for (int i = 0; i < 16; ++i)
	{
		Serial.print("buffer[");
		Serial.print(i);
		Serial.print("]: ");
		Serial.println(char(buffer1[i]));

	}
	Serial.println("IN pulisci blocco05");
	return true;
}

void LettoreRfid::resetMembers()
{
	nomeUser = "";
	ruoloUser = "";
	sessoUser = "";
	seriale = "";
}

LettoreRfid::~LettoreRfid()
{
	mfrc522.PCD_StopCrypto1();
}

String LettoreRfid::getNomeUser()
{
	return nomeUser;
}

void LettoreRfid::setNomeUser(String nomeUser)
{

	this->nomeUser = nomeUser;
}

String LettoreRfid::getRuoloUser()
{
	return ruoloUser;
}

String LettoreRfid::getSessoUser()
{
	return sessoUser;
}

void LettoreRfid::setRuoloUser(String ruoloUser)
{
	this->ruoloUser = ruoloUser;
}

void LettoreRfid::setSessoUser(String sessoUser)
{
	this->sessoUser = sessoUser;
}

bool LettoreRfid::isNuovaRilevazione()
{
	return NuovaRilevazione;
}

void LettoreRfid::CancellaSerialeOggi(uint32 seriale)
{
	for (byte i = 0; i < MAX_USERS; i++)
	{
		if (strArrayUid[i][0] == seriale)
		{
			strArrayUid[i][0] = "";
			strArrayUid[i][1] = "";
			return;
		}
	}
}

bool LettoreRfid::GetBadgeDaRegistrare()
{
	return badgeDaRegistrare;
}

void LettoreRfid::SetBadgeDaRegistrare(bool val)
{
	badgeDaRegistrare = val;
}





