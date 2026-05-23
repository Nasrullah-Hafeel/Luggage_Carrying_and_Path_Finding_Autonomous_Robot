/************* IR SENSOR PINS (ESP8266) *************/
#define IR_L D5
#define IR_C D6
#define IR_R D7

/************* L298N MOTOR PINS (ESP8266) *************/
#define IN1 D1
#define IN2 D2
#define IN3 D3
#define IN4 D4
#define ENA D0
#define ENB D8

#define BLACK 1

// ---- SPEEDS (tune only these) ----
int FWD_SLOW   = 70;   // slow forward (after kick)
int ROT_SLOW   = 70;   // slow rotate (after kick)

// ---- KICK (to start motors from stop) ----
int KICK_PWM   = 200;
int KICK_MS    = 100;

// ---- Junction detection (RIGHT cave) ----
const unsigned long JUNC_MS = 80;   // stable time to confirm right junction
unsigned long juncStart = 0;

// ---- Arrival marker (LONG BLACK TAPE = 111) ----
const unsigned long MARK_MS = 220;  // must be 111 for this long
unsigned long markStart = 0;

// ---- Turning mode ----
bool turningRight = false;
bool junctionLocked = false;

// ---- Anti-stuck during turn ----
unsigned long lastKickDuringTurn = 0;
const unsigned long TURN_KICK_EVERY_MS = 250;

// ---- Robot mode ----
enum Mode { GOING_OUT, RETURNING, STOPPED };
Mode mode = GOING_OUT;

// ----------- Helpers -----------
bool allBlack(int L, int C, int R) { return (L == BLACK && C == BLACK && R == BLACK); }
bool allWhite(int L, int C, int R) { return (L != BLACK && C != BLACK && R != BLACK); }

void setup() {
  Serial.begin(115200);

  pinMode(IR_L, INPUT);
  pinMode(IR_C, INPUT);
  pinMode(IR_R, INPUT);

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);

  stopMotors();
  Serial.println("RIGHT CAVE + RETURN TO START (marker=111)");
}

void loop() {
  if (mode == STOPPED) {
    stopMotors();
    while (1) { yield(); }
  }

  // ---------- TURN MODE ----------
  if (turningRight) {
    doRightTurning();
    return;
  }

  int L = digitalRead(IR_L);
  int C = digitalRead(IR_C);
  int R = digitalRead(IR_R);

  // ---------- ARRIVAL / START MARKER (111) ----------
  // GOING_OUT: 111 means "destination end" -> wait 5s -> rotate 180 -> RETURNING
  // RETURNING: 111 means "start point" -> stop forever
  if (allBlack(L, C, R)) {
    if (markStart == 0) markStart = millis();

    if (millis() - markStart >= MARK_MS) {
      stopMotors();

      if (mode == GOING_OUT) {
        Serial.println("DESTINATION MARKER (111) -> WAIT 5s -> TURN 180 -> RETURN");
        delay(5000);

        turn180LeftTimed();     // keep your rotation style
        mode = RETURNING;

        // reset things so returning does NOT re-trigger junction
        junctionLocked = true;  // block junction logic temporarily
        juncStart = 0;
        markStart = 0;

        delay(200);
        return;
      } else {
        Serial.println("START MARKER (111) -> STOP");
        mode = STOPPED;
        return;
      }
    }
  } else {
    markStart = 0;
  }

  // ---------- RETURNING MODE: DO NOT TAKE CAVES ----------
  // This is the MAIN FIX: ignore junction detection on return.
  if (mode == RETURNING) {
    // Once you are stable straight again, unlock (optional)
    if (L != BLACK && C == BLACK && R != BLACK) junctionLocked = false;

    // Just follow line normally (no cave forcing)
    lineFollowNormal(L, C, R);
    return;
  }

  // ---------- GOING_OUT MODE: your same RIGHT CAVE LOGIC ----------
  // Unlock junction when stable straight again
  if (L != BLACK && C == BLACK && R != BLACK) {
    junctionLocked = false;
  }

  // Right junction detection: (C=1 & R=1)
  if (!junctionLocked && (C == BLACK && R == BLACK)) {
    if (juncStart == 0) juncStart = millis();

    if (millis() - juncStart >= JUNC_MS) {
      Serial.println("RIGHT JUNCTION -> FORCE RIGHT");

      stopMotors();
      delay(40);

      forwardKickThenSlow();
      delay(80);
      stopMotors();
      delay(40);

      startRightTurn();
      juncStart = 0;
      return;
    }
  } else {
    juncStart = 0;
  }

  // Normal line follow
  lineFollowNormal(L, C, R);
}

// ================== LINE FOLLOW (same as you already used) ==================
void lineFollowNormal(int L, int C, int R) {
  if (L != BLACK && C == BLACK && R != BLACK) {
    forwardKickThenSlow();
  }
  else if (L == BLACK && C != BLACK && R != BLACK) {
    rotateLeftUntilCenterBlack();
  }
  else if (R == BLACK && C != BLACK && L != BLACK) {
    rotateRightUntilCenterBlack();
  }
  else if (C == BLACK) {
    forwardKickThenSlow();
  }
  else {
    stopMotors();
  }
}

// ================== TURN RIGHT MODE (unchanged style) ==================
void startRightTurn() {
  turningRight = true;
  lastKickDuringTurn = 0;

  rotateRightRaw();
  analogWrite(ENA, KICK_PWM);
  analogWrite(ENB, KICK_PWM);
  delay(KICK_MS);
  lastKickDuringTurn = millis();
}

void doRightTurning() {
  rotateRightRaw();

  if (millis() - lastKickDuringTurn >= TURN_KICK_EVERY_MS) {
    analogWrite(ENA, KICK_PWM);
    analogWrite(ENB, KICK_PWM);
    delay(KICK_MS);
    lastKickDuringTurn = millis();
  }

  // stop only when aligned (010)
  int L2 = digitalRead(IR_L);
  int C2 = digitalRead(IR_C);
  int R2 = digitalRead(IR_R);

  if (L2 != BLACK && C2 == BLACK && R2 != BLACK) {
    stopMotors();
    turningRight = false;
    junctionLocked = true;
    delay(60);
  }

  yield();
}

// ================== MOTOR HELPERS ==================
void forwardKickThenSlow() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);

  analogWrite(ENA, KICK_PWM);
  analogWrite(ENB, KICK_PWM);
  delay(KICK_MS);

  analogWrite(ENA, FWD_SLOW);
  analogWrite(ENB, FWD_SLOW);
}

void rotateLeftUntilCenterBlack() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);

  analogWrite(ENA, KICK_PWM);
  analogWrite(ENB, KICK_PWM);
  delay(KICK_MS);

  while (digitalRead(IR_C) != BLACK) {
    analogWrite(ENA, ROT_SLOW);
    analogWrite(ENB, ROT_SLOW);
    delay(1);
    yield();
  }
}

void rotateRightUntilCenterBlack() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);

  analogWrite(ENA, KICK_PWM);
  analogWrite(ENB, KICK_PWM);
  delay(KICK_MS);

  while (digitalRead(IR_C) != BLACK) {
    analogWrite(ENA, ROT_SLOW);
    analogWrite(ENB, ROT_SLOW);
    delay(1);
    yield();
  }
}

void rotateRightRaw() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  analogWrite(ENA, ROT_SLOW);
  analogWrite(ENB, ROT_SLOW);
}

// ✅ Simple timed 180 (no change to your turning style)
// Tune TURN180_MS once for your robot.
const unsigned long TURN180_MS = 900;

void turn180LeftTimed() {
  // rotate LEFT in place (same style)
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);

  // kick
  analogWrite(ENA, KICK_PWM);
  analogWrite(ENB, KICK_PWM);
  delay(KICK_MS);

  // timed rotate
  analogWrite(ENA, ROT_SLOW);
  analogWrite(ENB, ROT_SLOW);
  delay(TURN180_MS);

  stopMotors();
  delay(120);
}

void stopMotors() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}
