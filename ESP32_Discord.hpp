
//ESP32によるdiscordBOTを構築するライブラリ

//元のコード https://github.com/pingvortex/ESP32-Discord-bot/blob/main/main/esp32bot.ino

#pragma once
#include <Arduino.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#define WEBSOCKETS_NETWORK_TYPE NETWORK_ESP32

//#include <WebSockets2_Generic.h>
#include <ArduinoJson.h>

// --- SETTINGS ---
const char* token = "TOKEN"; // YOUR BOT TOKEN
const char* wifiName = "SSID"; // YOUR WIFI NAME/SSID
const char* wifiPassword = "PASSWORD"; // YOUR WIFI PASSWORD
const String botName = "BOT"; // WHAT NAME DO YOU WANT TO BE DISPLAYED ON THE DISPLAY (Can be left as BOT)

using namespace websockets2_generic;

WebsocketsClient client;

unsigned long lastHeartbeat = 0;
unsigned long heartbeatInterval = 40000;
unsigned long lastDisplayUpdate = 0;
long lastLatency = 0;
bool isConnected = false;

DynamicJsonDocument doc(3072);

class DISCORDBOT{
  public:

    void begin(char* token, const String botname);

    //メッセージやコマンドのコールバック関数の登録関数
    void SetReceiveCallBack(void* func){
      this->receiveCall = func;
    }

    void onMessageCallback(void* param, WebsocketsMessage message);

  private:
    //websocketやWifiの内部状態管理
    bool isConnected = false;

    char* TOKEN; //BOTのトークン(Discrodの開発者ページから取得可能)

    //Wifi関連は他のライブラリとの競合を避けるため初期化済みのWifiインスタンスをこのライブラリの初期化時に受け取るようにする

    String BOTNAME = "BOT"; //BOTの表示名

    //コールバック関数の格納用

    void* receiveCall;

    //識別情報の操作
    void identify() {
      StaticJsonDocument < 512 > idDoc;
      idDoc["op"] = 2;
      JsonObject d = idDoc.createNestedObject("d");
      d["token"] = token;

      JsonObject prop = d.createNestedObject("properties");
      prop["$os"] = "esp32";
      prop["$browser"] = "esp32-bot";
      prop["$device"] = "esp32";

      // 33280 = GUILD_MESSAGES (512) + MESSAGE_CONTENT
      d["intents"] = 33280;

      String output;
      serializeJson(idDoc, output);
      client.send(output);
    }
    
};

void DISCORDBOT::begin(char* token, const String botname){
  this->TOKEN = token;
  this->BOTNAME = botname;

  //Wifiの接続確認
  while(WiFi.status() != WL_CONNECTED){
    Serial.println("Wifi is not connected...");
    delay(1000);
  }

  client.onMessage(onMessageCallback);
  client.setInsecure();
  client.connect("wss://gateway.discord.gg/?v=10&encoding=json");
}

void DISCORDBOT::onMessageCallback(void* param, websockets2_generic::WebsocketsMessage message) {
  DISCORDBOT* self =  static_cast<DISCORDBOT*>(param);
  if (message.length() > 2500) return;

  doc.clear();
  DeserializationError error = deserializeJson(doc, message.data());
  if (error) return;

  if (doc["t"] == "MESSAGE_CREATE") {
    const char * content = doc["d"]["content"];
    const char * channelId = doc["d"]["channel_id"];
    bool isBot = doc["d"]["author"]["bot"] | false;

    if (content && !isBot) {
      String msg = String(content);

      //ここで受信タスクにメッセージの内容を渡す
      
    }
  }

  int op = doc["op"] | -1;
  if (op == 10) {
    heartbeatInterval = doc["d"]["heartbeat_interval"];
    identify();
    self->isConnected = true;
  }
}

