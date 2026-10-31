#include <Arduino.h>

#include <WiFi.h>
#include "ESP32_DiscordBot.hpp"

void setup(){
  Serial.begin(115200);

  WiFi.begin("ssid", "password");

  pinMode(0, INPUT_PULLUP);
  Discord.begin("");

  //AppID: 1551509750422704140
  //GuildID(ServerID): 1262390704303378442

  Discord.slash_command.add("ping", "ping", nullptr);
  
}

void loop(){
  if(!digitalRead(0)){
    while(digitalRead(0));
    Serial.println("ボタンが押されました");
    //Discord.send("1262390704303378445", DiscordMsg.content("メッセージの送信テスト完了"));//.reply("1554512757649576097"));

    /*
    DiscordEmbed embed;
    embed.title("ESP32").color(0x00ccff).description("埋め込みで送信").image("https://www.innomatter.com/wp-content/uploads/2023/05/DiscordLogo.jpg").url("https://github.com/kuromamekuromame88");
    //Discord.send("1262390704303378445", DiscordMsg.embed(embed));
    DiscordAllowedMentions mentions;
    mentions.users("");
    Discord.send("1262390704303378445", DiscordMsg.content("<@1142796417807683656>").allowedMentions(mentions));
    */

Discord.embed
  .title("ESP32 Test")
  .description("DiscordBot Library Test")
  .color(0x00FF00)
  .field("Temperature", "25.3 °C", true)
  .field("Voltage", "12.1 V", true)
  .field("Status", "OK");

Discord.allowed_mentions
  .users("1142796417807683656")
  .roles("1554865004954787841")
  .repliedUser(false);

Discord.message
  .content("テストメッセージ")
  .reply("1554797217548738581")
  .embed(Discord.embed)
  .allowedMentions(Discord.allowed_mentions);

Discord.send("1262390704303378445", Discord.message);
  }
}