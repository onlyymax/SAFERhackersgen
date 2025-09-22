#include <Servo.h>
#include <DHT.h>

// ======================= PIN DEFINITIONS ====================

// Analog sensors
#define PIN_SMOKE_SENSOR A2
#define PIN_LIGHT_SENSOR A0

// Left motor
#define PIN_PWM_LEFT 2
#define PIN_LEFT_IN1 3
#define PIN_LEFT_IN2 4

// Right motor
#define PIN_RIGHT_IN1 5
#define PIN_RIGHT_IN2 6
#define PIN_PWM_RIGHT 7

// Emergency light
#define PIN_EMERGENCY_LIGHT 26

// Ultrasonic sensors
#define PIN_TRIG_FRONT 10
#define PIN_ECHO_FRONT 11
#define PIN_TRIG_BACK 12
#define PIN_ECHO_BACK 13
#define PIN_TRIG_LEFT 14
#define PIN_ECHO_LEFT 15
#define PIN_TRIG_RIGHT 16
#define PIN_ECHO_RIGHT 17

// ===================== GLOBAL OBJECTS =======================
Servo servoPan;
Servo servoTilt;
HardwareSerial &SerialCom = Serial;
DHT tempHumiditySensor(25, DHT11);

// =================== GLOBAL VARIABLES =======================
const bool DEBUG = false; // Enable debug logs

int leftMotorSpeed = 90;  // 0–255
int rightMotorSpeed = 94; // 0–255
int cameraPanAngle = 0;
int cameraTiltAngle = 0;

enum RobotState
{
  IDLE,
  WARNING,
  PHOTO,
  AUTO,
  MANUAL
};

RobotState robotState;

// ================== DEBUG UTILITY FUNCTION ==================
void debugPrint(String msg)
{
  if (DEBUG)
  {
    SerialCom.print("DEBUG: ");
    SerialCom.println(msg);
  }
}

// ===================== DATA STRUCTURES ======================
struct EnvironmentalData
{
  float smoke;       // %
  float light;       // Lux (approx.)
  float temperature; // °C
  float humidity;    // %
};

// ==================== MOVEMENT FUNCTIONS ====================
void stopMotors()
{
  debugPrint("Motors stopped");
  digitalWrite(PIN_LEFT_IN1, LOW);
  digitalWrite(PIN_LEFT_IN2, LOW);
  digitalWrite(PIN_RIGHT_IN1, LOW);
  digitalWrite(PIN_RIGHT_IN2, LOW);
}

void moveForward()
{
  debugPrint("Moving forward");
  digitalWrite(PIN_LEFT_IN1, LOW);
  digitalWrite(PIN_LEFT_IN2, HIGH);
  digitalWrite(PIN_RIGHT_IN1, LOW);
  digitalWrite(PIN_RIGHT_IN2, HIGH);
  analogWrite(PIN_PWM_LEFT, leftMotorSpeed);
  analogWrite(PIN_PWM_RIGHT, rightMotorSpeed);
}

void moveBackward()
{
  debugPrint("Moving backward");
  digitalWrite(PIN_LEFT_IN1, HIGH);
  digitalWrite(PIN_LEFT_IN2, LOW);
  digitalWrite(PIN_RIGHT_IN1, HIGH);
  digitalWrite(PIN_RIGHT_IN2, LOW);
  analogWrite(PIN_PWM_LEFT, leftMotorSpeed);
  analogWrite(PIN_PWM_RIGHT, rightMotorSpeed);
}

void turnLeft(int turnDuration)
{
  debugPrint("Turning left");
  digitalWrite(PIN_LEFT_IN1, HIGH);
  digitalWrite(PIN_LEFT_IN2, LOW);
  digitalWrite(PIN_RIGHT_IN1, LOW);
  digitalWrite(PIN_RIGHT_IN2, HIGH);
  analogWrite(PIN_PWM_LEFT, leftMotorSpeed);
  analogWrite(PIN_PWM_RIGHT, rightMotorSpeed);
  delay(turnDuration);
}

void turnRight(int turnDuration)
{
  debugPrint("Turning right");
  digitalWrite(PIN_LEFT_IN1, LOW);
  digitalWrite(PIN_LEFT_IN2, HIGH);
  digitalWrite(PIN_RIGHT_IN1, HIGH);
  digitalWrite(PIN_RIGHT_IN2, LOW);
  analogWrite(PIN_PWM_LEFT, leftMotorSpeed);
  analogWrite(PIN_PWM_RIGHT, rightMotorSpeed);
  delay(turnDuration);
}

void setCameraAngles(uint8_t panAngle, uint8_t tiltAngle)
{
  debugPrint("Setting camera angles");
  int panCurrent = servoPan.read(); 
  int tiltCurrent = servoTilt.read();

  int maxDelta = max(abs(panAngle - panCurrent), abs(tiltAngle - tiltCurrent));
  int delayTime = map(maxDelta, 0, 180, 0, 2000);

  servoPan.write(panAngle);
  servoTilt.write(tiltAngle);

  delay(delayTime); 
}


// ==================== SENSOR FUNCTIONS ======================
EnvironmentalData readEnvironmentalSensors()
{
  EnvironmentalData data;
  data.smoke = round((analogRead(PIN_SMOKE_SENSOR) * 100.0 / 1023.0) * 10) / 10.0;
  data.light = analogRead(PIN_LIGHT_SENSOR) * 0.55 - 95.14;
  data.temperature = tempHumiditySensor.readTemperature();
  data.humidity = tempHumiditySensor.readHumidity();
  return data;
}

void sendSensorReport()
{
  EnvironmentalData data = readEnvironmentalSensors();
  SerialCom.print("{\"smoke\":");
  SerialCom.print(data.smoke, 1);
  SerialCom.print(",\"light\":");
  SerialCom.print(data.light, 2);
  SerialCom.print(",\"temperature\":");
  SerialCom.print(data.temperature, 2);
  SerialCom.print(",\"humidity\":");
  SerialCom.print(data.humidity, 2);
  SerialCom.println("}");
}

double measureDistance(int trigPin, int echoPin)
{
  const double MAX_DISTANCE = 300.0;
  const unsigned long TIMEOUT_US = MAX_DISTANCE * 58.0;
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, TIMEOUT_US);
  double distance = duration * 0.0343 / 2;
  return (distance <= 0 || distance > MAX_DISTANCE) ? MAX_DISTANCE : distance;
}

void emergencyBlink()
{
  for (int i = 0; i < 12; i++)
  {
    digitalWrite(PIN_EMERGENCY_LIGHT, HIGH);
    delay(200);
    digitalWrite(PIN_EMERGENCY_LIGHT, LOW);
    delay(400);
  }
}

// ================== NAVIGATION FUNCTION =====================
void autoNavigate()
{
  const float TOLERANCE = 2.5;
  const int MIN_DISTANCE = 50;
  const int TURN_DURATION = 2600;

  static float prevFront = 0, prevLeft = 0, prevRight = 0, prevBack = 0;
  static int stuckCounter = 0;

  float front = measureDistance(PIN_TRIG_FRONT, PIN_ECHO_FRONT);
  float left = measureDistance(PIN_TRIG_LEFT, PIN_ECHO_LEFT);
  float right = measureDistance(PIN_TRIG_RIGHT, PIN_ECHO_RIGHT);
  float back = measureDistance(PIN_TRIG_BACK, PIN_ECHO_BACK);

  debugPrint("Distances: F=" + String(front) + " L=" + String(left) +
             " R=" + String(right) + " B=" + String(back));

  bool outOfRange = front >= 240 && left >= 240 && right >= 240 && back >= 240;
  bool stuck = !outOfRange &&
               abs(front - prevFront) < TOLERANCE &&
               abs(left - prevLeft) < TOLERANCE &&
               abs(right - prevRight) < TOLERANCE &&
               abs(back - prevBack) < TOLERANCE;

  stuckCounter = stuck ? stuckCounter + 1 : 0;

  if (stuckCounter >= 3)
  {
    debugPrint("Persistent obstacle → evasive maneuver");
    moveBackward();
    delay(700);
    (left > right) ? turnLeft(TURN_DURATION) : turnRight(TURN_DURATION);
    stuckCounter = 0;
  }
  else if (front >= MIN_DISTANCE)
  {
    moveForward();
  }
  else
  {
    debugPrint("Front obstacle → deciding turn");
    stopMotors();
    delay(200);
    if (left >= MIN_DISTANCE && left >= right)
      turnLeft(TURN_DURATION);
    else if (right >= MIN_DISTANCE)
      turnRight(TURN_DURATION);
    else if (back >= MIN_DISTANCE)
    {
      moveBackward();
      delay(600);
    }
  }

  prevFront = front;
  prevLeft = left;
  prevRight = right;
  prevBack = back;
}

// ==================== SELF TEST =======================
void selfTest()
{
  debugPrint("\n=== SELF TEST START ===");

  debugPrint("Testing motors...");
  moveForward();
  delay(500);
  moveBackward();
  delay(500);
  stopMotors();
  debugPrint("Motors OK");

  debugPrint("Testing servos...");
  setCameraAngles(0, 0);
  delay(500);
  setCameraAngles(180, 90);
  delay(500);
  setCameraAngles(90, 45);
  delay(500);
  setCameraAngles(90, 90);
  debugPrint("Servos OK");

  debugPrint("Testing environmental sensors...");
  EnvironmentalData env = readEnvironmentalSensors();
  debugPrint("Smoke: ");
  debugPrint(String(env.smoke, 2));
  debugPrint("Light: ");
  debugPrint(String(env.light, 2));
  debugPrint("Temperature: ");
  debugPrint(String(env.temperature, 2));
  debugPrint("Humidity: ");
  debugPrint(String(env.humidity, 2));
  debugPrint("Environmental sensors OK");

  debugPrint("Testing ultrasonic sensors...");
  debugPrint("Front: ");
  debugPrint(String(measureDistance(PIN_TRIG_FRONT, PIN_ECHO_FRONT), 2));
  debugPrint("Back: ");
  debugPrint(String(measureDistance(PIN_TRIG_BACK, PIN_ECHO_BACK), 2));
  debugPrint("Left: ");
  debugPrint(String(measureDistance(PIN_TRIG_LEFT, PIN_ECHO_LEFT), 2));
  debugPrint("Right: ");
  debugPrint(String(measureDistance(PIN_TRIG_RIGHT, PIN_ECHO_RIGHT), 2));
  debugPrint("Ultrasonic sensors OK");

  debugPrint("Testing emergency light...");
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(PIN_EMERGENCY_LIGHT, HIGH);
    delay(200);
    digitalWrite(PIN_EMERGENCY_LIGHT, LOW);
    delay(200);
  }
  debugPrint("Emergency light OK");

  debugPrint("=== SELF TEST COMPLETE ===\n");
}

// ==================== COMMAND HANDLER =======================
void handleCommand(String cmd)
{
  if (cmd == "START")
  {
    robotState = AUTO;
    setCameraAngles(cameraPanAngle, cameraTiltAngle);
  }
  else if (cmd == "STOP")
  {
    robotState = IDLE;
    stopMotors();
  }
  else if (cmd == "WARNING")
  {
    robotState = IDLE;
    stopMotors();
    emergencyBlink();
  }
  else if (cmd == "PHOTO")
  {
    robotState = IDLE;
    stopMotors();
    setCameraAngles(random(0, 180), random(45, 85));
    sendSensorReport();
  }
  else
  {
    debugPrint("Comando non riconosciuto: " + cmd);
  }
}

// ========================= SETUP ============================
void setup()
{
  SerialCom.begin(115200);
  tempHumiditySensor.begin();

  for (int i = 2; i < 8; i++)
    pinMode(i, OUTPUT); // Motor pins
  for (int i = 10; i < 17; i += 2)
    pinMode(i, OUTPUT); // Ultrasonic trig
  for (int i = 11; i < 18; i += 2)
    pinMode(i, INPUT); // Ultrasonic echo
  for (int i = 18; i < 25; i++)
    pinMode(i, INPUT); // Analog pins
  pinMode(PIN_EMERGENCY_LIGHT, OUTPUT);

  servoPan.attach(8);
  servoTilt.attach(9);

  selfTest();

  setCameraAngles(cameraPanAngle, cameraTiltAngle);
  analogWrite(PIN_PWM_LEFT, leftMotorSpeed);
  analogWrite(PIN_PWM_RIGHT, rightMotorSpeed);

  robotState = IDLE;
}

// ========================== LOOP ============================
void loop()
{
  if (SerialCom.available() > 0)
  {
    String cmd = SerialCom.readStringUntil('\n');
    cmd.trim();
    handleCommand(cmd);
    debugPrint("Comando: " + cmd);
  }

  if (robotState == AUTO)
    autoNavigate();
}
