#include "LedControl.h"
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <DNSServer.h>
#include <TimeLib.h> 
#include <WiFiUdp.h>
#include <FS.h>

const char* project = "Mini_WordClock";

#define DIN D5
#define CLK D6
#define CS D7

const unsigned int localPort = 2390;
IPAddress timeServerIP;
const char* ntpServerName = "de.pool.ntp.org";
WiFiUDP udp;
WiFiManager wifiManager;

// Optionen
char timeZone[2] = "1"; // Central European Time
// Interne Hilfs-Variablen
time_t prevDisplay = 0;

LedControl matrix = LedControl(DIN, CLK, CS, 1);

void setup()
{
	matrix.shutdown(0, false);
	matrix.setIntensity(0, 7);
	matrix.clearDisplay(0);
	SetWords("Boot");

	Serial.begin(115200);
	Serial.println();
	Serial.println();

	//reset settings - for testing
	//wifiManager.resetSettings();

	wifiManager.setMinimumSignalQuality(1);

	if (!wifiManager.autoConnect(project))
	{
		Serial.println("failed to connect and hit timeout");
		delay(3000);
		//reset and try again, or maybe put it to deep sleep
		ESP.reset();
		delay(5000);
	}

	//if you get here you have connected to the WiFi
	Serial.println("connected...yeey :)");


	Serial.println("local ip");
	Serial.println(WiFi.localIP());
	Serial.println(WiFi.gatewayIP());
	Serial.println(WiFi.subnetMask());

	Serial.println("Starting UDP");
	udp.begin(localPort);
	Serial.print("Local port: ");
	Serial.println(udp.localPort());

	delay(100);
}

void loop()
{
	if (timeStatus() != timeNotSet)
	{
		//Test Time
		//setTime(15, 20, 00, 01, 01, 2012);
		if (now() != prevDisplay)
		{
			prevDisplay = now();

			SetDisplayTime();

		}
	}
	else
	{
		WiFi.hostByName(ntpServerName, timeServerIP);
		Serial.println("waiting for sync");
		setSyncProvider(getNtpTime);	
	}
}

int GetSummOrWinterHour()
{
	bool summerTime;

	if (month() < 3 || month() > 10)
	{
		summerTime = false; // keine Sommerzeit in Jan, Feb, Nov, Dez
	}
	else if (month() > 3 && month() < 10) {
		summerTime = true; // Sommerzeit in Apr, Mai, Jun, Jul, Aug, Sep
	}
	else if (month() == 3 && (hour() + 24 * day()) >= (1 + String(timeZone).toInt() + 24 * (31 - (5 * year() / 4 + 4) % 7)) || month() == 10 && (hour() + 24 * day()) < (1 + String(timeZone).toInt() + 24 * (31 - (5 * year() / 4 + 1) % 7)))
	{
		summerTime = true;
	}
	else
	{
		summerTime = false;
	}

	if (summerTime)
	{
		int temp = hourFormat12() + 1;
		if (temp == 13)
		{
			return 1;
		}
		else
		{
			return temp;
		}
	}
	else
	{
		return hourFormat12();
	}
}

void SetDisplayTime()
{
	matrix.clearDisplay(0);

	if (minute() >= 30 && minute() < 35)
	{
		SetWordsCheckForHalfFour();
	}
	else if (minute() >= 35 && minute() < 40)
	{
		SetWords("Top_5");
		SetWords("Nach");
		SetWordsCheckForHalfFour();
	}
	else if (minute() >= 40 && minute() < 45)
	{
		SetWords("Top_10");
		SetWords("Nach");
		SetWordsCheckForHalfFour();
	}
	else if (minute() >= 45 && minute() < 50)
	{
		SetWords("15");
		SetWords("Vor");
		SetWords(GetVorHour());
	}
	else if (minute() >= 50 && minute() < 55)
	{
		SetWords("Top_10");
		SetWords("Vor");
		SetWords(GetVorHour());
	}

	else if (minute() >= 55)
	{
		SetWords("Top_5");
		SetWords("Vor");
		SetWords(GetVorHour());
	}
	else if (minute() >= 5 && minute() < 10)
	{
		SetWords("Top_5");
		SetWords("Nach");
		SetWords(String(GetSummOrWinterHour()));
	}

	else if (minute() >= 10 && minute() < 15)
	{
		SetWords("Top_10");
		SetWords("Nach");
		SetWords(String(GetSummOrWinterHour()));
	}

	else if (minute() >= 15 && minute() < 20)
	{
		SetWords("15");
		SetWords("Nach");
		SetWords(String(GetSummOrWinterHour()));
	}
	else if (minute() >= 20 && minute() < 25)
	{
		SetWords("Top_10");
		SetWords("Vor");
		SetWordsCheckForHalfFour();
	}
	else if (minute() >= 25 && minute() < 30)
	{
		SetWords("Top_5");
		SetWords("Vor");
		SetWordsCheckForHalfFour();
	}
	else
	{
		SetWords(String(GetSummOrWinterHour()));
	}


}

void SetWordsCheckForHalfFour()
{
	if (GetVorHour() == "4")
	{
		SetWords("Halb_4");
	}
	else
	{
		SetWords("Halb");
		SetWords(GetVorHour());
	}
}

String GetVorHour()
{
	int h = GetSummOrWinterHour() + 1;

	if (h > 12)
	{
		h = 1;
	}
	return String(h);
}

void SetWords(String word)
{
	if (word == "1")
	{
		matrix.setRow(0, 3, B11110000);
	}
	else if (word == "2")
	{
		matrix.setRow(0, 4, B00011000);
		matrix.setRow(0, 5, B00000110);
	}
	else if (word == "3")
	{
		matrix.setRow(0, 5, B00011110);
	}
	else if (word == "4")
	{
		matrix.setRow(0, 2, B00001111);
	}
	else if (word == "5")
	{
		matrix.setRow(0, 4, B00000001);
		matrix.setRow(0, 5, B00000001);
		matrix.setRow(0, 6, B00000001);
		matrix.setRow(0, 7, B00000001);
	}
	else if (word == "6")
	{
		matrix.setRow(0, 3, B00011111);
	}
	else if (word == "7")
	{
		matrix.setRow(0, 4, B11100000);
		matrix.setRow(0, 5, B11100000);
	}
	else if (word == "8")
	{
		matrix.setRow(0, 7, B11110000);
	}
	else if (word == "9")
	{
		matrix.setRow(0, 6, B00001111);
	}
	else if (word == "10")
	{
		matrix.setRow(0, 6, B01111000);
	}
	else if (word == "11")
	{
		matrix.setRow(0, 7, B00000111);
	}
	else if (word == "12")
	{
		matrix.setRow(0, 4, B00011111);
	}
	else if (word == "Vor")
	{
		matrix.setRow(0, 1, B11100000);
	}
	else if (word == "Nach")
	{
		matrix.setRow(0, 1, B00011110);
	}
	else if (word == "Halb")
	{
		matrix.setRow(0, 2, B11110000);
	}
	else if (word == "15")
	{
		matrix.setRow(0, 0, B11111111);
	}
	else if (word == "Top_5")
	{
		matrix.setRow(0, 0, B11110000);
	}
	else if (word == "Top_10")
	{
		matrix.setRow(0, 0, B00001111);
	}
	else if (word == "Halb_4")
	{
		matrix.setRow(0, 2, B11111111);
	}
	else if (word == "Boot")
	{
		matrix.setRow(0, 0, B10000001);
		matrix.setRow(0, 7, B10000001);
	}
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];

time_t getNtpTime()
{
	while (udp.parsePacket() > 0);
	Serial.println("Transmit NTP Request");
	sendNTPpacket(timeServerIP);
	uint32_t beginWait = millis();
	while (millis() - beginWait < 1500)
	{
		int size = udp.parsePacket();
		if (size >= NTP_PACKET_SIZE)
		{
			Serial.println("Receive NTP Response");
			udp.read(packetBuffer, NTP_PACKET_SIZE);
			unsigned long secsSince1900;

			secsSince1900 = (unsigned long)packetBuffer[40] << 24;
			secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
			secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
			secsSince1900 |= (unsigned long)packetBuffer[43];
			return secsSince1900 - 2208988800UL + String(timeZone).toInt() * SECS_PER_HOUR;
		}
	}
	Serial.println("No NTP Response :-(");
	return 0;
}

void sendNTPpacket(IPAddress &address)
{
	memset(packetBuffer, 0, NTP_PACKET_SIZE);

	packetBuffer[0] = 0b11100011;
	packetBuffer[1] = 0;
	packetBuffer[2] = 6;
	packetBuffer[3] = 0xEC;

	packetBuffer[12] = 49;
	packetBuffer[13] = 0x4E;
	packetBuffer[14] = 49;
	packetBuffer[15] = 52;

	udp.beginPacket(address, 123);
	udp.write(packetBuffer, NTP_PACKET_SIZE);
	udp.endPacket();
}
