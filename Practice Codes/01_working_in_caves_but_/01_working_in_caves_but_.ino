/************* IR SENSOR PINS (ESP8266) *************/
#define IR_L D5   // GPIO14
#define IR_C D6   // GPIO12
#define IR_R D7   // GPIO13

/************* L298N MOTOR PINS (ESP8266) *************/
#define IN1 D1
#define IN2 D2
#define IN3 D3
#define IN4 D4
#define ENA D0
#define ENB D8

/************* SENSOR LOGIC *************/
#define BLACK 1

/************* VERY SLOW SPEED SETTINGS *************/
int FWD_SPEED   = 60;   // very slow forward
int ROT_SPEED   = 55;   // very slow rotate
int KICK_SPEED  = 170;  // start torque
int KICK_MS     = 80;   // kick time

void setup() {
  Serial.begin(115200);

  pinMode(IR_L, INPUT);
  pinMode(IR_C, INPUT);
  pinMode(IR_R, INPUT);

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);

  stopMotors();
}

/************* MAIN LOOP *************/
void loop() {
  int L = digitalRead(IR_L);
  int C = digitalRead(IR_C);
  int R = digitalRead(IR_R);

  // (optional) Serial monitor
  // Serial.printf("L:%d C:%d R:%d\n", L, C, R);

  // 1) ONLY CENTER BLACK -> FORWARD
  if (L != BLACK && C == BLACK && R != BLACK) {
    forwardVerySlow();
  }

  // 2) RIGHT BLACK -> robot angled LEFT -> rotate RIGHT slowly until CENTER only
  else if (R == BLACK && C != BLACK) {
    rotateRightUntilCenter();
  }

  // 3) LEFT BLACK -> robot angled RIGHT -> rotate LEFT slowly until CENTER only
  else if (L == BLACK && C != BLACK) {
    rotateLeftUntilCenter();
  }

  // 4) If center black but side also black -> go forward slowly (still on line)
  else if (C == BLACK) {
    forwardVerySlow();
  }

  // 5) ALL WHITE -> STOP
  else {
    stopMotors();
  }

  // no delay -> checks sensors again immediately
}

/************* MOTOR FUNCTIONS *************/
void forwardVerySlow() {
  // Kick so motors start
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);

  analogWrite(ENA, KICK_SPEED);
  analogWrite(ENB, KICK_SPEED);
  delay(KICK_MS);

  analogWrite(ENA, FWD_SPEED);
  analogWrite(ENB, FWD_SPEED);
}

void rotateRightUntilCenter() {
  // Rotate RIGHT slowly: LEFT forward, RIGHT backward
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);

  analogWrite(ENA, ROT_SPEED);
  analogWrite(ENB, ROT_SPEED);

  // keep checking sensors until only center is black
  while (true) {
    int L = digitalRead(IR_L);
    int C = digitalRead(IR_C);
    int R = digitalRead(IR_R);

    if (L != BLACK && C == BLACK && R != BLACK) {
      break;
    }
    // tiny delay to avoid watchdog reset
    delay(1);
  }
}

void rotateLeftUntilCenter() {
  // Rotate LEFT slowly: LEFT backward, RIGHT forward
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);

  analogWrite(ENA, ROT_SPEED);
  analogWrite(ENB, ROT_SPEED);

  while (true) {
    int L = digitalRead(IR_L);
    int C = digitalRead(IR_C);
    int R = digitalRead(IR_R);

    if (L != BLACK && C == BLACK && R != BLACK) {
      break;
    }
    delay(1);
  }
}

void stopMotors() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}
