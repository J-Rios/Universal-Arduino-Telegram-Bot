/*******************************************************************
 * An example of recieving location Data                           *
 *                                                                 *
 * By Brian Lough                                                  *
 * Modified by Jose Rios                                           *
 *******************************************************************/
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// Initialize Wifi connection to the router
const char ssid[] = "SSID";     // your network SSID (name)
const char password[] = "password"; // your network key

// Initialize Telegram BOT
#define BOTtoken "XXXXXXXXX:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"  // your Bot Token (Get from Botfather)

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

unsigned long Bot_mtbs = 1000; //mean time between scan messages
unsigned long Bot_lasttime;   //last time messages' scan has been done

void handleNewMessages(int numNewMessages) {
  static char message[512];
  for (int i = 0; i < numNewMessages; i++) {
    char* chat_id = bot.messages[i].chat_id;
    char* text = bot.messages[i].text;

    char* from_name = bot.messages[i].from_name;
    if (strcmp(from_name, "") == 0)
      from_name = (char*)"Guest\0";

    if (bot.messages[i].longitude != 0 || bot.messages[i].latitude != 0) {
      Serial.printf("Lat: %.6f\n", bot.messages[i].latitude);
      Serial.printf("Long: %.6f\n", bot.messages[i].longitude);

      sprintf(message, 
        "Lat: %.6f\n"
        "Long: %.6f\n"
        , bot.messages[i].latitude, bot.messages[i].longitude);
      bot.sendMessage(chat_id, message, "Markdown");
    } else if (strcmp(text, "/start") == 0) {
      sprintf(message, 
        "Welcome to Universal Arduino Telegram Bot library, %s.\n"
        "Share a location or a live location and the bot will respond with the co-ords.\n"
        , from_name);

      bot.sendMessage(chat_id, message, "Markdown");
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Set WiFi to station mode and disconnect from an AP if it was Previously connected
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
}

void loop() {
  if (millis() > Bot_lasttime + Bot_mtbs)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    Bot_lasttime = millis();
  }
}
