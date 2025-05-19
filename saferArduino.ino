#include <Servo.h>
#include <DHT.h>
#include <ArduinoJson.h>

// Pin Analogici
#define PIN_FUMO A2
#define PIN_LUCE A0

// Motore sinistro
#define PIN_PWM_S 2
#define PIN_S1 3
#define PIN_S2 4

// Motore destro
#define PIN_D1 5
#define PIN_D2 6
#define PIN_PWM_D 7
#define PIN_LUCE_EMERGENZA 26

// Sensori ultrasuoni
#define PIN_TRIG_FRONTE 10
#define PIN_ECHO_FRONTE 11
#define PIN_TRIG_RETRO 12
#define PIN_ECHO_RETRO 13
#define PIN_TRIG_SINISTRA 14
#define PIN_ECHO_SINISTRA 15
#define PIN_TRIG_DESTRA 16
#define PIN_ECHO_DESTRA 17

// Costanti globali
const int DISTANZA_MINIMA = 50;   // Distanza minima da un ostacolo in cm
const int TEMPO_SVOLTA_90 = 2600; // Millisecondi per ruotare
const bool DEBUG = false;          // Attiva i Print di debug

// Oggetti globali
Servo servoPan;  // Servo per movimento orizzontale
Servo servoTilt; // Servo per movimento verticale
JsonDocument reportSensori;
DHT sensoreTempUmidita(25, DHT11);

// Variabili globali
uint8_t comandoAttuale = 3; // Comando corrente (default: stop)

void setup()
{
  //Serial.begin(9600);
  Serial1.begin(9600);
  sensoreTempUmidita.begin();

  for (int i = 2; i < 8; i++)
  {
    pinMode(i, OUTPUT);
  }

  for (int i = 10; i < 17; i += 2)
  {
    pinMode(i, OUTPUT);
  }

  for (int i = 11; i < 18; i += 2)
  {
    pinMode(i, INPUT);
  }

  for (int i = 18; i < 25; i++)
  {
    pinMode(i, INPUT);
  }

  pinMode(PIN_LUCE_EMERGENZA, OUTPUT);

  servoPan.attach(8);
  servoTilt.attach(9);
  resetPanTilt();

  analogWrite(PIN_PWM_S, 90);
  analogWrite(PIN_PWM_D, 94);
}

void loop()
{
  if (Serial1.available()) {
    comandoAttuale = Serial1.read();
  }
  switch (comandoAttuale)
  {
  case 0:
    fermaMotori();
    muoviPanTilt();
    inviaReportSensori();
    comandoAttuale = 3;
    break;

  case 1:
    resetPanTilt();
    navigaAutomatica();
    break;

  case 2:
    fermaMotori();
    lampeggiaLuceEmergenza();
    break;

  case 3:
    fermaMotori();
    break;
  }
  delay(200);
}

void lampeggiaLuceEmergenza()
{
  digitalWrite(PIN_LUCE_EMERGENZA, HIGH);
  delay(200);
  digitalWrite(PIN_LUCE_EMERGENZA, LOW);
  delay(200);
}

void navigaAutomatica()
{
  const float TOLLERANZA = 2.5;

  static float distanzaPrecFrontale = 0;
  static float distanzaPrecSinistra = 0;
  static float distanzaPrecDestra = 0;
  static float distanzaPrecPosteriore = 0;
  static int contatoreBlocco = 0;

  float distanzaFrontale = misuraDistanza(PIN_TRIG_FRONTE, PIN_ECHO_FRONTE);
  float distanzaSinistra = misuraDistanza(PIN_TRIG_SINISTRA, PIN_ECHO_SINISTRA);
  float distanzaDestra = misuraDistanza(PIN_TRIG_DESTRA, PIN_ECHO_DESTRA);
  float distanzaPosteriore = misuraDistanza(PIN_TRIG_RETRO, PIN_ECHO_RETRO);

  if (DEBUG)
  {
    Serial1.print("F: "); Serial1.print(distanzaFrontale);
    Serial1.print(" S: "); Serial1.print(distanzaSinistra);
    Serial1.print(" D: "); Serial1.print(distanzaDestra);
    Serial1.print(" R: "); Serial1.println(distanzaPosteriore);
  }

  bool fuoriPortata = distanzaFrontale >= 240 && distanzaSinistra >= 240 && distanzaDestra >= 240 && distanzaPosteriore >= 240;
  bool bloccato = !fuoriPortata &&
                  abs(distanzaFrontale - distanzaPrecFrontale) < TOLLERANZA &&
                  abs(distanzaSinistra - distanzaPrecSinistra) < TOLLERANZA &&
                  abs(distanzaDestra - distanzaPrecDestra) < TOLLERANZA &&
                  abs(distanzaPosteriore - distanzaPrecPosteriore) < TOLLERANZA;

  contatoreBlocco = bloccato ? contatoreBlocco + 1 : 0;

  if (contatoreBlocco >= 3)
  {
    if (DEBUG) Serial1.println("Ostacolo persistente, manovra evasiva");
    indietro();
    delay(700);
    if (distanzaSinistra > distanzaDestra)
      svoltaSinistra();
    else
      svoltaDestra();
    contatoreBlocco = 0;
  }
  else if (distanzaFrontale >= DISTANZA_MINIMA)
  {
    avanti();
  }
  else
  {
    if (DEBUG) Serial1.println("Ostacolo frontale, decisione svolta");
    fermaMotori();
    delay(200);
    if (distanzaSinistra >= DISTANZA_MINIMA && distanzaSinistra >= distanzaDestra)
      svoltaSinistra();
    else if (distanzaDestra >= DISTANZA_MINIMA)
      svoltaDestra();
    else if (distanzaPosteriore >= DISTANZA_MINIMA)
    {
      indietro();
      delay(600);
    }
  }

  distanzaPrecFrontale = distanzaFrontale;
  distanzaPrecSinistra = distanzaSinistra;
  distanzaPrecDestra = distanzaDestra;
  distanzaPrecPosteriore = distanzaPosteriore;
}

void fermaMotori()
{
  if (DEBUG) Serial1.println("Motori fermati");
  digitalWrite(PIN_S1, LOW);
  digitalWrite(PIN_S2, LOW);
  digitalWrite(PIN_D1, LOW);
  digitalWrite(PIN_D2, LOW);
  delay(500);
}

void avanti()
{
  if (DEBUG) Serial1.println("Movimento in avanti");
  digitalWrite(PIN_S1, LOW);
  digitalWrite(PIN_S2, HIGH);
  digitalWrite(PIN_D1, LOW);
  digitalWrite(PIN_D2, HIGH);
}

void indietro()
{
  if (DEBUG) Serial1.println("Movimento indietro");
  digitalWrite(PIN_S1, HIGH);
  digitalWrite(PIN_S2, LOW);
  digitalWrite(PIN_D1, HIGH);
  digitalWrite(PIN_D2, LOW);
}

void svoltaSinistra()
{
  if (DEBUG) Serial1.println("Svolta a sinistra");
  digitalWrite(PIN_S1, HIGH);
  digitalWrite(PIN_S2, LOW);
  digitalWrite(PIN_D1, LOW);
  digitalWrite(PIN_D2, HIGH);
  delay(TEMPO_SVOLTA_90);
}

void svoltaDestra()
{
  if (DEBUG) Serial1.println("Svolta a destra");
  digitalWrite(PIN_S1, LOW);
  digitalWrite(PIN_S2, HIGH);
  digitalWrite(PIN_D1, HIGH);
  digitalWrite(PIN_D2, LOW);
  delay(TEMPO_SVOLTA_90);
}

void resetPanTilt()
{
  servoPan.write(90);
  servoTilt.write(90);
  if (DEBUG)
  {
    Serial1.println("Pan/Tilt riposizionati a 90°");
  }
}

void muoviPanTilt()
{
  uint8_t angoloPan = random(0, 180);
  uint8_t angoloTilt = random(45, 85);
  if (DEBUG)
  {
    Serial1.print("\nPan: "); Serial1.println(angoloPan);
    Serial1.print("Tilt: "); Serial1.println(angoloTilt);
  }
  servoPan.write(angoloPan);
  servoTilt.write(angoloTilt);
}

void inviaReportSensori()
{
  float fumo = analogRead(PIN_FUMO) * 100.0 / 1023.0;
  float luce = analogRead(PIN_LUCE) * 0.55 - 95.14;
  float temperatura = sensoreTempUmidita.readTemperature();
  float umidita = sensoreTempUmidita.readHumidity();

  reportSensori["fumo"] = round(fumo * 10) / 10.0;
  reportSensori["luce"] = luce;
  reportSensori["temperatura"] = temperatura;
  reportSensori["umidita"] = umidita;

  serializeJson(reportSensori, Serial1);
  Serial1.println();
}

double misuraDistanza(int pinTrigger, int pinEcho)
{
  const double DISTANZA_MASSIMA = 300.0;
  const unsigned long TIMEOUT_US = DISTANZA_MASSIMA * 58.0;

  digitalWrite(pinTrigger, LOW);
  delayMicroseconds(2);
  digitalWrite(pinTrigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(pinTrigger, LOW);

  long durata = pulseIn(pinEcho, HIGH, TIMEOUT_US);
  double distanza = durata * 0.0343 / 2;

  if (distanza <= 0 || distanza > DISTANZA_MASSIMA)
  {
    return DISTANZA_MASSIMA;
  }

  return distanza;
}