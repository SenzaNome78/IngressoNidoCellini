/*
 * LettoreRfid.h
 *
 *  Created on: 21 mag 2018
 *      Author: David
 */

#include <Arduino.h>
#include "IngressiNidoHelper.h"
#include <MFRC522.h>

const uint8_t RST_PIN = D3;		// Pin reset del lettore rfid
const uint8_t SS_PIN = D8;		// Pin seriale del lettore rfid
const byte MAX_USERS = 100;		// Utenti massimi di questo lettore

class LettoreRfid
{
public:
	LettoreRfid();
	~LettoreRfid();

	bool BadgeRilevato();
	bool ScriviNuovoBadge(String testoNome, String testoRuolo, String testoSesso);


	String getNomeUser();
	String getRuoloUser();
	String getSessoUser();

	bool isNuovaRilevazione();
	void CancellaSerialeOggi(String seriale);

	String getSeriale()
	{
		return seriale;
	}

	bool GetBadgeDaRegistrare();
	
	
	void SetBadgeDaRegistrare(bool val);
	
private:

	bool PulisciBlocco(byte block);

	bool ScriviBlocco(byte block, String stringa);
	String LeggiBlocco(byte block);

	String strArrayUid[MAX_USERS]; //Array che contiene i seriali inseriti finora
	MFRC522 mfrc522;

	String nomeUser;
	void setNomeUser(String nomeUser = "");

	String ruoloUser;
	void setRuoloUser(String ruoloUser = "");

	String sessoUser;
	void setSessoUser(String sessoUser = "");

	String seriale = "";
	void setSeriale(String seriale = "")
	{
		this->seriale = seriale;
	}
	
	bool badgeDaRegistrare = false;


	bool NuovaRilevazione = false;
	void SetNuovaRilevazione(String tmpSerial);

	void resetMembers();
};

