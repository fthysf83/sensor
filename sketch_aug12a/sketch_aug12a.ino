#define BLYNK_TEMPLATE_ID "********"
#define BLYNK_TEMPLATE_NAME "Assignment3"
#define BLYNK_AUTH_TOKEN "********"
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClientSecure.h> // For HTTPS communication
#include <BlynkSimpleEsp32.h>
#include <UniversalTelegramBot.h>
#include <DHT.h>

// Wi-Fi credentials
char ssid[] = "Yusofhasmah@unifi";  // Replace with your Wi-Fi network name
char pass[] = "fatihahyusoff21";     // Replace with your Wi-Fi password

// DHT11 Sensor Configuration
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// MQ135 Sensor Configuration
#define MQ135PIN 34

BlynkTimer timer;

// Telegram Bot settings
const String botToken = "****";  // Your bot token
const String chatID = "*******";  // Your chat ID
WiFiClientSecure client;  // For secure communication with Telegram
UniversalTelegramBot bot(botToken, client);

float temperature;
float humidity;
int mq135Value;

// Function to send a message via Telegram with retry logic
void sendTelegramMessage(String message) {
  bool sent = false;
  for (int i = 0; i < 3; i++) {  // Try 3 times
    sent = bot.sendMessage(chatID, message, "");
    if (sent) {
      break;
    } else {
      delay(5000);  // Wait for 5 seconds before retrying
    }
  }
}

// Function to handle incoming Telegram messages
void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;

    if (text == "/status") {
      String statusMessage = "Temperature: " + String(temperature) + " °C\n";
      statusMessage += "Humidity: " + String(humidity) + " %\n";
      statusMessage += "Air Quality (MQ135): " + String(mq135Value);
      bot.sendMessage(chat_id, statusMessage, "");
    } else if (text == "/read") {
      Blynk.syncVirtual(V1);  // Retrieve the last stored data
      String storedMessage = "Stored Data:\n";
      storedMessage += "Temperature: " + String(temperature) + " °C\n";
      storedMessage += "Humidity: " + String(humidity) + " %\n";
      storedMessage += "Air Quality (MQ135): " + String(mq135Value);
      bot.sendMessage(chat_id, storedMessage, "");
    }
  }
}

// *** Data Collection Method ***
void sendSensorData() {
  humidity = dht.readHumidity();  // Read humidity from DHT11 sensor
  temperature = dht.readTemperature();  // Read temperature from DHT11 sensor
  mq135Value = analogRead(MQ135PIN);  // Read air quality from MQ135 sensor

  // *** Data Processing Algorithms ***
  if (isnan(humidity) || isnan(temperature)) {  // Validation
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // *** Data Storage Implementation ***
  Blynk.virtualWrite(V1, temperature);  // Store temperature in Virtual Pin V1
  Blynk.virtualWrite(V2, humidity);  // Store humidity in Virtual Pin V2
  Blynk.virtualWrite(V3, mq135Value);  // Store air quality in Virtual Pin V3

  // Print data to Serial Monitor
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C, Humidity: ");
  Serial.print(humidity);
  Serial.print(" %, MQ135: ");
  Serial.println(mq135Value);

  // *** Analysis Techniques ***
  if (temperature > 28) {  // Threshold-Based Alert: High Temperature
    sendTelegramMessage("Warning: High Temperature! Current temperature is " + String(temperature) + " °C. Stay hydrated. Seek shade.");
  } else if (temperature < 28) {
    sendTelegramMessage("Good Temperature. Current temperature is " + String(temperature) + " °C. Enjoy the weather. Go outside.");
  }

  if (humidity < 20) {  // Threshold-Based Alert: Low Humidity
    sendTelegramMessage("Warning: Low Humidity! Current humidity is " + String(humidity) + " %. Use moisturizer. Stay hydrated.");
  } else if (humidity > 40) {
    sendTelegramMessage("Moderate Humidity. Current humidity is " + String(humidity) + " %. Maintain comfort. Continue regular activities.");
  } 

  if (mq135Value > 2000) {  // Threshold-Based Alert: Poor Air Quality
    sendTelegramMessage("Warning: Poor Air Quality! Current MQ135 value is " + String(mq135Value) + ". Avoid outdoor activities. Use air purifier.");
  } else {
    sendTelegramMessage("Good Air Quality. Current MQ135 value is " + String(mq135Value) + ". Breathe easily. Enjoy the outdoors.");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");

  // Connect to Wi-Fi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  // Initialize Blynk and DHT sensor
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  dht.begin();

  // Initialize Telegram bot
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);  // Add root certificate for api.telegram.org
  bot.longPoll = 60; // Optional: Set long polling timeout

  // Set up timer to send sensor data every 10 seconds
  timer.setInterval(10000L, sendSensorData);
}

void loop() {
  Blynk.run();
  timer.run();

  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

  while(numNewMessages) {
    handleNewMessages(numNewMessages);
    numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  }
}

BLYNK_WRITE(V1) {
  temperature = param.asFloat();
}

BLYNK_WRITE(V2) {
  humidity = param.asFloat();
}

BLYNK_WRITE(V3) {
  mq135Value = param.asInt();
}
