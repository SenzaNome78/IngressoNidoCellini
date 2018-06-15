#include <ESP8266HTTPUpdateServer.h>
#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include "LettoreRfid.h"
#include "IngressiNidoHelper.h"
#include <ESP8266WebServer.h> 
#include <ESP8266HTTPClient.h> 

// Istanza della classe LettoreRfid
LettoreRfid rfid;

// Mandiamo al server il seriale e il ruolo (bambino, educatore..) del badge 
int SendDataToWebServer(String userSerial, String ruolo, bool entrata);

// Funzione che scrive i dati passati dal server su un nuovo badge
bool AttivaModScrittura();

// instanza per gestire l'lcd
LiquidCrystal_I2C lcd(0x27, 20, 4);
// Pausa per il testo dell'lcd
const int lcdPause = 4000;

// Costanti contenenti gli ssid e password per il wifi
const char ssidSTA[] = "DHouse";			//ssid della rete
const char passwordSTA[] = "dav050315";		//password della rete


HTTPClient httpC;			// Usiamo il client http per comunicare le presente al server
WiFiServer webServer(80);	// Usiamo questo server per registrare nuovi bambini o educatori
WiFiClient client;			// client che usiamo per rilevare una connessione proveniente dal server
WiFiClient clientPres;		// client per le presenze

ESP8266HTTPClient hClient;
ESP8266WebServer wServer(80);

String NuovoUtenteHeader;	// stringa che contiene l'header del messagio get in caso di nuovo badge



// Variabili per scrittura badge se uso Delay
long tempoAttesaBadgeXScrittura;
const long tempoTotaleAttesaBadgeXScrittura = 5000; // RITARDO DI 3 SECONDO PER MINUTO

String nome_nuovo_badge;
String ruolo_nuovo_badge;
String sesso_nuovo_badge;

bool modScrittura = false; 

int idPresenza; // id della presenza, temporaneo per registrarlo nell'array della classe LettoreRfid

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

	//httpC.setReuse(true);
	//httpC.setReuse();
	


	Serial.println("\nConnected.");

	Serial.println(WiFi.localIP());

	
	wServer.on("/NewBadge.html", HTTPMethod::HTTP_ANY, NewBadgeOnIn, NewBadgeOnOut);
	wServer.addHandler()
	wServer.begin();
	Serial.println("SERVER BEGIN!!");
	//webServer.begin();
	// **********Init WIFI STATION**************
}

void NewBadgeOnIn()
{
	Serial.println("In NewBadgeOnIn");
	Serial.println(wServer.args());
	for (int i = 0; i < wServer.args(); i++)
	{
		Serial.print(wServer.argName(i));
		Serial.print(": ");
		Serial.println(wServer.arg(i));

	}
	wServer.sendContent("TEST");

}

void NewBadgeOnOut()
{
	Serial.println("In NewBadgeOnOut");
}

void loop()
{
	wServer.handleClient();
	//*********** INIZIO MOD SCRITTURA ***********************************
	// Se abbiamo attivatò la modalità scrittura, ci occupiamo di scrivere il nuovo badge
	// Blocco che rimane in ascolto di un eventuale richiesta per un nuovo 
	// badge dal server web
	//client = webServer.available();

	//if (client)
	//{
	//	Serial.println("\n[Client connected]");

	//	if (client.connected() && modScrittura == false)
	//	{
	//		NuovoUtenteHeader = client.readStringUntil('\r');

	//		nome_nuovo_badge = NuovoUtenteHeader.substring(NuovoUtenteHeader.indexOf("nome=") + 5, NuovoUtenteHeader.indexOf('&'));
	//		nome_nuovo_badge.trim();
	//		ruolo_nuovo_badge = NuovoUtenteHeader.substring(NuovoUtenteHeader.indexOf("ruolo=") + 6, NuovoUtenteHeader.indexOf("ruolo=") + 7);
	//		ruolo_nuovo_badge = ruolo_nuovo_badge.substring(0, 1);
	//		sesso_nuovo_badge = NuovoUtenteHeader.substring(NuovoUtenteHeader.indexOf("sesso=") + 6, NuovoUtenteHeader.indexOf("sesso=") + 7);
	//		sesso_nuovo_badge = sesso_nuovo_badge.substring(0, 1);

	//		if (nome_nuovo_badge != "" && ruolo_nuovo_badge != "" && sesso_nuovo_badge != "")
	//		{
	//			// i campi nuovo nome, ruolo e sesso contengono qualcosa, attiviamo la
	//			// modalità scrittura e usciamo dal loop

	//			modScrittura = true;
	//			AttivaModScrittura();


	//			return;

	//		}
	//	}
	//}
	//*********** FINE MOD SCRITTURA ***********************************

	//*********** INIZIO MOD LETTURA ***********************************
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

			//if (rfid.getSessoUser() == "M")
			//{
			//	tmpSesso = "stato inserito";
			//}
			//else if (rfid.getSessoUser() == "F")
			//{
			//	tmpSesso = "stata inserita";
			//}
			idPresenza = SendDataToWebServer(rfid.getSeriale(), rfid.getRuoloUser(), true);
			if (idPresenza > 0)
			{
				LcdPrintCentered("L'entrata di", 0, true, lcd);
				LcdPrintCentered(rfid.getNomeUser(), 1, true, lcd);
				LcdPrintCentered("e' stata registrata", 2, true, lcd);
				LcdPrintCentered("Buona giornata", 3, true, lcd);
			}
			else
			{
				rfid.CancellaSerialeOggi(rfid.getSeriale());
				LcdPrintCentered("Errore comunicazione", 0, true, lcd);
				LcdPrintCentered("col server. Per fa-", 1, true, lcd);
				LcdPrintCentered("vore contattare", 2, true, lcd);
				LcdPrintCentered("l'amministratore", 3, true, lcd);
				
			}

			delay(lcdPause);
		}
		else // Se il badge era già stato rilevato in questa sessione, visualizza un messaggio ed esce
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
			//LcdPrintCentered(rfid.getNomeUser(), 0, true, lcd);
			//LcdPrintCentered("era " + tmpSesso + ".", 1, true, lcd);
			//LcdPrintCentered("Grazie e", 2, true, lcd);
			//LcdPrintCentered("Buona giornata.", 3, true, lcd);

			if (rfid.getSessoUser() == "M")
			{
				tmpSesso = "stato inserito";
			}
			else if (rfid.getSessoUser() == "F")
			{
				tmpSesso = "stata inserita";
			}
			if (rfid.getRuoloUser() == "B")
			{
				tmpRuolo = "stato inserito";
			}
			else if (rfid.getRuoloUser() == "E")
			{
				tmpRuolo = "stata inserita";
			}


			if (!SendDataToWebServer(rfid.getSeriale(), rfid.getRuoloUser(), false))
			{
				rfid.CancellaSerialeOggi(rfid.getSeriale());
				LcdPrintCentered("Errore comunicazione", 0, true, lcd);
				LcdPrintCentered("col server. Per fa-", 1, true, lcd);
				LcdPrintCentered("vore contattare", 2, true, lcd);
				LcdPrintCentered("l'amministratore", 3, true, lcd);
			}
			else
			{

				rfid.CancellaSerialeOggi(rfid.getSeriale());
				LcdPrintCentered("Ciao", 0, true, lcd);
				LcdPrintCentered(rfid.getNomeUser(), 1, true, lcd);
				LcdPrintCentered("A presto e", 2, true, lcd);
				LcdPrintCentered("buona giornata!", 3, true, lcd);
			}


			delay(lcdPause);
		}
		//*********** FINE MOD LETTURA ***********************************
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
Invia il seriale al web server che si occuperà di registrarne l'entrata nel database
userSerial: il seriale da inviare
ruolo: il ruolo dell'utente (B: Bambino, E:Educatore)
entrata: indica se si tratta di un entrata (true) o un'uscita (false)
La funzione restituisce l'id della presenze registrata o '0' se non è riuscito a registrarla
*/
int SendDataToWebServer(String userSerial, String ruolo, bool entrata)
{
	int httpCode; // Ci serve per verificare che l'invio sia andato a buon fine
	userSerial.trim(); // Eliminiamo eventuali spazi esterni della stringa

	// *************** Usare GET ********************************************************
	String entr = String(entrata);
	Serial.println(entr);
	String tmpStr = "http://192.168.0.2:80/NidoCellini/src/php/RegEntry.php?seriale=" + userSerial + "&ruolo=" + ruolo + "&entrata=" + entr;

	Serial.println(tmpStr);
	httpC.begin(tmpStr); // Apriamo una connessione http verso il server

	httpCode = httpC.GET();
	// httpCode sarà negativo se c'e' stato un errore
	if (httpCode > 0)
	{
		// Se il server
		if (httpCode == HTTP_CODE_OK)
		{
			// Se tutto è andato bene restituiamo true
			Serial.print("tmpStr: ");
			Serial.println(tmpStr);
			Serial.print("httpCode: ");
			Serial.println(httpCode);
			//httpC.setReused();

			//String header;
			//
			//clientPres = webServer.available();
			//while (!clientPres.connected())
			//{
			//	Serial.println("InWhile");
			//	header = clientPres.readStringUntil('\r');
			//}
			//Serial.println(header);

			//clientPres.flush();
			//clientPres.stop();

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
					Serial.println(rfid.getSeriale());
					msgDiRitorno = String("&S=Registrato&seriale=") + rfid.getSeriale();
					client.print(msgDiRitorno);

					delay(200);
					client.flush();
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
					client.flush();
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
					client.print(msgDiRitorno); // Interrotto dall'utente
					delay(50);
					client.flush();
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
			}

		}

		// Se il tempo per registrare il badge è finito
		if (msgDiRitorno == "")
		{
			msgDiRitorno = "&F=Timeout";
			client.print(msgDiRitorno); // Interruzione per scadenza tempo
			delay(50);
			client.flush();
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
	client.flush();
	client.stop();

	clientInterr.flush();
	clientInterr.stop();

}