#define IR_L D5
#define IR_C D6
#define IR_R D7

#define IN1 D1
#define IN2 D2
#define IN3 D3
#define IN4 D4
#define ENA D0
#define ENB D8

#define BLACK 1

int FWD_SLOW   = 70;
int ROT_SLOW   = 70;

int KICK_PWM   = 200;
int KICK_MS    = 100;

unsigned long junctionStart = 0;
const unsigned long JUNCTION_CONFIRM_MS = 120;

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

void loop() {
  int L = digitalRead(IR_L);
  int C = digitalRead(IR_C);
  int R = digitalRead(IR_R);



  bool isJunction = (L == BLACK && C == BLACK); 
  if (isJunction) {
    if (junctionStart == 0) junctionStart = millis();

    if (millis() - junctionStart >= JUNCTION_CONFIRM_MS) {

      forwardKickThenSlow();
      delay(120);

      rotateLeftUntilCenterBlack();

      junctionStart = 0;
      return;
    }
  } else {
    junctionStart = 0;
  }



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

void stopMotors() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);

  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}
