#include <M5StickC.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>

// Replace with your network credentials
const char* ssid = "SSID";											// wifi ssid
const char* password = "password";									// wifi password
const int ledPin = 10;

#define BOTtoken "XXXXXXXXXX:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" 	// bot token (use your own here)
#define CHAT_ID "XXXXXXXXXX"                                      	// chat id (use your own here)

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

int currentInfo = 0;

float temp, tempInCelsius, pitch, roll, yaw;

// Checks for new messages every 1 second.
int botRequestDelay = 5000;
unsigned long lastTimeBotRan;

// Handle what happens when you receive new messages
void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/start") {
      String welcome = "Welcome, " + from_name + ".\n";
      welcome += "Use the following commands to control your outputs.\n\n";
      welcome += "/print to print something to the screen\n";
      welcome += "/stats to print stats like power, gps and temperature\n";
      welcome += "/led_on to turn the LED on\n";
      welcome += "/led_off to turn the LED off";
      bot.sendMessage(chat_id, welcome, "");
    }
    
    if (text == "/print") {
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.println("Hello " + from_name);
    }

    if (text == "/stats") {
      String stats = "Power: " + String(M5.Axp.GetBatPower()) + "\n";
      stats += "GPS: \n";
      stats += "   yaw Z " + String(yaw) + "\n";
      stats += "   pitch Y " + String(pitch) + "\n";
      stats += "   roll X " + String(roll) + "\n";
      stats += "Temp: " + String(temp) + " F, ";
      stats += String(tempInCelsius) + " C";
      bot.sendMessage(chat_id, stats, "");
    }

    if (text == "/led_on") {
      bot.sendMessage(chat_id, "LED has been turned on.", "");
      digitalWrite (ledPin, LOW);
      M5.update();
    }
    
    if (text == "/led_off") {
      bot.sendMessage(chat_id, "LED has been turned off.", "");
      digitalWrite (ledPin, HIGH);
      M5.update();
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode (ledPin, OUTPUT);
  digitalWrite (ledPin, HIGH);

  M5.begin();
  M5.IMU.Init();
  Wire.begin(0,26);
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  
  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());
}

void loop() {
  M5.update();

  M5.IMU.getAhrsData(&pitch, &roll, &yaw);
  M5.IMU.getTempData(&temp);
  tempInCelsius = (temp - 30) / 1.8;
    
  if(M5.BtnA.wasReleased()){
    nextInfo();
  }

  checkTelegram();
}

void checkTelegram() {
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
}

void nextInfo() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  switch(currentInfo) {
    case 1 :
      M5.Lcd.printf("Power  %.1f in bat", M5.Axp.GetBatPower());
      break;
    case 2 :
      M5.Lcd.printf("Temp %.2f F, ", temp);
      M5.Lcd.printf("%.2f C", tempInCelsius);
      break;
    case 0 :
      M5.Lcd.printf("yaw Z  %.2f", yaw);
      M5.Lcd.println("");
      M5.Lcd.printf("pitch Y %.2f", pitch);
      M5.Lcd.println("");
      M5.Lcd.printf("roll X  %.2f", roll);
      break;
  }
  if (currentInfo == 2) {
    currentInfo = 0;  
  } else {
    currentInfo += 1;
  }
}
