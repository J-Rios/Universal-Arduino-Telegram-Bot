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

/*
   **** Note Regarding Client Connection Keeping ****
   Client connection is established in functions that directly involve use of
   client, i.e sendGetToTelegram, sendPostToTelegram, and
   sendMultipartFormDataToTelegram. It is closed at the end of
   sendMultipartFormDataToTelegram, but not at the end of sendGetToTelegram and
   sendPostToTelegram as these may need to keep the connection alive for respose
   / response checking. Re-establishing a connection then wastes time which is
   noticeable in user experience. Due to this, it is important that connection
   be closed manually after calling sendGetToTelegram or sendPostToTelegram by
   calling closeClient(); Failure to close connection causes memory leakage and
   SSL errors
 */

#include "UniversalTelegramBot.h"

UniversalTelegramBot::UniversalTelegramBot(const char* token, Client &client) {
  _token[0] = '\0';
  name[0] = '\0';
  userName[0] = '\0';
  _msg[0] = '\0';

  strncpy(_token, token, TOKEN_LENGTH);
  _token[TOKEN_LENGTH-1] = '\0';
  this->client = &client;
}

char* UniversalTelegramBot::sendGetToTelegram(const char* command) {
  char http_get_cmd[256]; http_get_cmd[0] = '\0';
  unsigned long now;
  bool avail;

  // Connect with api.telegram.org if not already connected
  if (!client->connected()) {
    if (_debug)
      Serial.println(F("[BOT]Connecting to server"));
    if (!client->connect(HOST, SSL_PORT)) {
      if (_debug)
        Serial.println(F("[BOT]Conection error"));
    }
  }
  if (client->connected()) {
    if (_debug)
      Serial.println(F(".... connected to server"));

    char c;
    int ch_count = 0;
    snprintf_P(http_get_cmd, 256, "GET /%s", command);
	http_get_cmd[255] = '\0';
    client->println(http_get_cmd);
    now = millis();
    avail = false;
    memset(_msg, '\0', MAX_MESSAGE_LENGTH);
    while (millis() - now < (unsigned long)(longPoll * 1000 + waitForResponse)) {
      while (client->available()) {
        c = client->read();
        // Serial.write(c);
        if (ch_count < MAX_MESSAGE_LENGTH) {
          _msg[ch_count] = c;
          ch_count++;
          if(ch_count != MAX_MESSAGE_LENGTH)
            _msg[ch_count] = '\0';
        }
        avail = true;
      }
      if (avail) {
        if (_debug) {
          Serial.println();
          Serial.println(_msg);
          Serial.println();
        }
        break;
      }
      if(millis() < now)
        now = millis();
    }
  }

  return _msg;
}

char* UniversalTelegramBot::sendPostToTelegram(const char* command,
                                                JsonObject &payload) {
  char http_post_cmd[MAX_CMD_LENGTH]; http_post_cmd[0] = '\0';
  //String headers = ""; // Why store http response header if not used?
  long now;
  bool responseReceived;

  // Connect with api.telegram.org if not already connected
  if (!client->connected()) {
    if (_debug)
      Serial.println(F("[BOT Client]Connecting to server"));
    if (!client->connect(HOST, SSL_PORT)) {
      if (_debug)
        Serial.println(F("[BOT Client]Conection error"));
    }
  }
  if (client->connected()) {
    // POST URI
    snprintf_P(http_post_cmd, MAX_CMD_LENGTH, "POST /%s", command);
	http_post_cmd[MAX_CMD_LENGTH-1] = '\0';
    client->print(http_post_cmd);
    client->println(F(" HTTP/1.1"));
    // Host header
    client->print(F("Host:"));
    client->println(HOST);
    // JSON content type
    client->println(F("Content-Type: application/json"));

    // Content length
    int length = payload.measureLength();
    client->print(F("Content-Length:"));
    client->println(length);
    // End of headers
    client->println();
    // POST message body
    payload.printTo(*client); // Not really too slow?

    int ch_count = 0;
    char c;
    now = millis();
    responseReceived = false;
    bool finishedHeaders = false;
    bool currentLineIsBlank = true;
    memset(_msg, '\0', MAX_MESSAGE_LENGTH);
    while (millis() - now < waitForResponse) {
      while (client->available()) {
        c = client->read();
        responseReceived = true;

        if (!finishedHeaders) {
          if (currentLineIsBlank && c == '\n') {
            finishedHeaders = true;
          }/* else {
            headers = headers + c; // Why store http response header if not used?
          }*/
        } else {
          if (ch_count < MAX_MESSAGE_LENGTH) {
            _msg[ch_count] = c;
            ch_count++;

            if(ch_count != MAX_MESSAGE_LENGTH)
              _msg[ch_count] = '\0';
          }
        }

        if (c == '\n') {
          currentLineIsBlank = true;
        } else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }

      if (responseReceived) {
        if (_debug) {
          Serial.println();
          Serial.println(_msg);
          Serial.println();
        }
        break;
      }
    }
  }

  return _msg;
}

char* UniversalTelegramBot::sendMultipartFormDataToTelegram(
    const char* command, const char* binaryProperyName, const char* fileName,
    const char* contentType, const char* chat_id, int fileSize,
    MoreDataAvailable moreDataAvailableCallback,
    GetNextByte getNextByteCallback) {

  char http_post_cmd[MAX_CMD_LENGTH]; http_post_cmd[0] = '\0';
  char to_print[MAX_CMD_LENGTH]; to_print[0] = '\0';

  //String headers = ""; // Why store http response header if not used?
  long now;
  bool responseReceived;
  const char boundry[] = "------------------------b8f610217e83e29b";

  // Connect with api.telegram.org if not already connected
  if (!client->connected()) {
    if (_debug)
      Serial.println(F("[BOT Client]Connecting to server"));
    if (!client->connect(HOST, SSL_PORT)) {
      if (_debug)
        Serial.println(F("[BOT Client]Conection error"));
    }
  }
  if (client->connected()) {

    char start_request[MAX_MESSAGE_LENGTH] = "";
    char end_request[MAX_MESSAGE_LENGTH] = "";

    snprintf_P(start_request, MAX_MESSAGE_LENGTH, "--%s\r\n"
                           "content-disposition: form-data; name=\"chat_id\"\r\n"
                           "\r\n"
                           "%s\r\n"
                           "--%s\r\n"
                           "content-disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n"
                           "Content-Type: %s\r\n"
                           "\r\n"
                           , boundry, chat_id, boundry, binaryProperyName, fileName, contentType);
    start_request[MAX_MESSAGE_LENGTH-1] = '\0';

    snprintf_P(end_request, MAX_MESSAGE_LENGTH, "\r\n--%s--\r\n", boundry);
	end_request[MAX_MESSAGE_LENGTH-1] = '\0';

    snprintf_P(http_post_cmd, MAX_CMD_LENGTH, "POST /bot %s /%s", _token, command);
	http_post_cmd[MAX_CMD_LENGTH-1] = '\0';
    client->print(http_post_cmd);
    client->println(F(" HTTP/1.1"));
    // Host header
    client->print(F("Host: "));
    client->println(HOST);
    client->println(F("User-Agent: arduino/1.0"));
    client->println(F("Accept: */*"));

    int contentLength = fileSize + strlen(start_request) + strlen(end_request);
    snprintf_P(to_print, MAX_CMD_LENGTH, "Content-Length: %d", contentLength);
	to_print[MAX_CMD_LENGTH-1] = '\0';
    if (_debug)
      Serial.println(to_print);
    client->println(to_print);
    snprintf_P(to_print, MAX_CMD_LENGTH, "Content-Type: multipart/form-data; boundary=%s", boundry);
	to_print[MAX_CMD_LENGTH-1] = '\0';
    client->println(to_print);
    client->println("");

    client->print(start_request);

    if (_debug)
      Serial.print(start_request);

    byte buffer[512];
    int count = 0;
    //char ch;
    while (moreDataAvailableCallback()) {
      buffer[count] = getNextByteCallback();
      // client->write(ch);
      // Serial.write(ch);
      count++;
      if (count == 512) {
        // yield();
        if (_debug) {
          Serial.println(F("Sending full buffer"));
        }
        client->write((const uint8_t *)buffer, 512);
        count = 0;
      }
    }

    if (count > 0) {
      if (_debug) {
        Serial.println(F("Sending remaining buffer"));
      }
      client->write((const uint8_t *)buffer, count);
    }

    client->print(end_request);
    if (_debug)
      Serial.print(end_request);

    count = 0;
    int ch_count = 0;
    char c;
    now = millis();
    bool finishedHeaders = false;
    bool currentLineIsBlank = true;
    memset(_msg, '\0', MAX_MESSAGE_LENGTH);
    while (millis() - now < waitForResponse) {
      while (client->available()) {
        c = client->read();
        responseReceived = true;

        if (!finishedHeaders) {
          if (currentLineIsBlank && c == '\n') {
            finishedHeaders = true;
          }/* else {
            headers = headers + c; // Why store http response header if not used?
          }*/
        } else {
          if (ch_count < MAX_MESSAGE_LENGTH) {
            _msg[ch_count] = c;
            ch_count++;

            if(ch_count != MAX_MESSAGE_LENGTH)
              _msg[ch_count] = '\0';
          }
        }

        if (c == '\n') {
          currentLineIsBlank = true;
        } else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }

      if (responseReceived) {
        if (_debug) {
          Serial.println();
          Serial.println(_msg);
          Serial.println();
        }
        break;
      }
    }
  }

  closeClient();
  return _msg;
}

bool UniversalTelegramBot::getMe() {
  char command[MAX_CMD_LENGTH]; command[0] = '\0';
  snprintf_P(command, MAX_CMD_LENGTH, "bot%s/getMe", _token);
  command[MAX_CMD_LENGTH-1] = '\0';
  memset(_msg, '\0', MAX_MESSAGE_LENGTH);
  strncpy(_msg, sendGetToTelegram(command), MAX_MESSAGE_LENGTH); // receive reply from telegram
  _msg[MAX_MESSAGE_LENGTH-1] = '\0';
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.parseObject(_msg);

  closeClient();

  if (root.success()) {
    if (root.containsKey("result")) {
      strncpy(name, root["result"]["first_name"], MAX_USER_NAME_LENGTH);
      name[MAX_USER_NAME_LENGTH-1] = '\0';
      strncpy(userName, root["result"]["username"], MAX_USER_NAME_LENGTH);
      userName[MAX_USER_NAME_LENGTH-1] = '\0';
      return true;
    }
  }

  return false;
}

/***************************************************************
 * GetUpdates - function to receive messages from telegram *
 * (Argument to pass: the last+1 message to read)             *
 * Returns the number of new messages           *
 ***************************************************************/
int UniversalTelegramBot::getUpdates(long offset) {
  char command[MAX_CMD_LENGTH]; command[0] = '\0';
  if (_debug)
    Serial.println(F("GET Update Messages"));

  snprintf_P(command, MAX_CMD_LENGTH, "bot%s/getUpdates?offset=%ld&limit=%d", _token, offset, HANDLE_MESSAGES);
  command[MAX_CMD_LENGTH-1] = '\0';
  if (longPoll > 0) {
    snprintf_P(command, MAX_CMD_LENGTH, "%s%d", command, longPoll);
	command[MAX_CMD_LENGTH-1] = '\0';
  }
  memset(_msg, '\0', MAX_MESSAGE_LENGTH);
  strncpy(_msg, sendGetToTelegram(command), MAX_CMD_LENGTH); // receive reply from telegram.org
  _msg[MAX_MESSAGE_LENGTH-1] = '\0';

  if (strcmp(_msg, "") == 0) {
    if (_debug)
      Serial.println(F("Received empty string in response!"));
    // close the client as there's nothing to do with an empty string
    closeClient();
    return 0;
  } else {
    if (_debug) {
      Serial.print(F("Incoming message length: "));
      Serial.println(strlen(_msg));
      Serial.println(F("Creating DynamicJsonBuffer"));
    }

    // Parse response into Json object
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.parseObject(_msg);
    root.begin();

    if (root.success()) {
      // root.printTo(Serial);
      if (_debug)
        Serial.println();
      if (root.containsKey("result")) {
        int resultArrayLength = root["result"].size();
        if (resultArrayLength > 0) {
          int newMessageIndex = 0;
          // Step through all results
          for (int i = 0; i < resultArrayLength; i++) {
            JsonObject &result = root["result"][i];
            if (processResult(result, newMessageIndex)) {
              newMessageIndex++;
            }
          }
          // We will keep the client open because there may be a response to be
          // given
          return newMessageIndex;
        } else {
          if (_debug)
            Serial.println(F("no new messages"));
        }
      } else {
        if (_debug)
          Serial.println(F("Response contained no 'result'"));
      }
    } else { // Parsing failed
      if (strlen(_msg) < 2) { // Too short a message. Maybe connection issue
        if (_debug)
          Serial.println(F("Parsing error: Message too short"));
      } else {
        // Buffer may not be big enough, increase buffer or reduce max number of
        // messages
        if (_debug)
          Serial.println(F("Failed to parse update, the message could be too "
                           "big for the buffer"));
      }
    }
    // Close the client as no response is to be given
    closeClient();
    return 0;
  }
}

bool UniversalTelegramBot::processResult(JsonObject &result, int messageIndex) {
  int update_id = result["update_id"];
  // Check have we already dealt with this message (this shouldn't happen!)
  if (last_message_received != update_id) {
    last_message_received = update_id;
    messages[messageIndex].update_id = update_id;

    memset(messages[messageIndex].text, '\0', MAX_MESSAGE_TEXT_LENGTH);
    memset(messages[messageIndex].chat_id, '\0', MAX_ID_LENGTH);
    memset(messages[messageIndex].chat_title, '\0', MAX_USER_NAME_LENGTH);
    memset(messages[messageIndex].from_id, '\0', MAX_ID_LENGTH);
    memset(messages[messageIndex].from_name, '\0', MAX_USER_NAME_LENGTH);
    memset(messages[messageIndex].date, '\0', MAX_DATE_LENGTH);
    memset(messages[messageIndex].type, '\0', MAX_CMD_LENGTH);
    messages[messageIndex].longitude = 0;
    messages[messageIndex].latitude = 0;

    // Uncomment next to see receive json message
    //if (_debug)
      //result.printTo(Serial);

    if (result.containsKey("message")) {
      JsonObject &message = result["message"];
      strncpy(messages[messageIndex].type, "message", strlen("message")+1);
      strncpy(messages[messageIndex].from_id, message["from"].as<JsonObject>().get<char*>("id"), 
              message["from"]["id"].measureLength()+1);
      strncpy(messages[messageIndex].from_name, 
              message["from"].as<JsonObject>().get<char*>("first_name"),
              message["from"]["first_name"].measureLength()+1);
      strncpy(messages[messageIndex].date, message.get<char*>("date"), 
              message["date"].measureLength()+1);
      strncpy(messages[messageIndex].chat_id, message["chat"].as<JsonObject>().get<char*>("id"), 
              message["chat"]["id"].measureLength()+1);
      
      if (message["chat"].as<JsonObject>().containsKey("title")) {
        strncpy(messages[messageIndex].chat_title, 
                message["chat"].as<JsonObject>().get<char*>("title"), 
                message["chat"]["title"].measureLength()+1);
      }
      else
        strncpy(messages[messageIndex].chat_title, "", 1);

      if (message.containsKey("text")) {
        strncpy(messages[messageIndex].text, message.get<char*>("text"), 
                message["text"].measureLength()+1);
      } else if (message.containsKey("location")) {
        messages[messageIndex].longitude =
            message["location"]["longitude"].as<float>();
        messages[messageIndex].latitude =
            message["location"]["latitude"].as<float>();
      }
    } else if (result.containsKey("channel_post")) {
      JsonObject &message = result["channel_post"];
      strncpy(messages[messageIndex].type, "channel_post", strlen("channel_post")+1);
      strncpy(messages[messageIndex].text, message.get<char*>("text"), 
              message["text"].measureLength()+1);
      strncpy(messages[messageIndex].date, message.get<char*>("date"), 
              message["date"].measureLength()+1);
      strncpy(messages[messageIndex].chat_id, message["chat"].as<JsonObject>().get<char*>("id"), 
              message["chat"]["id"].measureLength()+1);
      strncpy(messages[messageIndex].chat_title, 
              message["chat"].as<JsonObject>().get<char*>("title"), 
              message["chat"]["title"].measureLength()+1);
    } else if (result.containsKey("callback_query")) {
      JsonObject &message = result["callback_query"];
      strncpy(messages[messageIndex].type, "callback_query", strlen("callback_query")+1);
      strncpy(messages[messageIndex].from_id, message["from"].as<JsonObject>().get<char*>("id"), 
              message["from"]["id"].measureLength()+1);
      strncpy(messages[messageIndex].from_name, 
              message["from"].as<JsonObject>().get<char*>("first_name"),
              message["from"]["first_name"].measureLength()+1);
      strncpy(messages[messageIndex].text, message.get<char*>("data"), 
              message["data"].measureLength()+1);
      if (message.containsKey("message")) {
        strncpy(messages[messageIndex].date, 
          message["message"].as<JsonObject>().get<char*>("date"), 
              message["message"]["date"].measureLength()+1);
        strncpy(messages[messageIndex].chat_id, 
                message["message"]["chat"].as<JsonObject>().get<char*>("id"), 
                message["message"]["chat"]["id"].measureLength()+1);
      }
      strncpy(messages[messageIndex].chat_title, "", 1);
    } else if (result.containsKey("edited_message")) {
      JsonObject &message = result["edited_message"];
      strncpy(messages[messageIndex].type, "edited_message", strlen("edited_message")+1);
      strncpy(messages[messageIndex].from_id, message["from"].as<JsonObject>().get<char*>("id"), 
              message["from"]["id"].measureLength()+1);
      strncpy(messages[messageIndex].from_name, 
              message["from"].as<JsonObject>().get<char*>("first_name"),
              message["from"]["first_name"].measureLength()+1);
      strncpy(messages[messageIndex].date, message.get<char*>("date"), 
              message["date"].measureLength()+1);
      strncpy(messages[messageIndex].chat_id, message["chat"].as<JsonObject>().get<char*>("id"), 
              message["chat"]["id"].measureLength()+1);
      if (message["chat"].as<JsonObject>().containsKey("title")) {
        strncpy(messages[messageIndex].chat_title, 
                message["chat"].as<JsonObject>().get<char*>("title"), 
                message["chat"]["title"].measureLength()+1);
      }
      else
        strncpy(messages[messageIndex].chat_title, "", 1);

      if (message.containsKey("text")) {
        strncpy(messages[messageIndex].text, message.get<char*>("text"), 
                message["text"].measureLength()+1);
      } else if (message.containsKey("location")) {
        messages[messageIndex].longitude = message["location"]["longitude"].as<float>();
        messages[messageIndex].latitude = message["location"]["latitude"].as<float>();
      }
    }

    messages[messageIndex].text[MAX_MESSAGE_TEXT_LENGTH-1] = '\0';
    messages[messageIndex].chat_id[MAX_ID_LENGTH-1] = '\0';
    messages[messageIndex].chat_title[MAX_USER_NAME_LENGTH-1] = '\0';
    messages[messageIndex].from_id[MAX_ID_LENGTH-1] = '\0';
    messages[messageIndex].from_name[MAX_USER_NAME_LENGTH-1] = '\0';
    messages[messageIndex].date[MAX_DATE_LENGTH-1] = '\0';
    messages[messageIndex].type[MAX_CMD_LENGTH-1] = '\0';

    return true;
  }
  return false;
}

/***********************************************************************
 * SendMessage - function to send message to telegram                  *
 * (Arguments to pass: chat_id, text to transmit and markup(optional)) *
 ***********************************************************************/
bool UniversalTelegramBot::sendSimpleMessage(const char* chat_id, const char* text,
                                             const char* parse_mode) {
  char command[MAX_CMD_LENGTH]; command[0] = '\0';
  bool sent = false;
  if (_debug)
    Serial.println(F("SEND Simple Message"));
  unsigned long sttime = millis();

  if (strcmp(text ,"") != 0) {
    while (millis() < sttime + 8000) { // loop for a while to send the message
      snprintf_P(command, MAX_CMD_LENGTH, "bot%s/sendMessage?chat_id=%s&text=%s&parse_mode=%s", _token, chat_id, 
              text, parse_mode);
	  command[MAX_CMD_LENGTH-1] = '\0';
      memset(_msg, '\0', MAX_MESSAGE_LENGTH);
      strncpy(_msg, sendGetToTelegram(command), MAX_MESSAGE_LENGTH);
      _msg[MAX_MESSAGE_LENGTH-1] = '\0';
      if (_debug)
        Serial.println(_msg);
      sent = checkForOkResponse(_msg);
      if (sent) {
        break;
      }
    }
  }
  closeClient();
  return sent;
}

bool UniversalTelegramBot::sendMessage(const char* chat_id, const char* text,
                                       const char* parse_mode) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject &payload = jsonBuffer.createObject();

  payload["chat_id"] = chat_id;
  payload["text"] = text;

  if (strcmp(parse_mode, "") != 0) {
    payload["parse_mode"] = parse_mode;
  }

  return sendPostMessage(payload);
}

bool UniversalTelegramBot::sendMessageWithReplyKeyboard(
    const char* chat_id, const char* text, const char* parse_mode, const char* keyboard,
    bool resize, bool oneTime, bool selective) {

  DynamicJsonBuffer jsonBuffer;
  JsonObject &payload = jsonBuffer.createObject();

  payload["chat_id"] = chat_id;
  payload["text"] = text;

  if (strcmp(parse_mode, "") != 0) {
    payload["parse_mode"] = parse_mode;
  }

  JsonObject &replyMarkup = payload.createNestedObject("reply_markup");

  // Reply keyboard is an array of arrays.
  // Outer array represents rows
  // Inner arrays represents columns
  // This example "ledon" and "ledoff" are two buttons on the top row
  // and "status is a single button on the next row"
  DynamicJsonBuffer keyboardBuffer;
  replyMarkup["keyboard"] = keyboardBuffer.parseArray(keyboard);

  // Telegram defaults these values to false, so to decrease the size of the
  // payload we will only send them if needed
  if (resize) {
    replyMarkup["resize_keyboard"] = resize;
  }

  if (oneTime) {
    replyMarkup["one_time_keyboard"] = oneTime;
  }

  if (selective) {
    replyMarkup["selective"] = selective;
  }

  return sendPostMessage(payload);
}

bool UniversalTelegramBot::sendMessageWithInlineKeyboard(const char* chat_id,
                                                         const char* text,
                                                         const char* parse_mode,
                                                         const char* keyboard) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject &payload = jsonBuffer.createObject();

  payload["chat_id"] = chat_id;
  payload["text"] = text;

  if (strcmp(parse_mode, "") != 0) {
    payload["parse_mode"] = parse_mode;
  }

  JsonObject &replyMarkup = payload.createNestedObject("reply_markup");

  DynamicJsonBuffer keyboardBuffer;
  replyMarkup["inline_keyboard"] = keyboardBuffer.parseArray(keyboard);

  return sendPostMessage(payload);
}

/***********************************************************************
 * SendPostMessage - function to send message to telegram                  *
 * (Arguments to pass: chat_id, text to transmit and markup(optional)) *
 ***********************************************************************/
bool UniversalTelegramBot::sendPostMessage(JsonObject &payload) {
  bool sent = false;
  if (_debug)
    Serial.println(F("SEND Post Message"));
  unsigned long sttime = millis();

  if (payload.containsKey("text")) {
    while (millis() < sttime + 8000) { // loop for a while to send the message
      char command[MAX_CMD_LENGTH]; command[0] = '\0';
      snprintf_P(command, MAX_CMD_LENGTH, "bot%s/sendMessage", _token);
	  command[MAX_CMD_LENGTH-1] = '\0';
      memset(_msg, '\0', MAX_MESSAGE_LENGTH);
      strncpy(_msg, sendPostToTelegram(command, payload), MAX_MESSAGE_LENGTH);
      _msg[MAX_MESSAGE_LENGTH-1] = '\0';
      if (_debug)
        Serial.println(_msg);
      sent = checkForOkResponse(_msg);
      if (sent) {
        break;
      }
    }
  }

  closeClient();
  return sent;
}

char* UniversalTelegramBot::sendPostPhoto(JsonObject &payload) {
  bool sent = false;
  memset(_msg, '\0', MAX_MESSAGE_LENGTH);
  if (_debug)
    Serial.println(F("SEND Post Photo"));
  unsigned long sttime = millis();

  if (payload.containsKey("photo")) {
    while (millis() < sttime + 8000) { // loop for a while to send the message
      char command[MAX_CMD_LENGTH]; command[0] = '\0';
      snprintf_P(command, MAX_CMD_LENGTH, "bot%s/sendPhoto", _token);
	  command[MAX_CMD_LENGTH-1] = '\0';
      strncpy(_msg, sendPostToTelegram(command, payload), MAX_MESSAGE_LENGTH);
      _msg[MAX_MESSAGE_LENGTH-1] = '\0';
      if (_debug)
        Serial.println(_msg);
      sent = checkForOkResponse(_msg);
      if (sent) {
        break;
      }
    }
  }

  closeClient();
  return _msg;
}

char* UniversalTelegramBot::sendPhotoByBinary(
    const char* chat_id, const char* contentType, int fileSize,
    MoreDataAvailable moreDataAvailableCallback,
    GetNextByte getNextByteCallback) {

  if (_debug)
    Serial.println("SEND Photo");

  memset(_msg, '\0', MAX_MESSAGE_LENGTH);
  strncpy(_msg, sendMultipartFormDataToTelegram(
      "sendPhoto", "photo", "img.jpg", contentType, chat_id, fileSize,
      moreDataAvailableCallback, getNextByteCallback), MAX_MESSAGE_LENGTH);
  _msg[MAX_MESSAGE_LENGTH-1] = '\0';
  if (_debug)
    Serial.println(_msg);

  return _msg;
}

char* UniversalTelegramBot::sendPhoto(const char* chat_id, const char* photo,
                                      const char* caption,
                                      bool disable_notification,
                                      int reply_to_message_id,
                                      const char* keyboard) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject &payload = jsonBuffer.createObject();

  payload["chat_id"] = chat_id;
  payload["photo"] = photo;

  if (caption) {
    payload["caption"] = caption;
  }

  if (disable_notification) {
    payload["disable_notification"] = disable_notification;
  }

  if (reply_to_message_id && reply_to_message_id != 0) {
    payload["reply_to_message_id"] = reply_to_message_id;
  }

  if (keyboard) {
    JsonObject &replyMarkup = payload.createNestedObject("reply_markup");

    DynamicJsonBuffer keyboardBuffer;
    replyMarkup["keyboard"] = keyboardBuffer.parseArray(keyboard);
  }

  return sendPostPhoto(payload);
}

bool UniversalTelegramBot::checkForOkResponse(char* response) {
  int responseLength = strlen(response);
  char substr[11]; substr[0] = '\0';
  for (int m = 0; m < responseLength + 1; m++) {
    if(m+10 < responseLength + 1) {
      memcpy(substr, &response[m], 10);
      substr[10] = '\0';
      if (strcmp(substr, "{\"ok\":true") == 0) // Chek if message has been properly sent
        return true;
    }
    else
      break;
  }

  return false;
}

bool UniversalTelegramBot::sendChatAction(const char* chat_id, const char* text) {
  bool sent = false;
  if (_debug)
    Serial.println(F("SEND Chat Action Message"));
  unsigned long sttime = millis();

  if (strcmp(text, "") != 0) {
    char command[MAX_CMD_LENGTH]; command[0] = '\0';
    memset(_msg, '\0', MAX_MESSAGE_LENGTH);
    while (millis() < sttime + 8000) { // loop for a while to send the message
      snprintf_P(command, MAX_CMD_LENGTH, "bot%s/sendChatAction?chat_id=%s&action=%s", _token, chat_id, text);
	  command[MAX_CMD_LENGTH-1] = '\0';
      strncpy(_msg, sendGetToTelegram(command), MAX_MESSAGE_LENGTH);
      _msg[MAX_MESSAGE_LENGTH-1] = '\0';
      if (_debug)
        Serial.println(_msg);
      sent = checkForOkResponse(_msg);

      if (sent) {
        break;
      }
    }
  }

  closeClient();
  return sent;
}

void UniversalTelegramBot::closeClient() {
  if (client->connected()) {
    if (_debug) {
      Serial.println(F("Closing client"));
    }
    client->stop();
  }
}
