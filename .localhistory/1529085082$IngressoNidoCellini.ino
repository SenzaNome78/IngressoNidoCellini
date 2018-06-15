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
const char ssidSTA[] = "DHouse";			//ssid della rete
const char passwordSTA[] = "dav050315";		//password della rete

WiFiServer webServer(80);	// Usiamo questo server per registrare nuovi bambini o educatori

HTTPClient httpC;				// Usiamo il client http per comunicare le presenze al server
ESP8266WebServer wServer(80);	// Server http principale

// Server http usato per interrompere la registrazione di un nuovo badge
ESP8266WebServer InterrServer(8080);

// Variabili temporali per l'attesa nella scrittura badge 
long tempoAttesaBadgeXScrittura;
const long tempoTotaleAttesaBadgeXScrittura = 8000;

int idPresenza; // id della presenza, temporaneo per registrarlo nell'array della classe LettoreRfid

String msgDiRitorno = ""; // Stringa contenente il messaggio da ritornare al server

// Mandiamo al server il seriale e il ruolo (bambino, educatore..) del badge 
int SendDataToWebServer(String userSerial, String ruolo, bool entrata);

// Funzione associa ad un badge l'utente che ci viene passato dal server
String AttivaModScrittura(String nomeNuovoBadge, String ruoloNuovoBadge, String sessoNuovoBadge);

void setup()
{
	//Serial.begin(9600);   // Prepariamo la seriale
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


	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.print(".");
	}

	Serial.print("\nConnesso con IP: ");
	Serial.println(WiFi.localIP());


	wServer.begin();
	wServer.on("/NewBadge.html", NewBadgeOnIn);
	wServer.on("/RegEntry.html", RegEntryOnIn);

	Serial.println("\nAvviato server http del lettore.");
}

// Event handler per quando viene richiesta la pagina NewBadge.html
// Associa ad un badge un utente registrato sul server
// Legge i parametri dalla request http fatta dal server apache
void NewBadgeOnIn()
{
	// DA CANCELLARE
	Serial.println("In NewBadgeOnIn");

	// I campi nome, ruolo e sesso contengono tutti e tre qualcosa, 
	// avviamo la funzione AttivaModScrittura
	if (wServer.arg("nome") != "" && wServer.arg("ruolo") != "" && wServer.arg("sesso") != "")
	{
		AttivaModScrittura(wServer.arg("nome"), wServer.arg("ruolo"), wServer.arg("sesso"));

		return;
	}
}

// Event handler per quando viene richiesta la pagina RegEntry.html
// Registra l'id della presenza, il cui seriale è stato precedentemente
// salvato nell'oggetto rfid
void RegEntryOnIn()
{
	rfid.SetIdPresenza(wServer.arg(0).toInt(), wServer.arg(1).toInt());

	// DA CANCELLARE
	for (int i = 0; i < 3; i++)
	{
		Serial.print("Seriale: ");
		Serial.println(rfid.strArrayUid[i][0]);
		Serial.print("idPres: ");
		Serial.println(rfid.strArrayUid[i][1]);
	}
}

void loop()
{
	wServer.handleClient();


	//*********** INIZIO LETTURA BADGE ***********************************
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
			idPresenza = SendDataToWebServer(String(rfid.getSeriale()), rfid.getRuoloUser(), true);
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

			if (!SendDataToWebServer(String(rfid.getSeriale()), rfid.getRuoloUser(), false))
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
	String tmpStr = "http://192.168.0.2/NidoCellini/src/php/RegEntry.php?seriale=" + userSerial + "&ruolo=" + ruolo + "&entrata=" + entr;

	httpC.begin(tmpStr); // Apriamo una connessione http verso il server

	httpCode = httpC.GET();

	// httpCode sarà negativo se c'e' stato un errore
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

String AttivaModScrittura(String nomeNuovoBadge, String ruoloNuovoBadge, String sessoNuovoBadge)
{
	bool scritturaBadgeRiuscita = false;
	msgDiRitorno = ""; // Stringa contenente il messaggio da ritornare al server

	//InterrServer.stop();
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
				msgDiRitorno = String("&S=Registrato&seriale=") + rfid.getSeriale();

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
		tempoAttesaBadgeXScrittura -= 500;
		delay(500);


	}

	// Se il tempo per registrare il badge è finito
	if (msgDiRitorno == "")
	{
		msgDiRitorno = "&F=Timeout";
		// Interruzione per scadenza tempo
		delay(50);
		wServer.send(200, "text / plain", msgDiRitorno);
		LcdPrintCentered("Tempo per registrare", 0, true, lcd);
		LcdPrintCentered("il badge esaurito.", 1, true, lcd);
		LcdPrintCentered("Si prega ritentare.", 2, true, lcd);
		LcdPrintCentered("Grazie. ", 3, true, lcd);
		delay(lcdPause);
	}

	tempoAttesaBadgeXScrittura = tempoTotaleAttesaBadgeXScrittura;


	rfid.CancellaSerialeOggi(rfid.getSeriale());


	return msgDiRitorno;
}

// Event handler per quando viene richiesta la pagina Interr.html
// Interrompe la fase di attesa per quando viene registrato un nuovo badge
void InterrOnIn()
{
	Serial.println("In InterrOnIN");
	//Serial.println("In InterrOnIN");
	//wServer.send(200, "text / plain", "InterrOk");

	if (InterrServer.arg("command") == "interr")
	{

		msgDiRitorno = "&F=Stop";
		wServer().send(200, "text / plain", msgDiRitorno);

		tempoAttesaBadgeXScrittura = 0;
		LcdPrintCentered("Registrazione badge", 0, true, lcd);
		LcdPrintCentered("interrotta", 1, true, lcd);
		LcdPrintCentered("dall'utente.", 2, true, lcd);
		LcdPrintCentered(" ", 3, true, lcd);
		delay(lcdPause);

		InterrServer.send(200, "text / plain", "InterrOk");
	}
}