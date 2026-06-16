/*
  Project HomeSeismograph (with Arduino)
  Authors: Islam Yekiya and Nurtas Nazarbayev

  Hardware:
  - Arduino Nano or Arduino Uno
  - MPU6050 accelerometer/gyroscope, I2C
  - 0.96 inch SSD1306 OLED display, I2C, 128x64
  - Buzzer on D11
  - Red LED on D12 through resistor

  Required libraries from Arduino Library Manager:
  - Adafruit MPU6050
  - Adafruit Unified Sensor
  - Adafruit SSD1306
  - Adafruit GFX Library

  Notes:
  - OLED SDA/SCL and MPU6050 SDA/SCL are connected together to the Arduino I2C bus.
  - Arduino Uno/Nano I2C: SDA = A4, SCL = A5.
  - The shown "Magnitude" is a relative vibration index, not an official earthquake magnitude scale.
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// -------------------- Pin settings --------------------
const int BUZZER_PIN = 11;
const int LED_PIN    = 12;

// -------------------- OLED settings --------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_MPU6050 mpu;

// -------------------- Vibration settings --------------------
// This threshold is for the relative vibration index shown on the OLED.
// If it is too sensitive, increase it. If it is not sensitive enough, decrease it.
const float ALERT_THRESHOLD = 2.0;

// Graph range. Higher value = graph becomes less sensitive.
const float GRAPH_MAX_VALUE = 5.0;

// Sampling interval in milliseconds.
const unsigned long SAMPLE_INTERVAL_MS = 40;

// Baseline acceleration magnitude measured during startup.
float baselineG = 1.0;

// Graph buffer for 128 OLED columns.
uint8_t graphData[SCREEN_WIDTH];

unsigned long lastSampleTime = 0;

// --------------------------------------------------------

float getTotalAccelerationG() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float ax = a.acceleration.x;
  float ay = a.acceleration.y;
  float az = a.acceleration.z;

  // Convert m/s^2 vector length to g units.
  float total = sqrt(ax * ax + ay * ay + az * az) / 9.80665;
  return total;
}

float calibrateBaseline() {
  const int samples = 80;
  float sum = 0;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("HomeSeismograph");
  display.println("Calibrating...");
  display.println("Keep device still");
  display.display();

  delay(500);

  for (int i = 0; i < samples; i++) {
    sum += getTotalAccelerationG();
    delay(20);
  }

  return sum / samples;
}

void shiftGraph(uint8_t value) {
  for (int i = 0; i < SCREEN_WIDTH - 1; i++) {
    graphData[i] = graphData[i + 1];
  }
  graphData[SCREEN_WIDTH - 1] = value;
}

void drawInterface(float vibrationIndex, bool alert) {
  display.clearDisplay();

  // Header
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("HomeSeismograph");

  display.setCursor(92, 0);
  if (alert) {
    display.print("ALERT");
  } else {
    display.print("OK");
  }

  // Magnitude value
  display.setCursor(0, 10);
  display.print("Magnitude: ");
  display.print(vibrationIndex, 2);

  display.setCursor(0, 20);
  display.print("Threshold: ");
  display.print(ALERT_THRESHOLD, 1);

  // Graph area
  const int graphTop = 30;
  const int graphBottom = 63;
  const int graphHeight = graphBottom - graphTop;

  display.drawLine(0, graphBottom, SCREEN_WIDTH - 1, graphBottom, SSD1306_WHITE);
  display.drawLine(0, graphTop, 0, graphBottom, SSD1306_WHITE);

  for (int x = 1; x < SCREEN_WIDTH; x++) {
    int y1 = graphBottom - graphData[x - 1];
    int y2 = graphBottom - graphData[x];
    display.drawLine(x - 1, y1, x, y2, SSD1306_WHITE);
  }

  // Draw a small vertical bar at the right side for current value.
  int barHeight = map(constrain((int)(vibrationIndex * 100), 0, (int)(GRAPH_MAX_VALUE * 100)),
                      0, (int)(GRAPH_MAX_VALUE * 100), 0, graphHeight);
  display.fillRect(122, graphBottom - barHeight, 5, barHeight, SSD1306_WHITE);

  display.display();
}

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  noTone(BUZZER_PIN);

  Serial.begin(9600);
  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    // If OLED is not found, stop here.
    while (true) {
      digitalWrite(LED_PIN, HIGH);
      delay(200);
      digitalWrite(LED_PIN, LOW);
      delay(200);
    }
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("HomeSeismograph");
  display.println("Starting...");
  display.display();

  if (!mpu.begin()) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("MPU6050 not found");
    display.println("Check wiring:");
    display.println("VCC GND SDA SCL");
    display.display();

    while (true) {
      digitalWrite(LED_PIN, HIGH);
      tone(BUZZER_PIN, 1200);
      delay(150);
      digitalWrite(LED_PIN, LOW);
      noTone(BUZZER_PIN);
      delay(150);
    }
  }

  // Sensor configuration.
  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  for (int i = 0; i < SCREEN_WIDTH; i++) {
    graphData[i] = 0;
  }

  baselineG = calibrateBaseline();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Ready");
  display.print("Baseline G: ");
  display.println(baselineG, 3);
  display.display();
  delay(1000);
}

void loop() {
  unsigned long now = millis();
  if (now - lastSampleTime < SAMPLE_INTERVAL_MS) {
    return;
  }
  lastSampleTime = now;

  float totalG = getTotalAccelerationG();

  // Relative vibration index.
  // Small acceleration changes around the baseline become a readable value on OLED.
  float vibrationG = abs(totalG - baselineG);
  float vibrationIndex = vibrationG * 10.0;

  bool alert = vibrationIndex >= ALERT_THRESHOLD;

  if (alert) {
    digitalWrite(LED_PIN, HIGH);
    tone(BUZZER_PIN, 1500);
  } else {
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN);
  }

  int graphHeight = 33;
  int graphValue = map(constrain((int)(vibrationIndex * 100), 0, (int)(GRAPH_MAX_VALUE * 100)),
                       0, (int)(GRAPH_MAX_VALUE * 100), 0, graphHeight);
  shiftGraph((uint8_t)graphValue);

  drawInterface(vibrationIndex, alert);

  Serial.print("totalG=");
  Serial.print(totalG, 3);
  Serial.print(" baselineG=");
  Serial.print(baselineG, 3);
  Serial.print(" vibrationIndex=");
  Serial.print(vibrationIndex, 2);
  Serial.print(" alert=");
  Serial.println(alert ? "YES" : "NO");
}
