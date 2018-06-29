/*
 * LettoreRfid.h
 *
 *  Created on: 21 mag 2018
 *      Author: David
 */

#include <Arduino.h>
#include "IngressiNidoHelper.h"
#include <MFRC522.h>

const uint8_t RST_PIN = D3;	 // Pin reset del lettore rfid
const uint8_t SS_PIN = D8;	 // Pin seriale del lettore rfid
const byte MAX_USERS = 50;	 // Utenti massimi di questo lettore
const byte bloccoNome = 12;  // Blocco di memoria contenente il nome
const byte bloccoRuolo = 13; // Blocco di memoria contenente il ruolo (un solo byte: B o E)
const byte bloccoSesso = 14; // Blocco di memoria contenente il sesso (un solo byte: F o M)

class LettoreRfid
{
public:
	LettoreRfid();
	~LettoreRfid();

	const static uint8_t NEW_BADGE_OK = 1;
	const static uint8_t NEW_BADGE_ERR = 2;
	const static uint8_t NEW_BADGE_ATTESA = 3;

	// Metodi di accesso publici
	String getNomeUser();
	String getRuoloUser();
	String getSessoUser();
	bool GetBadgeDaRegistrare();
	void SetBadgeDaRegistrare(bool val);
	bool isNuovaRilevazione();
	String getSerialeCorrente()
	{
		return serialeCorrente;
	}
	// FINE metodi di accesso publici


	// Funzione che associa ad un seriale già inserito con una entrata, l'id presenza
	// del database mySql. Useremo l'id per l'uscita
	bool SetIdPresenza(String seriale, String idPresenza);

	// Cancella un seriale dall'array
	void CancellaSerialeOggi(String seriale);

	// Rileva se un badge è stato avvicinato al lettore.
	bool BadgeRilevato();

	//Scriviamo un nuovo badge con il nome e il ruolo inviatoci dal server web
	uint8_t ScriviNuovoBadge(String testoNome, String testoRuolo, String testoSesso);

	// Funzione che trova l'id della presenza registrato nel nostro lettore
	// Come parametro richiede il seriale del badge
	String GetIdPresenzaFromSeriale(String paramSeriale);

	// Svuota l'array delle presenze
	void AzzeraPresenze();
private:
	// Inizio membri e loro metodi di accesso (privati)

	// Array bidimensionale che contiene i seriali inseriti in questa sessione
	// La prima colonna contiene i seriali mentre 
	// la seconda contiene gli idPresenza dei mySQL
	String strArrayUid[MAX_USERS][2];
	String serialeCorrente;

	bool badgeDaRegistrare = false;
	bool NuovaRilevazione = false;

	MFRC522 mfrc522; // Istanza del nostro lettore RFID
	String nomeUser;
	void setNomeUser(String nomeUser = "");

	String ruoloUser;
	void setRuoloUser(String ruoloUser = "");

	String sessoUser;
	void setSessoUser(String sessoUser = "");

	void setSerialeCorrente(String seriale)
	{
		this->serialeCorrente = seriale;
	}

	String idPresenzaCorrente;

	void setIdPresenzaCorrente(String paramIdPresenzaCorrente)
	{
		this->idPresenzaCorrente = paramIdPresenzaCorrente;
	}
	// FINE membri e loro metodi di accesso (privati)

	// Scrive un blocco di memoria in un badge
	// block è il numero del blocco da scrivere
	// stringa è la stringa che vogliamo memorizzare
	bool ScriviBlocco(byte block, String stringa);

	// Legge un blocco di memoria da un badge
	// block è il numero del blocco da leggere
	// restituisce la stringa contenuta nel blocco
	String LeggiBlocco(byte block);




	// Registra una nuova presenza se non già inserita
	// NuovaRilevazione viene impostata true o false se
	// si tratta di un nuovo inserimento o meno
	void SetNuovaRilevazione(String tmpSerial);

	// Imposta i membri temporanei come stringhe vuote
	void resetMembers();

};


