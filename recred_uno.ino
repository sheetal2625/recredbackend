/*
 * =========================================================
 *  ReCred v2 — Arduino UNO
 * =========================================================
 *  IR Sensor  → D2  (LOW = object present)
 *  Force/Pressure Sensor → A0  (threshold 650 = overweight/filled)
 *  Servo      → D9
 *  UART to Pi → TX/RX (USB, 115200 baud)
 *
 *  Protocol (UNO → Pi):
 *    UNO_READY          startup handshake
 *    TRIG OV=0          IR triggered, weight normal (< 650)
 *    TRIG OV=1          IR triggered, weight over (≥ 650, filled bottle)
 *    DONE               servo returned to centre
 *    DBG FSR=xxx TH=yyy OV=z   debug line
 *
 *  Protocol (Pi → UNO):
 *    SORT R             rotate servo RIGHT (reject)
 *    SORT L             rotate servo LEFT  (accept)
 *    CENTER             return to centre immediately
 *    SET_TH xxx         change threshold at runtime
 * =========================================================
 */

#include <Servo.h>

// ===================== PINS =====================
const int IR_PIN    = 2;
const int FSR_PIN   = A0;
const int SERVO_PIN = 9;

// ===================== TUNABLES =====================
int       FSR_THRESHOLD      = 650;    // ≥650 = overweight / filled bottle
const int SERVO_LEFT         = 0;      // accept  (empty bottle)
const int SERVO_RIGHT        = 180;    // reject  (filled / not bottle / timeout)
const int SERVO_CENTER       = 90;
const unsigned long SERVO_HOLD_MS   = 1200;  // hold before returning centre
const unsigned long DEBOUNCE_MS     = 200;
const unsigned long IR_STABLE_MS    = 80;    // IR must be clear this long
const unsigned long SORT_TIMEOUT_MS = 8000;  // Pi safety timeout

// ===================== STATE =====================
Servo sorter;
enum State { IDLE, WAIT_SORT_CMD, ACTUATING, WAIT_CLEAR };
State state = IDLE;

unsigned long tLastTrigger  = 0;
unsigned long tServoUntil   = 0;
unsigned long tSortCmdStart = 0;
unsigned long tIRClearStart = 0;

// ===================== HELPERS =====================
bool irTriggered() {
  return (digitalRead(IR_PIN) == LOW);  // LOW = object blocking sensor
}

int readFSR() {
  return analogRead(FSR_PIN);
}

String readLine() {
  static String buf = "";
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n') {
      String out = buf; buf = ""; out.trim(); return out;
    } else if (c != '\r') {
      buf += c;
      if (buf.length() > 80) buf = "";
    }
  }
  return "";
}

// ===================== SETUP =====================
void setup() {
  pinMode(IR_PIN, INPUT_PULLUP);
  sorter.attach(SERVO_PIN);
  sorter.write(SERVO_CENTER);
  Serial.begin(115200);
  delay(400);
  Serial.println("UNO_READY");
}

// ===================== LOOP =====================
void loop() {
  unsigned long now = millis();

  // ---- Read commands from Pi ----
  String cmd = readLine();
  if (cmd.length() > 0) {

    if (cmd == "SORT R") {
      sorter.write(SERVO_RIGHT);
      tServoUntil = now + SERVO_HOLD_MS;
      state = ACTUATING;

    } else if (cmd == "SORT L") {
      sorter.write(SERVO_LEFT);
      tServoUntil = now + SERVO_HOLD_MS;
      state = ACTUATING;

    } else if (cmd == "CENTER") {
      sorter.write(SERVO_CENTER);

    } else if (cmd.startsWith("SET_TH")) {
      int sp = cmd.indexOf(' ');
      if (sp > 0) {
        int v = cmd.substring(sp + 1).toInt();
        if (v >= 0 && v <= 1023) {
          FSR_THRESHOLD = v;
          Serial.print("TH_SET "); Serial.println(FSR_THRESHOLD);
        }
      }
    }
  }

  // ---- State machine ----
  switch (state) {

    case IDLE: {
      if (irTriggered() && (now - tLastTrigger) > DEBOUNCE_MS) {
        delay(100);  // let object settle on sensor
        int fsr = readFSR();
        bool overweight = (fsr >= FSR_THRESHOLD);

        Serial.print("DBG FSR="); Serial.print(fsr);
        Serial.print(" TH="); Serial.print(FSR_THRESHOLD);
        Serial.print(" OV="); Serial.println(overweight ? 1 : 0);

        Serial.print("TRIG OV=");
        Serial.println(overweight ? 1 : 0);

        tLastTrigger  = now;
        tSortCmdStart = now;
        state = WAIT_SORT_CMD;
      }
      break;
    }

    case WAIT_SORT_CMD: {
      if ((now - tSortCmdStart) > SORT_TIMEOUT_MS) {
        Serial.println("DBG SORT_TIMEOUT");
        state = IDLE;
      }
      break;
    }

    case ACTUATING: {
      if ((long)(now - tServoUntil) >= 0) {
        sorter.write(SERVO_CENTER);
        Serial.println("DONE");
        tIRClearStart = 0;
        state = WAIT_CLEAR;
      }
      break;
    }

    case WAIT_CLEAR: {
      if (!irTriggered()) {
        if (tIRClearStart == 0) tIRClearStart = now;
        if ((now - tIRClearStart) >= IR_STABLE_MS) state = IDLE;
      } else {
        tIRClearStart = 0;
      }
      break;
    }

    default: state = IDLE; break;
  }
}
