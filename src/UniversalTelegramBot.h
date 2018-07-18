/*
Copyright (c) 2018 Brian Lough. All right reserved.

UniversalTelegramBot - Library to create your own Telegram Bot using
ESP8266 or ESP32 on Arduino IDE.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef UniversalTelegramBot_h
#define UniversalTelegramBot_h

#include <Arduino.h>
#include <Client.h>

#define ARDUINOJSON_ENABLE_ARDUINO_STRING 0 // Disable String objects in ArduinoJson
#include <ArduinoJson.h>

#define HANDLE_MESSAGES 1

const char HOST[] = "api.telegram.org";
const uint16_t SSL_PORT = 443;
const uint8_t TOKEN_LENGTH = 46;
const uint8_t MAX_DATE_LENGTH = 64;
const uint8_t MAX_ID_LENGTH = UINT8_MAX;
const uint16_t MAX_CMD_LENGTH = 512;
const uint16_t MAX_USER_NAME_LENGTH = 256;
const uint16_t MAX_MESSAGE_TEXT_LENGTH = 4097;
const uint16_t MAX_MESSAGE_LENGTH = TOKEN_LENGTH + MAX_DATE_LENGTH + MAX_MESSAGE_TEXT_LENGTH + 
                                    MAX_ID_LENGTH + MAX_CMD_LENGTH + MAX_USER_NAME_LENGTH + 32;

typedef bool (*MoreDataAvailable)();
typedef byte (*GetNextByte)();

struct telegramMessage {
  char text[MAX_MESSAGE_TEXT_LENGTH];
  char chat_id[MAX_ID_LENGTH];
  char chat_title[MAX_USER_NAME_LENGTH];
  char from_id[MAX_ID_LENGTH];
  char from_name[MAX_USER_NAME_LENGTH];
  char date[MAX_DATE_LENGTH];
  char type[MAX_CMD_LENGTH];
  float longitude;
  float latitude;
  int update_id;
};

class UniversalTelegramBot {
public:
  UniversalTelegramBot(const char* token, Client &client);
  char* sendGetToTelegram(const char* command);
  char* sendPostToTelegram(const char* command, JsonObject &payload);
  char*
  sendMultipartFormDataToTelegram(const char* command, const char* binaryProperyName,
                                  const char* fileName, const char* contentType,
                                  const char* chat_id, int fileSize,
                                  MoreDataAvailable moreDataAvailableCallback,
                                  GetNextByte getNextByteCallback);

  bool getMe();

  bool sendSimpleMessage(const char* chat_id, const char* text, const char* parse_mode);
  bool sendMessage(const char* chat_id, const char* text, const char* parse_mode = "");
  bool sendMessageWithReplyKeyboard(const char* chat_id, const char* text,
                                    const char* parse_mode, const char* keyboard,
                                    bool resize = false, bool oneTime = false,
                                    bool selective = false);
  bool sendMessageWithInlineKeyboard(const char* chat_id, const char* text,
                                     const char* parse_mode, const char* keyboard);

  bool sendChatAction(const char* chat_id, const char* text);

  bool sendPostMessage(JsonObject &payload);
  char* sendPostPhoto(JsonObject &payload);
  char* sendPhotoByBinary(const char* chat_id, const char* contentType, int fileSize,
                           MoreDataAvailable moreDataAvailableCallback,
                           GetNextByte getNextByteCallback);
  char* sendPhoto(const char* chat_id, const char* photo, const char* caption = "",
                   bool disable_notification = false,
                   int reply_to_message_id = 0, const char* keyboard = "");

  int getUpdates(long offset);
  bool checkForOkResponse(char* response);
  telegramMessage messages[HANDLE_MESSAGES]; //
  long last_message_received = 0;
  char name[MAX_USER_NAME_LENGTH];
  char userName[MAX_USER_NAME_LENGTH];
  uint16_t longPoll = 0;
  bool _debug = false;
  uint16_t waitForResponse = 1500;

private:
  // JsonObject * parseUpdates(const char* response);
  char _token[TOKEN_LENGTH];
  char _msg[MAX_MESSAGE_LENGTH];
  Client *client;
  bool processResult(JsonObject &result, int messageIndex);
  void closeClient();
};

#endif
