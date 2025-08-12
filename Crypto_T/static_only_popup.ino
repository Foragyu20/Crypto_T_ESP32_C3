void loop() {
  unsigned long now = millis();

  // Update prices every 10 minutes
  static unsigned long lastFetch = 0;
  if (now - lastFetch > 600000) {
    lastFetch = now;
    fetchPrices();
  }

  // Switch coin display every 10 seconds
  static bool redraw = true;
  if (now - lastSwitchTime > switchInterval) {
    lastSwitchTime = now;
    currentCoinIndex++;
    if (currentCoinIndex >= 5) currentCoinIndex = 0;
    redraw = true; // Mark that we need to update screen
  }

  if (redraw) {
    redraw = false;
    display.clearDisplay();

    // Main crypto info
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 16);
    display.print(coins[currentCoinIndex].name);

    // Price
    display.setCursor(0, 36);
    String priceStr = "$" + String(coins[currentCoinIndex].currentPrice, 
                                   (strcmp(coins[currentCoinIndex].name, "ADA") == 0 || 
                                    strcmp(coins[currentCoinIndex].name, "DOGE") == 0) ? 4 : 2);
    display.print(priceStr);

    // Change %
    display.setCursor(0, 52);
    display.setTextSize(1);
    display.print(formatChange(coins[currentCoinIndex].currentPrice, coins[currentCoinIndex].basePrice));

    // WiFi indicator
    if (WiFi.status() == WL_CONNECTED) {
      display.setCursor(SCREEN_WIDTH - 24, 0);
      display.print("WiFi");
    } else {
      display.setCursor(SCREEN_WIDTH - 18, 0);
      display.print("X");
    }

    // Coin indicator
    display.setCursor(0, 0);
    display.print((currentCoinIndex + 1));
    display.print("/5");

    display.display();
  }

  // WiFi reconnection check
  static unsigned long lastWiFiCheck = 0;
  if (now - lastWiFiCheck > 30000) {
    lastWiFiCheck = now;
    if (WiFi.status() != WL_CONNECTED) connectWiFi();
  }
}