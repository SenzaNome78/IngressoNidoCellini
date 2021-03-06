#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include "LettoreRfid.h"
#include "IngressiNidoHelper.h"
#include <ESP8266WebServer.h> 

LettoreRfid rfid; // Istanza della classe LettoreRfid
LiquidCrystal_I2C lcd(0x27, 20, 4); // instanza per gestire l'lcd

const int lcdPause = 4000; // Pausa per il testo dell'lcd

// Costanti contenenti gli ssid e password per il wifi
const char ssidSTA[] = "NidoCellini";			//ssid della rete
const char passwordSTA[] = "NidoCelliniPass";		//password della rete

WiFiServer webServer(80);	// Usiamo questo server per registrare nuovi bambini o educatori

HTTPClient httpC;				// Usiamo il client http per comunicare le presenze al server
ESP8266WebServer wServer(80);	// Server http principale

// Server http usato per interrompere la registrazione di un nuovo badge
ESP8266WebServer InterrServer(8080);



int idPresenza; // id della presenza, temporaneo per registrarlo nell'array della classe LettoreRfid

String msgDiRitorno = ""; // Stringa contenente il messaggio da ritornare al server

// Mandiamo al server il seriale e il ruolo (bambino, educatore..) del badge 
int SendDataToWebServer(String userSerial,
						String ruolo,
						bool entrata,
						unsigned long idPresenzaUscita = 0);

// Funzione associa ad un badge l'utente che ci viene passato dal server
String AttivaModScrittura(String nomeNuovoBadge, String ruoloNuovoBadge, String sessoNuovoBadge);


// Variabili temporali per l'attesa nella scrittura badge 
long tempoAttesaBadgeXScrittura = 0;
const long tempoTotaleAttesaBadgeXScrittura = 5000;


 void setup()
{
	Serial.begin(115200);   // Prepariamo la seriale

	tempoAttesaBadgeXScrittura = tempoTotaleAttesaBadgeXScrittura;

	// **********Init RFID**************
	SPI.begin();      // Init SPI bus
	// **********Init RFID**************

	// **********Init LCD**************
	lcd.init();  //inizializiamo l'lcd
	lcd.backlight();  // Accendiamo la luce di sfondo dell'lcd
	// **********Init LCD**************

	// **********Init WIFI STATION**************
	Serial.println("");
	Serial.print("Connessione a ");
	Serial.println(ssidSTA);

	WiFi.mode(WiFiMode::WIFI_STA);

	WiFi.begin(ssidSTA, passwordSTA);
	// **********Init WIFI STATION**************


	// Tentativo di connettersi alla rete Wi-Fi
	// Dopo un tempo stabilito visualizza un msg di errore
	tempoAttesaBadgeXScrittura = tempoTotaleAttesaBadgeXScrittura;
	while (WiFi.status() != WL_CONNECTED)
	{
		if (tempoAttesaBadgeXScrittura < 1)
		{

			LcdPrintCentered("Errore collegamento", 0, true, lcd);
			LcdPrintCentered("alla rete WiFi.", 1, true, lcd);
			LcdPrintCentered("Contattare", 2, true, lcd);
			LcdPrintCentered("l'amministratore", 3, true, lcd);
			continue;
		} 
		LcdPrintCentered("In attesa del", 0, true, lcd);
		LcdPrintCentered("collegamento", 1, true, lcd);
		LcdPrintCentered(" col server.", 2, true, lcd);
		LcdPrintCentered("Attendere prego.", 3, true, lcd);
		tempoAttesaBadgeXScrittura -= 500;

		delay(500);
		Serial.print(".");
	}
	tempoAttesaBadgeXScrittura = 0;

	// Siamo connessi alla rete Wi-Fi
	Serial.print("\nConnesso con IP: ");
	Serial.println(WiFi.localIP());


	// Attiviamo il server http e registriamo i gestori degli eventi
	wServer.begin();
	wServer.on("/NewBadge.html", NewBadgeOnIn);
	wServer.on("/RegEntry.html", RegEntryOnIn);
	wServer.on("/Diagnostica.html", DiagnosticaOnIn);

	Serial.println("\nAvviato server HTTP del lettore.");
}

// Event handler per quando viene richiesta la pagina NewBadge.html
// Associa ad un badge un utente registrato sul server
// Legge i parametri dalla request http fatta dal server apache
void NewBadgeOnIn()
{
	// I campi nome, ruolo e sesso contengono tutti e tre qualcosa, 
	// avviamo la funzione AttivaModScrittura
	if (wServer.arg("nome") != "" && wServer.arg("ruolo") != "" && wServer.arg("sesso") != "")
	{
		AttivaModScrittura(wServer.arg("nome"), wServer.arg("ruolo"), wServer.arg("sesso"));

		return;
	}
}

// Event handler per quando viene richiesta la pagina RegEntry.html
// Registra l'id della presenza, il cui seriale � stato precedentemente
// salvato nell'oggetto rfid
void RegEntryOnIn()
{
	rfid.SetIdPresenza(wServer.arg(0).toInt(), wServer.arg(1).toInt());
}

// Event handler per quando viene richiesta la pagina Diagnostica.html
// Lo usiamo quando dobbiamo eseguire dei comandi di diagnostica:
// Reset del lettore rfid (dopo un upload fare un power cycle per 
// far funzionare il reset. Problema noto dell'Esp8266)
void DiagnosticaOnIn()
{
	ResetLettore();
}

void loop()
{
	wServer.handleClient(); // Se il server sta contattando il lettore, gestisce gli eventi collegati

	//*********** INIZIO LETTURA BADGE ***********************************
	if (rfid.BadgeRilevato()) // Un badge � stato avvicinato al lettore
	{
		if (rfid.GetBadgeDaRegistrare())
		{
			LcdPrintCentered("Il badge non e'", 0, true, lcd);
			LcdPrintCentered("stato registrato.", 1, true, lcd);
			LcdPrintCentered("Per favore contatti", 2, true, lcd);
			LcdPrintCentered("l'amministratore.", 3, true, lcd);

			PlayBuzzer(); // Riproduce un suono

			delay(lcdPause);
			return;
		}

		String tmpSesso = "";
		String tmpRuolo = "";
		if (rfid.isNuovaRilevazione()) // NUOVA entrata rilevata
		{

			PlayBuzzer(); // Riproduce un suono

			//if (rfid.getSessoUser() == "M")
			//{
			//	tmpSesso = "stato inserito";
			//}
			//else if (rfid.getSessoUser() == "F")
			//{
			//	tmpSesso = "stata inserita";
			//}
			idPresenza = SendDataToWebServer(String(rfid.getSerialeCorrente()), rfid.getRuoloUser(), true, 0);
			if (idPresenza > 0)
			{
				LcdPrintCentered("L'entrata di", 0, true, lcd);
				LcdPrintCentered(rfid.getNomeUser(), 1, true, lcd);
				LcdPrintCentered("e' stata registrata", 2, true, lcd);
				LcdPrintCentered("Buona giornata", 3, true, lcd);
			}
			else
			{
				rfid.CancellaSerialeOggi(rfid.getSerialeCorrente());
				LcdPrintCentered("Errore comunicazione", 0, true, lcd);
				LcdPrintCentered("col server. Per fa-", 1, true, lcd);
				LcdPrintCentered("vore contattare", 2, true, lcd);
				LcdPrintCentered("l'amministratore", 3, true, lcd);

			}

			delay(lcdPause);
		}
		else
		{
			PlayBuzzer(); // Riproduce un suono

			// Stiamo registrando un'uscita, quindi togliamo il seriale
			// dal nostro rfid, non prima di avere inviato il tutto
			// al server per segnare l'uscita stessa.
			if (SendDataToWebServer(String(rfid.getSerialeCorrente()),
									rfid.getRuoloUser(),
									false,
									rfid.GetIdPresenzaFromSeriale(rfid.getSerialeCorrente())))
			{

				rfid.CancellaSerialeOggi(rfid.getSerialeCorrente());

				LcdPrintCentered("Ciao", 0, true, lcd);
				LcdPrintCentered(rfid.getNomeUser(), 1, true, lcd);
				LcdPrintCentered("A presto e", 2, true, lcd);
				LcdPrintCentered("buona giornata!", 3, true, lcd);
			}
			else
			{
				rfid.CancellaSerialeOggi(rfid.getSerialeCorrente());
				LcdPrintCentered("Errore comunicazione", 0, true, lcd);
				LcdPrintCentered("col server. Per fa-", 1, true, lcd);
				LcdPrintCentered("vore contattare", 2, true, lcd);
				LcdPrintCentered("l'amministratore", 3, true, lcd);
			}

			delay(lcdPause);
		}
		//*********** FINE LETTURA BADGE ***********************************
	}
	else // Se nessun badge viene avvicinato visualizza un messaggio ed esci dal loop
	{
		LcdPrintCentered("Buongiorno!", 0, true, lcd);
		LcdPrintCentered("Per favore", 1, true, lcd);
		LcdPrintCentered("Avvicini il badge", 2, true, lcd);
		LcdPrintCentered("Grazie!", 3, true, lcd);
		return;
	}
}

/*
Invia il seriale al web server che si occuper� di registrarne l'entrata nel database
userSerial: il seriale da inviare
ruolo: il ruolo dell'utente (B: Bambino, E:Educatore)
entrata: indica se si tratta di un entrata (true) o un'uscita (false)
La funzione restituisce l'id della presenze registrata o '0' se non � riuscito a registrarla
*/
int SendDataToWebServer(String userSerial,
						String ruolo,
						bool entrata,
						unsigned long idPresenzaUscita)
{
	int httpCode; // Ci serve per verificare che l'invio sia andato a buon fine
	userSerial.trim(); // Eliminiamo eventuali spazi esterni della stringa

	// *************** Usare GET ********************************************************
	String entr;
	if (entrata == true)
	{
		entr = "vero";
	}
	else
	{
		entr = "falso";
	}
	String urlToServer = "";



	if (entrata)
	{
		urlToServer = "http://192.168.0.2/NidoCellini/src/php/RegEntry.php?seriale="
			+ userSerial
			+ "&ruolo=" + ruolo
			+ "&entrata=" + entr
			+ "&idPresenzaUscita=" + idPresenzaUscita;
	}
	else // Uscita
	{
		urlToServer = "http://192.168.0.2/NidoCellini/src/php/RegEntry.php?seriale="
			+ userSerial
			+ "&ruolo=" + ruolo
			+ "&entrata=" + entr
			+ "&idPresenzaUscita=" + idPresenzaUscita;
	}

	httpC.begin(urlToServer); // Apriamo una connessione http verso il server

	httpCode = httpC.GET(); // Inviamo la richiesta GET, usando i suoi parametri per registrare la presenza

	// httpCode sar� negativo se c'e' stato un errore
	if (httpCode > 0)
	{
		// Se il server
		if (httpCode == HTTP_CODE_OK)
		{
			httpC.end();
			
			return true;
		}
		// Altrimenti false
		httpC.end();
		return 0;
	}
	else
	{
		// Se c'e' stato un errore lo stampiamo sulla seriale
		Serial.printf("[HTTP] GET... fallito. Errore: %s\n",
					  httpC.errorToString(httpCode).c_str());
		httpC.end();
		return 0;
	}
	httpC.end();
}


// Proviamo a scrivere un nuovo badge, entro il tempo tempoTotaleAttesaBadgeXScrittura
// impostato nella sezione globale
String AttivaModScrittura(String nomeNuovoBadge, String ruoloNuovoBadge, String sessoNuovoBadge)
{
	bool scritturaBadgeRiuscita = false;
	msgDiRitorno = ""; // Stringa contenente il messaggio da ritornare al server

	InterrServer.begin();
	InterrServer.on("/Interr.html", InterrOnIn);

	while (tempoAttesaBadgeXScrittura > 0)
	{
		InterrServer.handleClient();
		if (rfid.BadgeRilevato())
		{
			PlayBuzzer();
			scritturaBadgeRiuscita = rfid.ScriviNuovoBadge(nomeNuovoBadge, ruoloNuovoBadge, sessoNuovoBadge);//

			if (scritturaBadgeRiuscita)
			{
				msgDiRitorno = String("&S=Registrato&seriale=") + rfid.getSerialeCorrente();

				delay(50);
				wServer.send(200, "text / plain", msgDiRitorno);
				LcdPrintCentered("Badge registrato.", 0, true, lcd);
				LcdPrintCentered("Allontanarlo dal", 1, true, lcd);
				LcdPrintCentered("lettore. Grazie e", 2, true, lcd);
				LcdPrintCentered("buona giornata. ", 3, true, lcd);
				delay(lcdPause);
			}
			else
			{
				msgDiRitorno = "&F=ScriviNuovoBadge";
				delay(50);
				wServer.send(200, "text / plain", msgDiRitorno);
				LcdPrintCentered("Errore registrazione", 0, true, lcd);
				LcdPrintCentered("del badge.", 1, true, lcd);
				LcdPrintCentered("Contattare", 2, true, lcd);
				LcdPrintCentered("l'amministratore.", 3, true, lcd);
				delay(lcdPause);
			}
			break;
		}

		LcdPrintCentered("Avvicinare il badge", 0, true, lcd);
		LcdPrintCentered("da registrare", 1, true, lcd);
		LcdPrintCentered("entro un minuto.", 2, true, lcd);
		LcdPrintCentered("Grazie.", 3, true, lcd);
		tempoAttesaBadgeXScrittura -= 200;
		delay(200);
	}

	// Se il tempo per registrare il badge � finito
	if (msgDiRitorno == "")
	{
		msgDiRitorno = "&F=Timeout";
		delay(50);
		wServer.send(200, "text / plain", msgDiRitorno);
		LcdPrintCentered("Tempo per registrare", 0, true, lcd);
		LcdPrintCentered("il badge esaurito.", 1, true, lcd);
		LcdPrintCentered("Si prega ritentare.", 2, true, lcd);
		LcdPrintCentered("Grazie. ", 3, true, lcd);
		delay(lcdPause);
	}

	tempoAttesaBadgeXScrittura = tempoTotaleAttesaBadgeXScrittura;

	rfid.CancellaSerialeOggi(rfid.getSerialeCorrente());

	return msgDiRitorno;
}


// Event handler per quando viene richiesta la pagina Interr.html
// Interrompe la fase di attesa per quando viene registrato un nuovo badge
void InterrOnIn()
{
	if (InterrServer.arg("command") == "interr")
	{
		// Inviamo questo messaggio al server per far visualizzare
		// il messaggio appropriato al browser web
		msgDiRitorno = "&F=Stop";

		// Impostando questa variabile a zero la funzione AttivaModScrittura
		// si fermer�
		tempoAttesaBadgeXScrittura = 0;

		InterrServer.send(200, "text / plain", "InterrOk");
		wServer.send(200, "text / plain", "InterrWOk");

		LcdPrintCentered("Registrazione badge", 0, true, lcd);
		LcdPrintCentered("interrotta", 1, true, lcd);
		LcdPrintCentered("dall'utente.", 2, true, lcd);
		LcdPrintCentered(" ", 3, true, lcd);
		delay(lcdPause);
	}
}