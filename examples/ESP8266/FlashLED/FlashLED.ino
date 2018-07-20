/*******************************************************************
 * An example of bot that receives commands and turns on and off   *
 * an LED.                                                         *
 *                                                                 *
 * written by Giacarlo Bacchio (Gianbacchio on Github)             *
 * adapted by Brian Lough                                          *
 * modified by Jose Rios                                           *
 *******************************************************************/
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// Initialize Wifi connection to the router
const char ssid[] = "XXXXXX";     // your network SSID (name)
const char password[] = "YYYYYY"; // your network key

// Initialize Telegram BOT
#define BOTtoken "XXXXXXXXX:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"  // your Bot Token (Get from Botfather)

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

unsigned long Bot_mtbs = 1000; //mean time between scan messages
unsigned long Bot_lasttime;   //last time messages' scan has been done
bool Start = false;

const int ledPin = 13;
int ledStatus = 0;

void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(numNewMessages);

  for (int i=0; i<numNewMessages; i++) {
    char* chat_id = bot.messages[i].chat_id;
    char* text = bot.messages[i].text;

    char* from_name = bot.messages[i].from_name;
    if (strcmp(from_name, "") == 0)
      from_name = (char*)"Guest\0";

    if (strcmp(text, "/ledon") == 0) {
      digitalWrite(ledPin, HIGH);   // turn the LED on (HIGH is the voltage level)
      ledStatus = 1;
      bot.sendMessage(chat_id, "Led is ON", "");
    }

    if (strcmp(text, "/ledoff") == 0) {
      ledStatus = 0;
      digitalWrite(ledPin, LOW);    // turn the LED off (LOW is the voltage level)
      bot.sendMessage(chat_id, "Led is OFF", "");
    }

    if (strcmp(text, "/status") == 0) {
      if(ledStatus)
        bot.sendMessage(chat_id, "Led is ON", "");
      else
        bot.sendMessage(chat_id, "Led is OFF", "");
    }

    if (strcmp(text, "/start") == 0) {
      static char welcome[512];
      sprintf(welcome, "Welcome to Universal Arduino Telegram Bot library, %s.\n"
        "This is Flash Led Bot example.\n"
        "\n"
        "/ledon : to switch the Led ON\n"
        "/ledoff : to switch the Led OFF\n"
        "/status : Returns current status of LED"
        , from_name);
      bot.sendMessage(chat_id, welcome, "Markdown");
    }
  }
}


void setup() {
  Serial.begin(115200);

  // Set WiFi to station mode and disconnect from an AP if it was Previously
  // connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // attempt to connect to Wifi network:
  Serial.printf("\nConnecting Wifi: %s\n", ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  pinMode(ledPin, OUTPUT); // initialize digital ledPin as an output.
  delay(10);
  digitalWrite(ledPin, LOW); // initialize pin as off
}

void loop() {
  if (millis() > Bot_lasttime + Bot_mtbs)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    Bot_lasttime = millis();
  }
}
