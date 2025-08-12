// Animation variables for popup effect
bool animatingIn = true;
unsigned long animationStartTime = 0;
const unsigned long animationDuration = 500; // 500ms animation
int animationY = 0;

// Easing function for smooth animation
float easeOutQuart(float t) {
  return 1 - pow(1 - t, 4);
}

void displayCoin() {
  display.clearDisplay();
  
  // Calculate animation progress
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
  
  // Calculate Y position based on animation
  int targetY = 16; // Center position
  animationY = (int)(targetY * progress - (1.0 - progress) * 30); // Start from above
  
  // Your display content here - use animationY for Y positioning
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, animationY);
  display.print("Your Content");
  
  display.display();
}

// In your main loop, when switching content:
// Start new popup animation
animatingIn = true;
animationStartTime = millis();