/*
   Avocado Watering Monitor
   Board used: WeMos D1 NodeMcu Lua V3 ESP8266
   Sensor: YL-69 / YL-39 Soil Moisture Sensor
   By g4x - 01/2018
*/

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266TelegramBOT.h>
#include <ArduinoJson.h>

// YL-69 sensor
#define moistureCriticalLevel 400
byte pinMoistureSensor = A0;
byte pinMoistureVCC = 4; // D2
int  moistureValue;

// protothreading
int  moistureReadingInterval = 86400;   // interval in seconds between sensor reading (every 24h)
long moistureLastRead = 0;              // last time the moisture was read

int  botScanInterval = 1;               // interval time between scan messages (seconds)
long botLastScan;                       // last time messages' scan has been done

long nowMillis;

// WiFi connection
const char* ssid     = "POURNAMI";      // wifi name
const char* password = "rajitha4574";      // wifi password

// Telegram bot
#define botMyChatID "2144275777"          // reference to my phone's chat
#define botToken "6127891460:AAEXMwF0g7x-MdRpKl_jMyJu4fkycgN-WU8"
#define botName "Plant Water"
#define botUsername "PlantWater_asking_BOT"
TelegramBOT bot(botToken,botName,botUsername);

class String botCommand;
class String botResponse;


void setup() {
  Serial.begin(9600);
  delay(1000);

  // init the moisture sensor
  pinMode(pinMoistureSensor, INPUT);

  pinMode(pinMoistureVCC, OUTPUT);
  digitalWrite(pinMoistureVCC, LOW);    // by default, we do not power the sensor

  // connect to wifi
  Serial.print("Connecting to \"");
  Serial.print(ssid);
  Serial.println("\"");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected (");
  Serial.print(WiFi.localIP());
  Serial.println(")");
  Serial.println("");

  // start bot
  bot.begin();
}


void loop() {
  checkSoilMoisture();
  handleBotMessages();
}


void checkSoilMoisture() {
  nowMillis = millis();

  if ((nowMillis > moistureLastRead + (moistureReadingInterval * 1000)) or moistureLastRead == 0) {
    moistureLastRead = nowMillis;

    moistureValue = readMoisture();

    if (moistureValue < moistureCriticalLevel) {
      // send value to Telegram chat
      botResponse = "Avocado: Water me! (Humidity Level [0-1023]: ";
      botResponse.concat(moistureValue);
      botResponse.concat(")");
      bot.sendMessage(botMyChatID, botResponse, "");    // send notification to my phone
    }

    Serial.print("*************** Humidity Level (0-1023): ");
    Serial.println(moistureValue);
    Serial.println("");
  }
}


int readMoisture() {
  digitalWrite(pinMoistureVCC, HIGH); // power up sensor

  delay(500);
  int value = analogRead(pinMoistureSensor);

  digitalWrite(pinMoistureVCC, LOW); // power down sensor

  return 1023 - value;
}


/* Check if the bot received any message */
void handleBotMessages() {
  nowMillis = millis();

  if (nowMillis > botLastScan + (botScanInterval * 1000)) {
    botLastScan = millis();

    bot.getUpdates(bot.message[0][1]);   // launch API GetUpdates up to xxx message

    // loop at messages received
    for (int i = 1; i < bot.message[0][0].toInt() + 1; i++)      {
      handleBotCommands(i);
    }
    bot.message[0][0] = "";   // All messages have been replied - reset new messages
  }
}


/* Execute command sent to the bot */
void handleBotCommands(int line) {
  botCommand = bot.message[line][5]; // message reiceived
  botCommand.toUpperCase();  // not case sensitive anymore

  if (botCommand.equals("/READ")) {

    // read data
    moistureValue = readMoisture();

    //botResponse = "Sensor value: ";
    botResponse = "Humidity Level (0-1023): ";
    botResponse.concat(moistureValue);
    if (moistureValue < moistureCriticalLevel) {
      botResponse.concat(" (critical)");
    }

  } else if (botCommand.equals("/HELP")) {

    botResponse = "Allowed commands are:";
    bot.sendMessage(bot.message[line][4], botResponse, "");
    botResponse = "/read - Read soil moisture";
    bot.sendMessage(bot.message[line][4], botResponse, "");
    botResponse = "/ip - Get local IP address";

  } else if (botCommand.equals("START") or botCommand.equals("HI") or botCommand.equals("HELLO") or botCommand.equals("COUCOU") or botCommand.equals("SALUT")) {

    botResponse = "Hello " + bot.message[line][2] + " !";  // user's name
    
  } else if (botCommand.equals("/IP")) {
    
    botResponse = "Local IP address: ";
    botResponse.concat(WiFi.localIP().toString());
    
  } else {

    botResponse = "Unknown command, use /help for command list.";

  }
  bot.sendMessage(bot.message[line][4], botResponse, "");    // bot.message[line][4] is chat ID
}