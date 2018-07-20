/******************************************************************
* An example of how to use a bulk messages to subscribed users.   *
*                                                                 *
*                                                                 *
* written by Vadim Sinitski                                       *
* modified by Jose Rios                                           *
*******************************************************************/
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <FS.h>
#include <ArduinoJson.h>

// Initialize Wifi connection to the router
char ssid[] = "XXXXXX";     // your network SSID (name)
char password[] = "YYYYYY"; // your network key

// Initialize Telegram BOT
#define BOTtoken "XXXXXXXXX:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"  // your Bot Token (Get from Botfather)

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOTtoken, secured_client);

unsigned long Bot_mtbs = 1000; // mean time between scan messages
unsigned long Bot_lasttime;   // last time messages' scan has been done

int bulk_messages_mtbs = 1500; // mean time between send messages, 1.5 seconds
int messages_limit_per_second = 25; // Telegram API have limit for bulk messages ~30 messages per second

const char subscribed_users_filename[] = "/subscribed_users.json";

DynamicJsonBuffer jsonBuffer;

JsonObject& getSubscribedUsers() {
  File subscribedUsersFile = SPIFFS.open(subscribed_users_filename, "r");

  if (!subscribedUsersFile) {
    Serial.println("Failed to open subscribed users file");

    // Create empty file (w+ not working as expect)
    File f = SPIFFS.open(subscribed_users_filename, "w");
    f.close();

    JsonObject& users = jsonBuffer.createObject();

    return users;
  } else {

    size_t size = subscribedUsersFile.size();

    if (size > 1024) {
      Serial.println("Subscribed users file is too large");
      //return users;
    }

    static char file_content[1024];
    memset(file_content, '\0', 1024);
    subscribedUsersFile.readBytes(file_content, 1024);
    file_content[1023] = '\0';

    JsonObject& users = jsonBuffer.parseObject(file_content);

    if (!users.success()) {
      Serial.println("Failed to parse subscribed users file");
      return users;
    }

    subscribedUsersFile.close();

    return users;
  }
}

bool addSubscribedUser(const char* chat_id, const char* from_name) {
  JsonObject& users = getSubscribedUsers();

  File subscribedUsersFile = SPIFFS.open(subscribed_users_filename, "w+");

  if (!subscribedUsersFile) {
    Serial.println("Failed to open subscribed users file for writing");
    //return false;
  }

  users.set(chat_id, from_name);
  users.printTo(subscribedUsersFile);

  subscribedUsersFile.close();

  return true;
}

bool removeSubscribedUser(const char* chat_id) {
  JsonObject& users = getSubscribedUsers();

  File subscribedUsersFile = SPIFFS.open(subscribed_users_filename, "w");

  if (!subscribedUsersFile) {
    Serial.println("Failed to open subscribed users file for writing");
    return false;
  }

  users.remove(chat_id);
  users.printTo(subscribedUsersFile);

  subscribedUsersFile.close();

  return true;
}

void sendMessageToAllSubscribedUsers(const char* message) {
  int users_processed = 0;

  JsonObject& users = getSubscribedUsers();

  for (JsonObject::iterator it=users.begin(); it!=users.end(); ++it) {
    users_processed++;

    if (users_processed < messages_limit_per_second)  {
      const char* chat_id = it->key;
      bot.sendMessage(chat_id, message, "");
    } else {
      delay(bulk_messages_mtbs);
      users_processed = 0;
    }
  }
}

void handleNewMessages(int numNewMessages) {
  char* chat_id;
  char* text;
  char* from_name;

  Serial.println("handleNewMessages");
  Serial.println(numNewMessages);

  for (int i=0; i<numNewMessages; i++) {
    chat_id = bot.messages[i].chat_id;
    text = bot.messages[i].text;

    from_name = bot.messages[i].from_name;
    if (strcmp(from_name, "") == 0)
      from_name = (char*)"Guest\0";

    if (strcmp(text, "/start") == 0) {
      if (addSubscribedUser(chat_id, from_name)) {
        static char welcome[128];
        memset(welcome, '\0', 128);
        sprintf(welcome, "Welcome to Universal Arduino Telegram Bot library, %s.\n"
          "This is Bulk Messages example.\n"
          "\n"
          "/showallusers : show all subscribed users\n"
          "/testbulkmessage : send test message to subscribed users\n"
          "/removeallusers : remove all subscribed users\n"
          "/stop : unsubscribe from bot\n", from_name);
        bot.sendMessage(chat_id, welcome, "Markdown");
      } else {
        bot.sendMessage(chat_id, "Something wrong, please try again (later?)", "");
      }
    }

    if (strcmp(text, "/stop") == 0) {
      if (removeSubscribedUser(chat_id)) {
          char to_print[64];
          to_print[0] = '\0';
          sprintf(to_print, "Thank you, %s , we always waiting you back", from_name);
        bot.sendMessage(chat_id, to_print, "");
      } else {
        bot.sendMessage(chat_id, "Something wrong, please try again (later?)", "");
      }
    }

    if (strcmp(text, "/testbulkmessage") == 0) {
      sendMessageToAllSubscribedUsers("ATTENTION, this is bulk message for all subscribed users!");
    }

    if (strcmp(text, "/showallusers") == 0) {
      File subscribedUsersFile = SPIFFS.open(subscribed_users_filename, "r");

      if (!subscribedUsersFile) {
        bot.sendMessage(chat_id, "No subscription file", "");
      }

      size_t size = subscribedUsersFile.size();

      if (size > 1024) {
        bot.sendMessage(chat_id, "Subscribed users file is too large", "");
      } else {
        static char file_content[1024];
        memset(file_content, '\0', 1024);
        subscribedUsersFile.readBytes(file_content, 1024);
        file_content[1023] = '\0';
        bot.sendMessage(chat_id, file_content, "");
      }
    }

    if (strcmp(text, "/removeallusers") == 0) {
      if (SPIFFS.remove(subscribed_users_filename)) {
        bot.sendMessage(chat_id, "All users removed", "");
      } else {
        bot.sendMessage(chat_id, "Something wrong, please try again (later?)", "");
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

  if (!SPIFFS.begin()) {
    Serial.print("\nFailed to mount file system");
    return;
  }

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
