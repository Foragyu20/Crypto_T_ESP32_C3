#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SDA_PIN 3
#define SCL_PIN 4

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* ssid = "SET_YOUR_WIFI_HERE";
const char* password = "YOUR_WIFI_PASSWORD";

const char* apiUrl = "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin,ethereum,cardano,solana,dogecoin&vs_currencies=usd";

struct Coin {
  const char* name;
  float basePrice;
  float currentPrice;
  String text;
};

Coin coins[5] = {
  {"BTC", 116314.0, 0, ""},
  {"ETH", 3841.0, 0, ""},
  {"ADA", 0.7760, 0, ""},
  {"SOL", 172.23, 0, ""},
  {"DOGE", 0.21, 0, ""}
};

int currentCoinIndex = 0;
unsigned long lastSwitchTime = 0;
const unsigned long switchInterval = 10000; // 10 seconds per coin

int16_t scrollY = -16; // Start above screen
unsigned long lastScrollTime = 0;
const int scrollSpeed = 100; // milliseconds between scroll steps

String formatChange(float current, float base) {
  if (base == 0) return "N/A"; // Prevent division by zero
  float change = ((current - base) / base) * 100.0;
  return (change >= 0 ? "+" : "") + String(change, 2) + "%";
}

void connectWiFi() {
  WiFi.begin(ssid, password);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println("Connecting WiFi...");
  display.display();

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) { // Add timeout
    delay(500);
    display.print(".");
    display.display();
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    display.clearDisplay();
    display.setCursor(0, 10);
    display.println("WiFi Connected!");
    display.setCursor(0, 20);
    display.print("IP: ");
    display.println(WiFi.localIP());
    display.display();
    delay(2000);
  } else {
    display.clearDisplay();
    display.setCursor(0, 10);
    display.println("WiFi Failed!");
    display.display();
    delay(2000);
  }
}

void fetchPrices() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, attempting reconnect...");
    connectWiFi();
    return;
  }

  HTTPClient http;
  http.begin(apiUrl);
  http.setTimeout(10000); // 10 second timeout
  
  int code = http.GET();
  if (code == 200) {
    String payload = http.getString();
    
    // Use StaticJsonDocument for better memory management
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
      Serial.println("JSON parsing failed");
      http.end();
      return;
    }

    // Check if data exists before accessing
    if (doc["bitcoin"]["usd"]) coins[0].currentPrice = doc["bitcoin"]["usd"];
    if (doc["ethereum"]["usd"]) coins[1].currentPrice = doc["ethereum"]["usd"];
    if (doc["cardano"]["usd"]) coins[2].currentPrice = doc["cardano"]["usd"];
    if (doc["solana"]["usd"]) coins[3].currentPrice = doc["solana"]["usd"];
    if (doc["dogecoin"]["usd"]) coins[4].currentPrice = doc["dogecoin"]["usd"];

    // Prepare text for each coin
    for (int i = 0; i < 5; i++) {
      if (coins[i].currentPrice > 0) { // Only update if we have valid data
        int decimals = (strcmp(coins[i].name, "ADA") == 0 || strcmp(coins[i].name, "DOGE") == 0) ? 4 : 2;
        coins[i].text = String(coins[i].name) + ": $" + String(coins[i].currentPrice, decimals) + " " + formatChange(coins[i].currentPrice, coins[i].basePrice) + "   ";
      }
    }
    
    Serial.println("Prices updated successfully");
  } else {
    Serial.print("HTTP Error: ");
    Serial.println(code);
  }
  http.end();
}

void setup() {
  Serial.begin(115200);
  delay(1000); // Give serial time to initialize
  
  Serial.println("Starting Crypto Ticker...");
  
  Wire.begin(SDA_PIN, SCL_PIN);
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed");
    while (1) {
      delay(1000);
    }
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println("Crypto Ticker");
  display.setCursor(0, 20);
  display.println("Booting...");
  display.display();
  delay(1000);

  connectWiFi();
  
  // Initialize coin text with default messages
  for (int i = 0; i < 5; i++) {
    coins[i].text = String(coins[i].name) + ": Loading...   ";
  }
  
  fetchPrices();
  lastSwitchTime = millis();
}

void loop() {
  unsigned long now = millis();

  // Update prices every 60 seconds
  static unsigned long lastFetch = 0;
  if (now - lastFetch > 60000) {
    lastFetch = now;
    Serial.println("Fetching new prices...");
    fetchPrices();
  }

  // Switch coin display every 10 seconds
  if (now - lastSwitchTime > switchInterval) {
    lastSwitchTime = now;
    currentCoinIndex++;
    if (currentCoinIndex >= 5) currentCoinIndex = 0;
    scrollY = -16; // Reset to start above screen
    Serial.print("Switching to coin: ");
    Serial.println(coins[currentCoinIndex].name);
  }

  // Scroll current coin text vertically
  if (now - lastScrollTime > scrollSpeed) {
    lastScrollTime = now;

    display.clearDisplay();
    
    // Main crypto info - larger text
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, scrollY);
    display.print(coins[currentCoinIndex].name);
    
    // Price on next line
    display.setCursor(0, scrollY + 20);
    String priceStr = "$" + String(coins[currentCoinIndex].currentPrice, 
                                   (strcmp(coins[currentCoinIndex].name, "ADA") == 0 || 
                                    strcmp(coins[currentCoinIndex].name, "DOGE") == 0) ? 4 : 2);
    display.print(priceStr);
    
    // Percentage change on next line
    display.setCursor(0, scrollY + 40);
    String changeStr = formatChange(coins[currentCoinIndex].currentPrice, coins[currentCoinIndex].basePrice);
    display.print(changeStr);
    
    // WiFi status indicator (small, top-right)
    display.setTextSize(1);
    if (WiFi.status() == WL_CONNECTED) {
      display.setCursor(SCREEN_WIDTH - 24, 0);
      display.print("WiFi");
    } else {
      display.setCursor(SCREEN_WIDTH - 18, 0);
      display.print("X");
    }
    
    // Coin indicator (small, top-left)
    display.setCursor(0, 0);
    display.print((currentCoinIndex + 1));
    display.print("/5");
    
    display.display();

    scrollY += 1; // Move down 1 pixel at a time
    
    // Reset when text has scrolled past bottom of screen
    if (scrollY > SCREEN_HEIGHT + 20) {
      scrollY = -60; // Start well above screen to show full cycle
    }
  }
  
  // Handle WiFi reconnection if needed
  static unsigned long lastWiFiCheck = 0;
  if (now - lastWiFiCheck > 30000) { // Check WiFi every 30 seconds
    lastWiFiCheck = now;
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected, attempting reconnection...");
      connectWiFi();
    }
  }
}
