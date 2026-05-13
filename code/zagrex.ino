/*
 * ============================================================
 *  ZAGREX — High-Power RC Combat Robot
 *  AUK Robotics Competition — Fall 2025
 *  ETE Department | The American University of Kurdistan
 * ------------------------------------------------------------
 *  Hardware:
 *    • ESP32 WROOM
 *    • 2× BTS7960 43A Motor Driver
 *    • 4× JGB37-545 12V DC Gear Motors (1000 RPM)
 *    • 11.1V LiPo (3S) via XT60 Connector
 *    • DC-DC 10A Step-Down Converter (→ 5V for ESP32)
 *  Control:
 *    • Dabble Bluetooth App (GamePad module)
 * ============================================================
 */

#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE
#include <DabbleESP32.h>

// ─────────────────────────────────────────
//  BTS7960 Driver A  →  LEFT motors (Front-Left + Rear-Left)
// ─────────────────────────────────────────
#define L_RPWM   25   // Forward PWM
#define L_LPWM   26   // Backward PWM
#define L_R_EN   27   // Right Enable
#define L_L_EN   14   // Left Enable

// ─────────────────────────────────────────
//  BTS7960 Driver B  →  RIGHT motors (Front-Right + Rear-Right)
// ─────────────────────────────────────────
#define R_RPWM   32   // Forward PWM
#define R_LPWM   33   // Backward PWM
#define R_R_EN   18   // Right Enable
#define R_L_EN   19   // Left Enable

// ─────────────────────────────────────────
//  PWM Config
// ─────────────────────────────────────────
#define PWM_FREQ        20000   // 20 kHz (above hearing range)
#define PWM_RESOLUTION  8       // 8-bit: 0–255

#define CH_L_FWD  0
#define CH_L_BWD  1
#define CH_R_FWD  2
#define CH_R_BWD  3

// ─────────────────────────────────────────
//  Speed Settings
// ─────────────────────────────────────────
#define FULL_SPEED   240   // out of 255 (leaves headroom)
#define TURN_SPEED   180   // tighter control for turning

// ─────────────────────────────────────────
//  Optional Status LED
// ─────────────────────────────────────────
#define LED_PIN  2   // built-in LED on most ESP32 boards

// ─────────────────────────────────────────
//  Forward declarations
// ─────────────────────────────────────────
void moveForward();
void moveBackward();
void turnLeft();
void turnRight();
void stopAll();
void setLeft(int pwm);
void setRight(int pwm);

// ════════════════════════════════════════
void setup() {
  Serial.begin(115200);

  // Enable pins
  pinMode(L_R_EN, OUTPUT); digitalWrite(L_R_EN, HIGH);
  pinMode(L_L_EN, OUTPUT); digitalWrite(L_L_EN, HIGH);
  pinMode(R_R_EN, OUTPUT); digitalWrite(R_R_EN, HIGH);
  pinMode(R_L_EN, OUTPUT); digitalWrite(R_L_EN, HIGH);

  // PWM channels — Left side
  ledcSetup(CH_L_FWD, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(L_RPWM, CH_L_FWD);

  ledcSetup(CH_L_BWD, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(L_LPWM, CH_L_BWD);

  // PWM channels — Right side
  ledcSetup(CH_R_FWD, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(R_RPWM, CH_R_FWD);

  ledcSetup(CH_R_BWD, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(R_LPWM, CH_R_BWD);

  // LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);   // ON = Bluetooth ready

  stopAll();

  // Start Dabble — name must match what Dabble app scans
  Dabble.begin("ZAGREX");

  Serial.println("ZAGREX ready. Waiting for Dabble...");
}

// ════════════════════════════════════════
void loop() {
  Dabble.processInput();

  // ── Movement ──────────────────────────
  if      (GamePad.isUpPressed())    moveForward();
  else if (GamePad.isDownPressed())  moveBackward();
  else if (GamePad.isLeftPressed())  turnLeft();
  else if (GamePad.isRightPressed()) turnRight();
  else                               stopAll();

  // ── Boost (Triangle = full power burst) ─
  // Hold Triangle + direction for max speed override
  if (GamePad.isTrianglePressed()) {
    if      (GamePad.isUpPressed())   { setLeft(255); setRight(255); }
    else if (GamePad.isDownPressed()) {
      ledcWrite(CH_L_FWD, 0); ledcWrite(CH_R_FWD, 0);
      ledcWrite(CH_L_BWD, 255); ledcWrite(CH_R_BWD, 255);
    }
  }

  // Debug output
  Serial.print("U:"); Serial.print(GamePad.isUpPressed());
  Serial.print(" D:"); Serial.print(GamePad.isDownPressed());
  Serial.print(" L:"); Serial.print(GamePad.isLeftPressed());
  Serial.print(" R:"); Serial.println(GamePad.isRightPressed());
  delay(20);
}

// ════════════════════════════════════════
//  Movement Functions
// ════════════════════════════════════════

void moveForward() {
  // Both sides forward
  ledcWrite(CH_L_FWD, FULL_SPEED); ledcWrite(CH_L_BWD, 0);
  ledcWrite(CH_R_FWD, FULL_SPEED); ledcWrite(CH_R_BWD, 0);
}

void moveBackward() {
  ledcWrite(CH_L_FWD, 0); ledcWrite(CH_L_BWD, FULL_SPEED);
  ledcWrite(CH_R_FWD, 0); ledcWrite(CH_R_BWD, FULL_SPEED);
}

void turnLeft() {
  // Left side backward, right side forward → pivot left
  ledcWrite(CH_L_FWD, 0);          ledcWrite(CH_L_BWD, TURN_SPEED);
  ledcWrite(CH_R_FWD, TURN_SPEED); ledcWrite(CH_R_BWD, 0);
}

void turnRight() {
  // Left side forward, right side backward → pivot right
  ledcWrite(CH_L_FWD, TURN_SPEED); ledcWrite(CH_L_BWD, 0);
  ledcWrite(CH_R_FWD, 0);          ledcWrite(CH_R_BWD, TURN_SPEED);
}

void stopAll() {
  ledcWrite(CH_L_FWD, 0); ledcWrite(CH_L_BWD, 0);
  ledcWrite(CH_R_FWD, 0); ledcWrite(CH_R_BWD, 0);
}

// ─── helpers ───────────────────────────
void setLeft(int pwm) {
  ledcWrite(CH_L_FWD, pwm); ledcWrite(CH_L_BWD, 0);
}
void setRight(int pwm) {
  ledcWrite(CH_R_FWD, pwm); ledcWrite(CH_R_BWD, 0);
}
