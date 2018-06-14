#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include "LettoreRfid.h"
#include "IngressiNidoHelper.h"

LettoreRfid rfid;

bool SendDataToWebServer(String userSerial, String ruolo);

//String prepareHtmlPage();

bool AttivaModScrittura();

// instanza per gestire l'lcd
LiquidCrystal_I2C lcd(0x27, 20, 4);
// Pausa per il testo dell'lcd
const int lcdPause = 4000;

// variabili contenenti gli ssid e password per il wifi
const char ssidSTA[] = "DHouse";			//ssid station
const char passwordSTA[] = "dav050315";		//password station
const char ssidAP[] = "NidoCellini";		//ssid ap
const char passwordAP[] = "cellininido";	//password ap

HTTPClient httpC;
WiFiServer webServer(80); // Usiamo questo server per registrare nuovi bambini o educatori
WiFiClient client;
String NuovoUtenteHeader;


// Variabili per scrittura badge se uso millis()
unsigned long prevMillis = 0;
unsigned long timeAttesaBadge = 200;
unsigned long currMillis = 0;

// Variabili per scrittura badge se uso Delay
long tempoAttesaBadgeXScrittura;
const long tempoTotaleAttesaBadgeXScrittura = 8000; // RITARDO DI 3 SECONDO PER MINUTO

String nome_nuovo_badge;
String ruolo_nuovo_badge;
String sesso_nuovo_badge;


bool modScrittura = false;


void setup()
{
	//Serial.begin(9600);   // Prepariamo la seriale
	Serial.begin(115200);   // Prepariamo la seriale

	// ******Init Variabili tempo*******

	// ******Init Variabili tempo*******

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
	Serial.print("Connecting to ");
	Serial.println(ssidSTA);

	WiFi.mode(WiFiMode::WIFI_STA);

	WiFi.begin(ssidSTA, passwordSTA);

	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.print(".");
	}
	Serial.println("\nConnected.");

	Serial.println(WiFi.localIP());
	webServer.begin();
	// **********Init WIFI STATION**************
}

void loop()
{
	unsigned long curMillis = millis();
	//*********** INIZIO MOD SCRITTURA ***********************************
	// Se abbiamo attivatò la modalità scrittura, ci occupiamo di scrivere il nuovo badge


	// Blocco che rimane in ascolto di un eventuale richiesta per un nuovo 
	// badge dal server web

	client = webServer.available();

	if (client)
	{
		Serial.println("\n[Client connected]");
		if (client.connected() && modScrittura == false)
		{
			NuovoUtenteHeader = client.readStringUntil('\r');

			nome_nuovo_badge = NuovoUtenteHeader.substring(NuovoUtenteHeader.indexOf("nome=") + 5, NuovoUtenteHeader.indexOf('&'));
			nome_nuovo_badge.trim();
			ruolo_nuovo_badge = NuovoUtenteHeader.substring(NuovoUtenteHeader.indexOf("ruolo=") + 6, NuovoUtenteHeader.indexOf("ruolo=") + 7);
			ruolo_nuovo_badge = ruolo_nuovo_badge.substring(0, 1);
			sesso_nuovo_badge = NuovoUtenteHeader.substring(NuovoUtenteHeader.indexOf("sesso=") + 6, NuovoUtenteHeader.indexOf("sesso=") + 7);
			sesso_nuovo_badge = sesso_nuovo_badge.substring(0, 1);

			if (nome_nuovo_badge != "" && ruolo_nuovo_badge != "" && sesso_nuovo_badge != "")
			{
				// i campi nuovo nome, ruolo e sesso contengono qualcosa, attiviamo la
				// modalità scrittura e usciamo dal loop

				modScrittura = true;
				AttivaModScrittura();

				//if (AttivaModScrittura())
				//{
				//	client.println("&successo=S");
				//}
				//else
				//{
				//	client.println("&successo=N");
				//}
				return;

			}
		}
	}
	//*********** FINE MOD SCRITTURA ***********************************


	if (rfid.BadgeRilevato()) // Un badge è stato avvicinato al lettore
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

			if (rfid.getSessoUser() == "M")
			{
				tmpSesso = "stato inserito";
			}
			else if (rfid.getSessoUser() == "F")
			{
				tmpSesso = "stata inserita";
			}

			if (!SendDataToWebServer(rfid.getSeriale(), rfid.getRuoloUser()))
			{
				rfid.CancellaSerialeOggi(rfid.getSeriale());
				LcdPrintCentered("Errore comunicazione", 0, true, lcd);
				LcdPrintCentered("col server. Per fa-", 1, true, lcd);
				LcdPrintCentered("vore contattare", 2, true, lcd);
				LcdPrintCentered("l'amministratore", 3, true, lcd);
			}
			else
			{
				LcdPrintCentered(rfid.getNomeUser(), 0, true, lcd);
				LcdPrintCentered("e' " + tmpSesso + ".", 1, true, lcd);
				LcdPrintCentered("Grazie e", 2, true, lcd);
				LcdPrintCentered("Buona giornata.", 3, true, lcd);
			}


			delay(lcdPause);
			// DA ELIMINARE
			/*Serial.print("Nome: ");
			Serial.println(rfid.getNomeUser());
			Serial.print("Ruolo: ");
			Serial.println(rfid.getRuoloUser());
			Serial.print("Sesso: ");
			Serial.println(rfid.getSessoUser());*/
			// /DA ELIMINARE
		}
		else // Se il badge era già stato rilevato in questa sessione, visualizza un messaggio ed esce
		{
			if (rfid.getSessoUser() == "M")
			{
				tmpSesso = "stato inserito";
			}
			else if (rfid.getSessoUser() == "F")
			{
				tmpSesso = "stata inserita";
			}
			LcdPrintCentered(rfid.getNomeUser(), 0, true, lcd);
			LcdPrintCentered("era " + tmpSesso + ".", 1, true, lcd);
			LcdPrintCentered("Grazie e", 2, true, lcd);
			LcdPrintCentered("Buona giornata.", 3, true, lcd);

			delay(lcdPause);

			// DA ELIMINARE
			/*Serial.println("Già inserito!");
			SendDataToWebServer(rfid.getSeriale());
			Serial.print("Nome: ");
			Serial.println(rfid.getNomeUser());
			Serial.print("Ruolo: ");
			Serial.println(rfid.getRuoloUser());
			Serial.print("Sesso: ");
			Serial.println(rfid.getSessoUser());*/

			// /DA ELIMINARE

		}
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
Invia il seriale del bambino al web server che si occuperà di registrarne l'entrata nel database
userSerial: il seriale da inviare
la funzione ritorna true se l'invio è andato a buon fine, false se c'e' stato qualche problema
*/
bool SendDataToWebServer(String userSerial, String ruolo)
{
	int httpCode; // Ci serve per verificare che l'invio sia andato a buon fine
	userSerial.trim(); // Eliminiamo eventuali spazi esterni della stringa

	
	// *************** Usare GET ********************************************************
	String tmpStr = "http://192.168.0.2:80/NidoCellini/src/php/RegEntry.php?seriale="
	+ userSerial
	+"&ruolo="
		+ ruolo;
	Serial.println(tmpStr);
	httpC.begin(tmpStr);
	//Serial.print("[HTTP] GET...\n");
	// start connection and send HTTP header
	httpCode = httpC.GET();
	// httpCode will be negative on error
	if (httpCode > 0)
	{
		// HTTP header has been send and Server response header has been handled
		//		Serial.printf("[HTTP] GET... code: %d\n", httpCode);
		// file found at server
		if (httpCode == HTTP_CODE_OK)
		{
			String payload = httpC.getString();
			//Serial.println(payload);
			httpC.end();
			return true;
		}
		return false;
	}
	else
	{
		Serial.printf("[HTTP] GET... failed, error: %s\n",
					  httpC.errorToString(httpCode).c_str());
		httpC.end();
		return false;
	}

}


// prepare a web page to be send to a client (web browser)
//String prepareHtmlPage()
//{
//	String htmlPage = String("HTTP/1.1 200 OK\r\n")
//		+ "Content-Type: text/html\r\n" + "Connection: close\r\n"
//		+ // the connection will be closed after completion of the response
//		  //            "Refresh: 5\r\n" +  // refresh the page automatically every 5 sec
//		"\r\n" + "<!DOCTYPE HTML>" + "<link rel=\"icon\" href=\"data:,\">"
//		+ "<html>Analog input:  " + String(analogRead(A0)) + "</html>"
//		+ "\n";
//	return htmlPage;
//}

bool AttivaModScrittura()
{
	WiFiClient clientInterr;
	String stopParam = "";
	String stopHeader = "";
	
	bool scritturaBadgeRiuscita = false;
	String msgDiRitorno = ""; // Stringa contenente il messaggio da ritornare al server
		 
	if (modScrittura)
	{

		Serial.println("MODALITA' scrittura Attivata");

		while (tempoAttesaBadgeXScrittura > 0)
		{
			if (rfid.BadgeRilevato())
			{
				PlayBuzzer();
				scritturaBadgeRiuscita = rfid.ScriviNuovoBadge(nome_nuovo_badge, ruolo_nuovo_badge, sesso_nuovo_badge);//
				
				if (scritturaBadgeRiuscita)
				{
					msgDiRitorno = "&S = Registrato";
					client.print(msgDiRitorno);
					delay(50);
					client.stop();

					LcdPrintCentered("Badge registrato.", 0, true, lcd);
					LcdPrintCentered("Allontanarlo dal", 1, true, lcd);
					LcdPrintCentered("lettore. Grazie e", 2, true, lcd);
					LcdPrintCentered("buona giornata. ", 3, true, lcd);
					delay(lcdPause);
				}
				else
				{
					msgDiRitorno = "&F=ScriviNuovoBadge";
					client.print(msgDiRitorno);
					delay(50);
					client.stop();

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
			tempoAttesaBadgeXScrittura -= 500;
			delay(500);

			clientInterr = webServer.available();
			if (clientInterr.available())
			{
				stopHeader = clientInterr.readStringUntil('\r');
				if (stopHeader.indexOf("command=stop") != -1)
				{
					delay(50);
					clientInterr.stop();
					msgDiRitorno = "&F=Stop";
					client.print(msgDiRitorno); // Interruzione per scadenza tempo
					delay(50);
					client.stop();
					LcdPrintCentered("Registrazione badge", 0, true, lcd);
					LcdPrintCentered("interrotta", 1, true, lcd);
					LcdPrintCentered("dall'utente.", 2, true, lcd);
					LcdPrintCentered(" ", 3, true, lcd);
					delay(lcdPause);
					
					break;
				}
				else
					Serial.println("NON CANCELLARE!");

				delay(10);
				//clientInterr.stop();
				//tempoAttesaBadgeXScrittura = 0;
			}

		}

		// Se il tempo per registrare il badge è finito
		if (msgDiRitorno == "")
		{
			msgDiRitorno = "&F=Timeout";
			client.print(msgDiRitorno); // Interruzione per scadenza tempo
			delay(50);
			client.stop();

			LcdPrintCentered("Tempo per registrare", 0, true, lcd);
			LcdPrintCentered("il badge esaurito.", 1, true, lcd);
			LcdPrintCentered("Si prega ritentare.", 2, true, lcd);
			LcdPrintCentered("Grazie. ", 3, true, lcd);
			delay(lcdPause);
		}

		tempoAttesaBadgeXScrittura = tempoTotaleAttesaBadgeXScrittura;
		nome_nuovo_badge = "";
		ruolo_nuovo_badge = "";
		sesso_nuovo_badge = "";

		rfid.CancellaSerialeOggi(rfid.getSeriale());

		modScrittura = false;

		return scritturaBadgeRiuscita;
	}
}