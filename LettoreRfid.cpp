#include "LettoreRfid.h"

MFRC522::StatusCode status;	// Status code dal lettore

// Costruttore
LettoreRfid::LettoreRfid() :
	mfrc522(SS_PIN, RST_PIN)
{
	SPI.begin();
	mfrc522.PCD_Init();

	// Inizializzo l'array contenente i seriali inseriti e l'id delle presenze
	for (int i = 0; i < MAX_USERS; i++)
	{
		strArrayUid[i][0] = "";
		strArrayUid[i][1] = "";
	}
}

/***********************************************************************
 * Rileva se un badge � stato avvicinato al lettore.
 * Se � cos� viene inizializzato con PICC_ReadCardSeriale.
 * Se viene passato true al parametro lettura stiamo rilevando una presenza
 * memorizziamo il seriale del badge e eseguiamo la funzione SetNuovaRilevazione
 * Altrimenti stiamo effetuando una scrittura, leggiamo sempre il seriale
 * ma non ci interessa se sia una nuova rilevazione o cosa contengano i blocchi
 */
bool LettoreRfid::BadgeRilevato()
{
	if (mfrc522.PICC_IsNewCardPresent())
	{
		resetMembers();
		if (mfrc522.PICC_ReadCardSerial())
		{
			// Per prima cosa registriamo il seriale nel membro 
			// serialeCorrente
			String tmpSerial = "";
			for (int i = 0; i < mfrc522.uid.size; i++)
			{
				tmpSerial += String(mfrc522.uid.uidByte[i]);
			}
			setSerialeCorrente(tmpSerial);

			// Leggiamo questi valori dal badge
			nomeUser = LeggiBlocco(bloccoNome);
			ruoloUser = LeggiBlocco(bloccoRuolo);
			sessoUser = LeggiBlocco(bloccoSesso);

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

// Legge un blocco di memoria da un badge
// block � il numero del blocco da leggere
// restituisce la stringa contenuta nel blocco
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

// Registra una nuova presenza se non gi� inserita
// NuovaRilevazione viene impostata true o false se
// si tratta di un nuovo inserimento o meno
void LettoreRfid::SetNuovaRilevazione(String tmpSerial)
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

	setSerialeCorrente(tmpSerial);

	// Se il seriale � gi� presente in questa sessione
	// imposta NuovaRilevazione a false ed esce dalla funzione
	for (byte i = 0; i < MAX_USERS; i++)
	{
		if (strArrayUid[i][0] == getSerialeCorrente())
		{
			NuovaRilevazione = false;
			return;
		}
	}

	// Se siamo qui il seriale � nuovo in questa sessione
	// Cerchiamo il primo spazio vuoto nella array strArrayUid
	// e lo inseriamo in quel posto
	for (byte i = 0; i < MAX_USERS; i++)
	{
		if (strArrayUid[i][0] == "")
		{
			strArrayUid[i][0] = getSerialeCorrente();
			NuovaRilevazione = true;
			return;
		}
	}
}

/*
 * Scriviamo un nuovo badge con il nome e il ruolo inviatoci dal server web
 */
uint8_t LettoreRfid::ScriviNuovoBadge(String testoNome, String testoRuolo, String testoSesso)
{
	if (mfrc522.PICC_IsNewCardPresent())
	{
		resetMembers();
		if (mfrc522.PICC_ReadCardSerial())
		{
			// Per prima cosa registriamo il seriale nel membro 
			// serialeCorrente
			String tmpSerial = "";
			for (int i = 0; i < mfrc522.uid.size; i++)
			{
				tmpSerial += String(mfrc522.uid.uidByte[i]);
			}
			setSerialeCorrente(tmpSerial);

			// Scriviamo nei blocchi di memmoria del badge
			// i parametri passati a questa funzione
			ScriviBlocco(bloccoNome, testoNome);
			ScriviBlocco(bloccoRuolo, testoRuolo);
			ScriviBlocco(bloccoSesso, testoSesso);

			mfrc522.PCD_StopCrypto1();
			mfrc522.PICC_HaltA();

			// Il badge � stato letto correttamente
			return NEW_BADGE_OK;
		}
		else
		{
			mfrc522.PCD_StopCrypto1();
			mfrc522.PICC_HaltA();

			// La lettura del badge non � riuscita
			return NEW_BADGE_ERR;
		}
	}
	else
	{
		mfrc522.PCD_StopCrypto1();
		mfrc522.PICC_HaltA();

		// Siamo in attesa di un badge
		return NEW_BADGE_ATTESA;
	}
}

// Funzione che trova l'id della presenza registrato nel nostro lettore
// Come parametro richiede il seriale del badge
String LettoreRfid::GetIdPresenzaFromSeriale(String paramSeriale)
{
	for (int i = 0; i < MAX_USERS; i++)
	{
		if (strArrayUid[i][0] == paramSeriale)
		{
			return strArrayUid[i][1];
		}
	}
	return "";
}

// Funzione che associa ad un seriale gi� inserito con una entrata, l'id presenza
// del database mySql. Useremo l'id per l'uscita
bool LettoreRfid::SetIdPresenza(String seriale, String idPresenza)
{
	setIdPresenzaCorrente(idPresenza);

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

// Scrive un blocco di memoria in un badge
// block � il numero del blocco da scrivere
// stringa � la stringa che vogliamo memorizzare
bool LettoreRfid::ScriviBlocco(byte block, String stringa)
{
	MFRC522::MIFARE_Key key = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

	// Autenticazione per il blocco della PICC che conterr� il nome
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


// Cancella un seriale dall'array
void LettoreRfid::CancellaSerialeOggi(String seriale)
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

// Svuota l'array delle presenze
void LettoreRfid::AzzeraPresenze()
{
	resetMembers();
	for (byte i = 0; i < MAX_USERS; i++)
	{
		strArrayUid[i][0] = "";
		strArrayUid[i][1] = "";
	}
}

// Imposta i membri temporanei come stringhe vuote
void LettoreRfid::resetMembers()
{
	nomeUser = "";
	ruoloUser = "";
	sessoUser = "";
	serialeCorrente = "";
}

/*********************************************
/ Da qui in poi iniziano i metodi di accesso *
/ ai membri privati.						 *
/*********************************************/

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

bool LettoreRfid::GetBadgeDaRegistrare()
{
	return badgeDaRegistrare;
}

void LettoreRfid::SetBadgeDaRegistrare(bool val)
{
	badgeDaRegistrare = val;
}





