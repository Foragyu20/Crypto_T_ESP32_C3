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

// ========== COIN DATA CLASS ==========
class CoinData {
public:
  struct Coin {
    const char* name;
    float basePrice;
    float currentPrice;
    String text;
  };

private:
  Coin coins[5] = {
    {"BTC", 116314.0, 0, ""},
    {"ETH", 3841.0, 0, ""},
    {"ADA", 0.7760, 0, ""},
    {"SOL", 172.23, 0, ""},
    {"DOGE", 0.21, 0, ""}
  };
  int currentIndex = 0;

public:
  void initializeCoins() {
    for (int i = 0; i < 5; i++) {
      coins[i].text = String(coins[i].name) + ": Loading...";
    }
  }

  Coin& getCurrentCoin() {
    return coins[currentIndex];
  }

  Coin& getCoin(int index) {
    return coins[index];
  }

  void nextCoin() {
    currentIndex++;
    if (currentIndex >= 5) currentIndex = 0;
  }

  int getCurrentIndex() {
    return currentIndex;
  }

  void updateCoinText(int index, float price) {
    if (price > 0) {
      int decimals = (strcmp(coins[index].name, "ADA") == 0 || strcmp(coins[index].name, "DOGE") == 0) ? 4 : 2;
      coins[index].text = String(coins[index].name) + ": $" + String(price, decimals) + " " + formatChange(price, coins[index].basePrice);
    }
  }

private:
  String formatChange(float current, float base) {
    if (base == 0) return "N/A";
    float change = ((current - base) / base) * 100.0;
    return (change >= 0 ? "+" : "") + String(change, 2) + "%";
  }
};

// ========== WIFI MANAGER CLASS ==========
class WiFiManager {
private:
  const char* ssid;
  const char* password;
  unsigned long lastCheck = 0;
  const unsigned long checkInterval = 30000;

public:
  WiFiManager(const char* wifiSSID, const char* wifiPassword) 
    : ssid(wifiSSID), password(wifiPassword) {}

  void connect() {
    WiFi.begin(ssid, password);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 10);
    display.println("Connecting WiFi...");
    display.display();

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
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

  void checkConnection() {
    unsigned long now = millis();
    if (now - lastCheck > checkInterval) {
      lastCheck = now;
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected, attempting reconnection...");
        connect();
      }
    }
  }

  bool isConnected() {
    return WiFi.status() == WL_CONNECTED;
  }
};

// ========== DATA FETCHER CLASS ==========
class DataFetcher {
private:
  const char* apiUrl;
  unsigned long lastFetch = 0;
  const unsigned long fetchInterval = 60000;

public:
  DataFetcher(const char* url) : apiUrl(url) {}

  void updatePrices(CoinData& coinData, WiFiManager& wifi) {
    unsigned long now = millis();
    if (now - lastFetch > fetchInterval) {
      lastFetch = now;
      Serial.println("Fetching new prices...");
      fetchPrices(coinData, wifi);
    }
  }

private:
  void fetchPrices(CoinData& coinData, WiFiManager& wifi) {
    if (!wifi.isConnected()) {
      Serial.println("WiFi not connected, attempting reconnect...");
      wifi.connect();
      return;
    }

    HTTPClient http;
    http.begin(apiUrl);
    http.setTimeout(10000);
    
    int code = http.GET();
    if (code == 200) {
      String payload = http.getString();
      
      StaticJsonDocument<2048> doc;
      DeserializationError error = deserializeJson(doc, payload);
      
      if (error) {
        Serial.println("JSON parsing failed");
        http.end();
        return;
      }
      
      if (doc["bitcoin"]["usd"]) {
        coinData.getCoin(0).currentPrice = doc["bitcoin"]["usd"];
        coinData.updateCoinText(0, coinData.getCoin(0).currentPrice);
      }
      if (doc["ethereum"]["usd"]) {
        coinData.getCoin(1).currentPrice = doc["ethereum"]["usd"];
        coinData.updateCoinText(1, coinData.getCoin(1).currentPrice);
      }
      if (doc["cardano"]["usd"]) {
        coinData.getCoin(2).currentPrice = doc["cardano"]["usd"];
        coinData.updateCoinText(2, coinData.getCoin(2).currentPrice);
      }
      if (doc["solana"]["usd"]) {
        coinData.getCoin(3).currentPrice = doc["solana"]["usd"];
        coinData.updateCoinText(3, coinData.getCoin(3).currentPrice);
      }
      if (doc["dogecoin"]["usd"]) {
        coinData.getCoin(4).currentPrice = doc["dogecoin"]["usd"];
        coinData.updateCoinText(4, coinData.getCoin(4).currentPrice);
      }
      
      Serial.println("Prices updated successfully");
    } else {
      Serial.print("HTTP Error: ");
      Serial.println(code);
    }
    http.end();
  }
};

// ========== SCROLLING ANIMATION CLASS ==========
class ScrollingAnimation {
private:
  int16_t scrollY = -16;
  unsigned long lastScrollTime = 0;
  const int scrollSpeed = 100;

public:
  void init() {
    scrollY = -16;
    lastScrollTime = millis();
  }

  void update() {
    unsigned long now = millis();
    if (now - lastScrollTime > scrollSpeed) {
      lastScrollTime = now;
      scrollY += 1;
      if (scrollY > SCREEN_HEIGHT + 20) {
        scrollY = -60;
      }
    }
  }

  void display(CoinData& coinData, WiFiManager& wifi) {
    display.clearDisplay();
    
    // Main crypto info
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, scrollY);
    display.print(coinData.getCurrentCoin().name);
    
    display.setCursor(0, scrollY + 20);
    String priceStr = "$" + String(coinData.getCurrentCoin().currentPrice, 
                                   (strcmp(coinData.getCurrentCoin().name, "ADA") == 0 || 
                                    strcmp(coinData.getCurrentCoin().name, "DOGE") == 0) ? 4 : 2);
    display.print(priceStr);
    
    display.setCursor(0, scrollY + 40);
    String changeStr = coinData.getCurrentCoin().text;
    changeStr = changeStr.substring(changeStr.indexOf(' ') + 1);
    changeStr = changeStr.substring(changeStr.indexOf(' ') + 1);
    display.print(changeStr);
    
    // Status indicators
    display.setTextSize(1);
    if (wifi.isConnected()) {
      display.setCursor(SCREEN_WIDTH - 24, 0);
      display.print("WiFi");
    } else {
      display.setCursor(SCREEN_WIDTH - 18, 0);
      display.print("X");
    }
    
    display.setCursor(0, 0);
    display.print((coinData.getCurrentIndex() + 1));
    display.print("/5");
    
    display.display();
  }

  void resetOnSwitch() {
    scrollY = -16;
  }
};

// ========== POPUP ANIMATION CLASS ==========
class PopupAnimation {
private:
  bool animatingIn = true;
  unsigned long animationStartTime = 0;
  const unsigned long animationDuration = 500;
  int animationY = 0;

  float easeOutQuart(float t) {
    return 1 - pow(1 - t, 4);
  }

public:
  void init() {
    animatingIn = true;
    animationStartTime = millis();
  }

  void display(CoinData& coinData, WiFiManager& wifi) {
    display.clearDisplay();
    
    unsigned long now = millis();
    float progress = 0;
    
    if (animatingIn) {
      progress = (float)(now - animationStartTime) / animationDuration;
      if (progress >= 1.0) {
        progress = 1.0;
        animatingIn = false;
      }
      progress = easeOutQuart(progress);
    } else {
      progress = 1.0;
    }
    
    int targetY = 16;
    animationY = (int)(targetY * progress - (1.0 - progress) * 30);
    
    // Status indicators
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    if (wifi.isConnected()) {
      display.setCursor(SCREEN_WIDTH - 24, 0);
      display.print("WiFi");
    } else {
      display.setCursor(SCREEN_WIDTH - 18, 0);
      display.print("X");
    }
    
    display.setCursor(0, 0);
    display.print((coinData.getCurrentIndex() + 1));
    display.print("/5");
    
    // Main coin display with animation
    display.setTextSize(2);
    display.setCursor(0, animationY);
    display.print(coinData.getCurrentCoin().name);
    
    display.setCursor(0, animationY + 16);
    String priceStr = "$" + String(coinData.getCurrentCoin().currentPrice, 
                                   (strcmp(coinData.getCurrentCoin().name, "ADA") == 0 || 
                                    strcmp(coinData.getCurrentCoin().name, "DOGE") == 0) ? 4 : 2);
    display.print(priceStr);
    
    display.setTextSize(1);
    display.setCursor(0, animationY + 36);
    String changeStr = coinData.getCurrentCoin().text;
    changeStr = changeStr.substring(changeStr.indexOf(' ') + 1);
    changeStr = changeStr.substring(changeStr.indexOf(' ') + 1);
    display.print("Change: ");
    display.print(changeStr);
    
    display.display();
  }

  void triggerOnSwitch() {
    animatingIn = true;
    animationStartTime = millis();
  }
};

// ========== COIN SWITCHER CLASS ==========
class CoinSwitcher {
private:
  unsigned long lastSwitchTime = 0;
  const unsigned long switchInterval = 10000;

public:
  void init() {
    lastSwitchTime = millis();
  }

  bool shouldSwitch() {
    unsigned long now = millis();
    if (now - lastSwitchTime > switchInterval) {
      lastSwitchTime = now;
      return true;
    }
    return false;
  }

  void handleSwitch(CoinData& coinData, ScrollingAnimation& scrollAnim, PopupAnimation& popupAnim) {
    if (shouldSwitch()) {
      coinData.nextCoin();
      
      // CHOOSE ONE: Comment out the animation you don't want to use
      scrollAnim.resetOnSwitch();    // For scrolling animation
      // popupAnim.triggerOnSwitch(); // For popup animation (uncomment this and comment above)
      
      Serial.print("Switching to coin: ");
      Serial.println(coinData.getCurrentCoin().name);
    }
  }
};

// ========== GLOBAL OBJECTS ==========
CoinData coinData;
WiFiManager wifiManager("SET_YOUR_WIFI_HERE", "YOUR_WIFI_PASSWORD");
DataFetcher dataFetcher("https://api.coingecko.com/api/v3/simple/price?ids=bitcoin,ethereum,cardano,solana,dogecoin&vs_currencies=usd");
ScrollingAnimation scrollingAnim;
PopupAnimation popupAnim;
CoinSwitcher coinSwitcher;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
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

  // Initialize all modules
  wifiManager.connect();
  coinData.initializeCoins();
  coinSwitcher.init();
  
  // CHOOSE ONE: Comment out the animation you don't want to use
  scrollingAnim.init();    // For scrolling animation
  // popupAnim.init();     // For popup animation (uncomment this and comment above)
  
  dataFetcher.updatePrices(coinData, wifiManager);
}

void loop() {
  // Update systems
  dataFetcher.updatePrices(coinData, wifiManager);
  wifiManager.checkConnection();
  coinSwitcher.handleSwitch(coinData, scrollingAnim, popupAnim);
  
  // CHOOSE ONE: Comment out the animation you don't want to use
  scrollingAnim.update();                           // For scrolling animation
  scrollingAnim.display(coinData, wifiManager);     // For scrolling animation
  // popupAnim.display(coinData, wifiManager);      // For popup animation (uncomment this and comment above two)
  // delay(16);                                     // For popup animation (uncomment this and comment above two)
}