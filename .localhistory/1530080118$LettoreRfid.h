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

	bool BadgeRilevato();
	uint8_t ScriviNuovoBadge(String testoNome, String testoRuolo, String testoSesso);

	bool SetIdPresenza(String seriale, String idPresenza);

	String getNomeUser();
	String getRuoloUser();
	String getSessoUser();

	bool isNuovaRilevazione();

	void CancellaSerialeOggi(String seriale);

	String getSerialeCorrente()
	{
		return serialeCorrente;
	}

	bool GetBadgeDaRegistrare();
	
	
	void SetBadgeDaRegistrare(bool val);
	

	// Funzione che trova l'id della presenza registrato nel nostro lettore
	// Come parametro richiede il seriale del badge
	String GetIdPresenzaFromSeriale(String paramSeriale);

private:
	
	String strArrayUid[MAX_USERS][2]; //Array che contiene i seriali inseriti finora
	bool PulisciBlocco(byte block);

	bool ScriviBlocco(byte block, String stringa);

	String LeggiBlocco(byte block);
		
	MFRC522 mfrc522; // Istanza del nostro lettore RFID

	String nomeUser;
	void setNomeUser(String nomeUser = "");

	String ruoloUser;
	void setRuoloUser(String ruoloUser = "");

	String sessoUser;
	void setSessoUser(String sessoUser = "");

	String serialeCorrente;

	void setSerialeCorrente(String seriale)
	{
		this->serialeCorrente = seriale;
	}

	String idPresenzaCorrente;
	void setIdPresenzaCorrente(String paramIdPresenzaCorrente)
	{
		this->idPresenzaCorrente = paramIdPresenzaCorrente;
	}
	
	bool badgeDaRegistrare = false;


	bool NuovaRilevazione = false;
	void SetNuovaRilevazione(String tmpSerial);

	void resetMembers();
};


