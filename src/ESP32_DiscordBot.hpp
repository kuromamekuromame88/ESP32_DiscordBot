#pragma once

#include <Arduino.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#include <WebSocketsClient.h>

/*platformio.iniに

extra_scripts = 
  pre:run_gen_script.py

board_build.embed_txtfiles = 
  data/cert/x509_crt_bundle.bin

を書いておくこと
*/
extern const uint8_t rootca_crt_bundle_start[] asm("_binary_data_cert_x509_crt_bundle_bin_start");

#include <ArduinoJson.h>


//フラグ用の変数
const bool FLAG_DEBUG = false;


unsigned long lastHeartbeat = 0;
unsigned long heartbeatInterval = 40000;
long lastLatency = 0;    
bool WSisConnected = false;

//intentsの一覧
namespace DiscordIntent{
  constexpr uint32_t GUILDS                   = 1UL << 0;
  constexpr uint32_t GUILD_MEMBERS            = 1UL << 1;
  constexpr uint32_t GUILD_MODERATION         = 1UL << 2;
  constexpr uint32_t GUILD_EXPRESSIONS        = 1UL << 3;
  constexpr uint32_t GUILD_INTEGRATIONS       = 1UL << 4;
  constexpr uint32_t GUILD_WEBHOOKS           = 1UL << 5;
  constexpr uint32_t GUILD_INVITES            = 1UL << 6;
  constexpr uint32_t GUILD_VOICE_STATES       = 1UL << 7;
  constexpr uint32_t GUILD_PRESENCES          = 1UL << 8;
  constexpr uint32_t GUILD_MESSAGES           = 1UL << 9;
  constexpr uint32_t GUILD_MESSAGE_REACTIONS  = 1UL << 10;
  constexpr uint32_t GUILD_MESSAGE_TYPING     = 1UL << 11;
  constexpr uint32_t DIRECT_MESSAGES          = 1UL << 12;
  constexpr uint32_t DIRECT_MESSAGE_REACTIONS = 1UL << 13;
  constexpr uint32_t DIRECT_MESSAGE_TYPING    = 1UL << 14;
  constexpr uint32_t MESSAGE_CONTENT          = 1UL << 15;
  constexpr uint32_t GUILD_SCHEDULED_EVENTS   = 1UL << 16;
}
using namespace DiscordIntent;


//メッセージ送信関係

//メッセージ送信時に使用する構造体のメッセージのタイプ指定
/*
enum MsgType{
  //末尾に_があるものは配列になるメッセージタイプ
  MSG_Content,
  MSG_Embeds_,
  MSG_Components_,
  MSG_Attachments_,
  MSG_Stickers_,
  MSG_Poll,
  MSG_MessageReference,
  MSG_AllowedMentions,
};

//メッセージ送信時に使用する構造体
struct DiscordMsgData{
  MsgType Msgtype;  
  const char* TextContent;
};*/

//JSON構築用
class DiscordMessage{
  public:
    DiscordMessage& content(const char* content); //通常のテキストメッセージ
    DiscordMessage& reply(const char* msgID);
    DiscordMessage& embed();
    DiscordMessage& components();
    DiscordMessage& attachments();
    DiscordMessage& stickers();
    DiscordMessage& poll();
    DiscordMessage& messageReference();
    DiscordMessage& AllowedMentions();
    
  private:
    JsonDocument Msgpld; //内部で直接Jsonを構築させる

    friend class DiscordBot;
};

DiscordMessage& DiscordMessage::content(const char* content){
  Msgpld["content"] = content;
  return *this;
}


DiscordMessage Message;



//メインクラス
class DiscordBot{
  public:
    bool begin(const char* Token, uint32_t Intent);

    bool send(const char* channelId, DiscordMessage& message);
    bool send(const char* channelId, JsonDocument json);

    void sendIdentify();
    void hexdump(const void * mem, uint32_t len, uint8_t cols = 16);
    
  private:
    char TOKEN[128];
    uint32_t GatewayIntent = 0;

    WebSocketsClient webSocket;

    void setClock();
};
DiscordBot Discord;

//public
bool DiscordBot::begin(const char* Token, uint32_t Intent){
  strncpy(TOKEN, Token, sizeof(TOKEN) - 1);
  TOKEN[sizeof(TOKEN) - 1] = '\0';

  GatewayIntent = Intent;
  
  while(WiFi.status() != WL_CONNECTED){
    Serial.println("Wifi is not connected...");
    delay(500);
  }

  setClock();

  webSocket.beginSslWithBundle("gateway.discord.gg", 443, "/?v=10&encoding=json", rootca_crt_bundle_start, "");
  
  // イベントハンドラー
  webSocket.onEvent(webSocketEvent);

  // 切断時に5秒ごとに再接続
  webSocket.setReconnectInterval(5000);
}

bool DiscordBot::send(const char* channelId, DiscordMessage& message){
  WiFiClientSecure httpsClient;
  httpsClient.setInsecure();
  HTTPClient http;
  String url = "https://discord.com/api/v10/channels/" + String(channelId) + "/messages";
  if (http.begin(httpsClient, url)){
    http.addHeader("Authorization", "Bot " + String(TOKEN));
    http.addHeader("Content-Type", "application/json");
    String payload;
    serializeJson(message.Msgpld, payload);
    int httpCode = http.POST(payload);
    http.end();
    message.Msgpld.clear();
    return true;
  }
  return false;
}

//独自json送信用のオーバーロードのsend
bool DiscordBot::send(const char* channelId, JsonDocument json){
  WiFiClientSecure httpsClient;
  httpsClient.setInsecure();
  HTTPClient http;
  String url = "https://discord.com/api/v10/channels/" + String(channelId) + "/messages";
  if (http.begin(httpsClient, url)){
    http.addHeader("Authorization", "Bot " + String(TOKEN));
    http.addHeader("Content-Type", "application/json");
    String payload;
    serializeJson(json, payload);
    int httpCode = http.POST(payload);
    http.end();
    return true;
  }
  return false;
}

void DiscordBot::sendIdentify(){
  StaticJsonDocument<512> identify;

  identify["op"] = 2;

  JsonObject d = identify.createNestedObject("d");

  d["token"] = this->TOKEN;

  //全てのインテントを設定する圧縮版
  //d["intents"] = (1U<<17)-1;
  d["intents"] = this->GatewayIntent;

  //d["intents"] = GUILD_MESSAGES | (1<<12) | (1<<15);

  JsonObject properties = d.createNestedObject("properties");
  properties["os"] = "linux";
  properties["browser"] = "esp32";
  properties["device"] = "esp32";

  String output;
  serializeJson(identify, output);

  Serial.println("[Discord] Identifyを送信します");
  webSocket.sendTXT(output);
}

void DiscordBot::hexdump(const void * mem, uint32_t len, uint8_t cols = 16){
  const uint8_t * src = (const uint8_t *)mem;
  Serial.printf("\n[HEXDUMP] Address: 0x%08X len: 0x%X (%d)", (ptrdiff_t)src, len, len);
  for(uint32_t i = 0; i < len; i++){
    if(i % cols == 0){
      Serial.printf("\n[0x%08X] 0x%08X: ", (ptrdiff_t)src, i);
    }
    Serial.printf("%02X ", *src);
    src++;
  }
  Serial.printf("\n");
}

//websocketイベントハンドラ
StaticJsonDocument<1024> doc;
DeserializationError error;
int op;
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length){
  switch(type){
    case WStype_DISCONNECTED:
      WSisConnected = false;
      Serial.printf("[WSc] Disconnected!\n");
      break;

    case WStype_CONNECTED:
      WSisConnected = true;
      Serial.printf("[WSc] Connected to url: %s\n", payload);

      // send message to server when Connected
      //webSocket.sendTXT("Connected");
      break;

    case WStype_TEXT:
      error = deserializeJson(doc, payload, length);

      if(error){
        Serial.printf("[Discord] JSON parse error: %s\n", error.c_str());
        break;
      }

      op = doc["op"] | -1;

      if(FLAG_DEBUG) Serial.printf("[WSc] get text: %s\n", payload);
      
      if(op == 10){
        heartbeatInterval = doc["d"]["heartbeat_interval"] | 40000;
        Serial.printf("[Discord] Hello を受け取りました。ハートビートのインターバル: %lu ms\n", heartbeatInterval);
        lastHeartbeat = millis();
        Discord.sendIdentify();
      }else if(op == 0){
        const char* eventName = doc["t"] | "";

        if (strcmp(eventName, "MESSAGE_CREATE") == 0){
          const char* content = doc["d"]["content"] | "";
          const char* author = doc["d"]["author"]["username"] | "";
          const char* channelId = doc["d"]["channel_id"] | "";

          const bool isBot = doc["d"]["author"]["bot"] | false;
          Serial.println("[Discord] Message received!");
          Serial.printf("  Author : %s\n", author);
          Serial.printf("  Channel: %s\n", channelId);
          Serial.printf("  Content: %s\n", content);
          Serial.printf("  isBot  : %s\n", isBot?"true":"false");

          if(!isBot){
            //bot以外のユーザーのメッセージをリピート
            //Discord.sendMessage(channelId, content);
          }
        }
      }else if(op==2){
        //スラッシュコマンドの実行通知

      }
      break;

    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);
      Discord.hexdump(payload, length);
      // send data to server
      // webSocket.sendBIN(payload, length);
      break;

    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;
  }
}

//private

void DiscordBot::setClock(){
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print(F("Waiting for NTP time sync: "));
  time_t nowSecs = time(nullptr);
  while(nowSecs < 8 * 3600 * 2){
    delay(500);
    Serial.print(F("."));
    yield();
    nowSecs = time(nullptr);
  }

  Serial.println();
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  Serial.print(F("現在の時間: "));
  Serial.print(asctime(&timeinfo));
}