/*******************************************************************
 * An example of bot that show bot action message.                 *
 *                                                                 *
 * written by Vadim Sinitski                                       *
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
bool Start = false;

void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(numNewMessages);

  for (int i=0; i<numNewMessages; i++) {
    char* chat_id = bot.messages[i].chat_id;
    char* text = bot.messages[i].text;

    char* from_name = bot.messages[i].from_name;
    if (strcmp(from_name, "") == 0)
      from_name = (char*)"Guest\0";

    if (strcmp(text, "/send_test_action") == 0) {
      bot.sendChatAction(chat_id, "typing");
      delay(4000);
      bot.sendMessage(chat_id, "Did you see the action message?");

      // You can't use own message, just choose from one of bellow

      //typing for text messages
      //upload_photo for photos
      //record_video or upload_video for videos
      //record_audio or upload_audio for audio files
      //upload_document for general files
      //find_location for location data

      //more info here - https://core.telegram.org/bots/api#sendchataction
    }

    if (strcmp(text, "/start") == 0) {
      static char welcome[512];
      sprintf(welcome, 
        "Welcome to Universal Arduino Telegram Bot library, %s.\n"
        "This is Chat Action Bot example.\n"
        "\n"
        "/send_test_action : to send test chat action message\n"
        , from_name);
      bot.sendMessage(chat_id, welcome);
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
