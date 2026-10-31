#include <Arduino.h>

#include <WiFi.h>


#include "ESP32_DiscordBot.hpp"

const char token[] = "TOKEN";

void setup(){
  Discord.begin(token, (1<<17)-1);
  WiFi.begin("aterm-e26bca-g", "05a7282157314");

}

void loop(){

}