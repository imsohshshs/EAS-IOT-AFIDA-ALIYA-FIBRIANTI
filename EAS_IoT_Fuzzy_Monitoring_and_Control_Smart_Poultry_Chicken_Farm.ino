#include <DHT.h>
#include <Fuzzy.h>
#include <FuzzyComposition.h>
#include <FuzzyInput.h>
#include <FuzzyIO.h>
#include <FuzzyOutput.h>
#include <FuzzyRule.h>
#include <FuzzyRuleAntecedent.h>
#include <FuzzyRuleConsequent.h>
#include <FuzzySet.h>
#include <Servo.h>

#define DHTPIN 2
#define DHTTYPE DHT22
#define TRIGGER_PIN 9
#define ECHO_PIN 10
#define LDR_PIN A2
#define SERVO_PIN 6
#define RELAY_LIGHT_PIN 11
#define RELAY_MOTOR_PIN 7

DHT dht(DHTPIN, DHTTYPE);
Servo servo;
Fuzzy *fuzzy = new Fuzzy();

void setup() {
  Serial.begin(9600);

  // Initialize sensors and actuators
  dht.begin();
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(RELAY_LIGHT_PIN, OUTPUT);
  pinMode(RELAY_MOTOR_PIN, OUTPUT);
  servo.attach(SERVO_PIN);

  // Fuzzy Logic Configuration
  // Fuzzy sets for humidity
  FuzzySet *lowHumidity = new FuzzySet(0, 0, 20, 40);
  FuzzySet *mediumHumidity = new FuzzySet(30, 50, 50, 70);
  FuzzySet *highHumidity = new FuzzySet(60, 80, 100, 100);

  // Fuzzy sets for distance
  FuzzySet *shortDistance = new FuzzySet(0, 0, 10, 30);
  FuzzySet *mediumDistance = new FuzzySet(20, 50, 50, 80);
  FuzzySet *longDistance = new FuzzySet(70, 100, 200, 200);

  // Fuzzy sets for LDR
  FuzzySet *dark = new FuzzySet(0, 0, 200, 400);
  FuzzySet *dim = new FuzzySet(300, 500, 500, 700);
  FuzzySet *bright = new FuzzySet(600, 800, 1023, 1023);

  // Fuzzy sets for outputs
  FuzzySet *lightOff = new FuzzySet(0, 0, 0, 0);
  FuzzySet *lightLow = new FuzzySet(50, 100, 100, 150);
  FuzzySet *lightHigh = new FuzzySet(200, 255, 255, 255);

  // Inputs and outputs
  FuzzyInput *humidity = new FuzzyInput(1);
  humidity->addFuzzySet(lowHumidity);
  humidity->addFuzzySet(mediumHumidity);
  humidity->addFuzzySet(highHumidity);
  fuzzy->addFuzzyInput(humidity);

  FuzzyInput *distance = new FuzzyInput(2);
  distance->addFuzzySet(shortDistance);
  distance->addFuzzySet(mediumDistance);
  distance->addFuzzySet(longDistance);
  fuzzy->addFuzzyInput(distance);

  FuzzyInput *ldr = new FuzzyInput(3);
  ldr->addFuzzySet(dark);
  ldr->addFuzzySet(dim);
  ldr->addFuzzySet(bright);
  fuzzy->addFuzzyInput(ldr);

  FuzzyOutput *lightControl = new FuzzyOutput(1);
  lightControl->addFuzzySet(lightOff);
  lightControl->addFuzzySet(lightLow);
  lightControl->addFuzzySet(lightHigh);
  fuzzy->addFuzzyOutput(lightControl);

  // Fuzzy rules
  FuzzyRuleAntecedent *ifHumidityHighAndDistanceShort = new FuzzyRuleAntecedent();
  ifHumidityHighAndDistanceShort->joinWithAND(highHumidity, shortDistance);
  FuzzyRuleConsequent *thenLightHigh = new FuzzyRuleConsequent();
  thenLightHigh->addOutput(lightHigh);
  FuzzyRule *rule1 = new FuzzyRule(1, ifHumidityHighAndDistanceShort, thenLightHigh);
  fuzzy->addFuzzyRule(rule1);

  FuzzyRuleAntecedent *ifLdrDark = new FuzzyRuleAntecedent();
  ifLdrDark->joinSingle(dark);
  FuzzyRuleConsequent *thenLightOn = new FuzzyRuleConsequent();
  thenLightOn->addOutput(lightHigh);
  FuzzyRule *rule2 = new FuzzyRule(2, ifLdrDark, thenLightOn);
  fuzzy->addFuzzyRule(rule2);

  FuzzyRuleAntecedent *ifDistanceLong = new FuzzyRuleAntecedent();
  ifDistanceLong->joinSingle(longDistance);
  FuzzyRuleConsequent *thenLightLow = new FuzzyRuleConsequent();
  thenLightLow->addOutput(lightLow);
  FuzzyRule *rule3 = new FuzzyRule(3, ifDistanceLong, thenLightLow);
  fuzzy->addFuzzyRule(rule3);
}

float getDistance() {
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  return (duration * 0.034 / 2);
}

void loop() {
  // Sensor readings
  float humidity = dht.readHumidity();
  float distance = getDistance();
  int ldrValue = analogRead(LDR_PIN);

  if (isnan(humidity) || isnan(distance)) {
    Serial.println("Failed to read sensors");
    return;
  }

  // Apply fuzzy logic
  fuzzy->setInput(1, humidity);
  fuzzy->setInput(2, distance);
  fuzzy->setInput(3, ldrValue);
  fuzzy->fuzzify();

  int lightIntensity = fuzzy->defuzzify(1);

  // Control actuators
  analogWrite(RELAY_LIGHT_PIN, lightIntensity);

  if (ldrValue < 300) {
    digitalWrite(RELAY_MOTOR_PIN, HIGH); // Close curtain
  } else {
    digitalWrite(RELAY_MOTOR_PIN, LOW); // Open curtain
  }

  if (distance < 30) {
    servo.write(90); // Open valve
  } else {
    servo.write(0);  // Close valve
  }

  // Debugging
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" Distance: ");
  Serial.print(distance);
  Serial.print(" LDR: ");
  Serial.print(ldrValue);
  Serial.print(" Light Intensity: ");
  Serial.println(lightIntensity);

  delay(500);
}
