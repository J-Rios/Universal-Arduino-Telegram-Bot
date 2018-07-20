/*******************************************************************
 * An example of bot that echos back any messages received,        *
 * including ones from channels                                    *
 *                                                                 *
 * written by Brian Lough                                          *
 * modified by Jose Rios                                           *
 *******************************************************************/
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// Initialize Wifi connection to the router
char ssid[] = "XXXXXX";     // your network SSID (name)
char password[] = "YYYYYY"; // your network key

// Initialize Telegram BOT
#define BOTtoken "XXXXXXXXX:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"  // your Bot Token (Get from Botfather)

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

unsigned long Bot_mtbs = 1000; //mean time between scan messages
unsigned long Bot_lasttime;   //last time messages' scan has been done

void setup() {
  Serial.begin(115200);

  // Set WiFi to station mode and disconnect from an AP if it was Previously
  // connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Attempt to connect to Wifi network:
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
  static char msg_to_send[512];
  if (millis() > Bot_lasttime + Bot_mtbs)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      Serial.println("got response");
      for (int i=0; i<numNewMessages; i++) {
        memset(msg_to_send, '\0', 512);
        sprintf(msg_to_send, "%s %s", bot.messages[i].chat_title, bot.messages[i].text);
        if(strcmp(bot.messages[i].type, "channel_post") == 0) {
          bot.sendMessage(bot.messages[i].chat_id, msg_to_send, "");
        } else {
          bot.sendMessage(bot.messages[i].chat_id, bot.messages[i].text, "");
        }
      }

      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    Bot_lasttime = millis();
  }
}
