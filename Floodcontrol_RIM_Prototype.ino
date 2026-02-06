// Pin definitions
int trigPin = 5;
int echoPin = 6;
int motorPin1 = 3;   // Motor driver input 1
int motorPin2 = 4;   // Motor driver input 2
byte speed = 255;    // Motor speed
int threshold = 60;  // Distance in cm to trigger conveyor
bool motorRunning = false;
char command;
String status = "OFF";

// For water detection logic
long prevDistance = 0;
int stableCount = 0;         // how many consecutive times distance stays the same
const int stableLimit = 10;  // number of stable readings before deciding "water"
const int delta = 8;         // minimum change in cm to consider as "sudden change"

void setup() {
  Serial.begin(9600); // For Bluetooth HC-05
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
}

// Measure distance with ultrasonic sensor
long getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 30000); // timeout 30ms
  if (duration == 0) return -1; // no echo
  return duration * 0.034 / 2; // Convert to cm
}

void forward() {
  analogWrite(motorPin1, speed);
  analogWrite(motorPin2, 0);
  motorRunning = true;
}

void stop() {
  analogWrite(motorPin1, 0);
  analogWrite(motorPin2, 0);
  motorRunning = false;
}

void loop() {
  if (Serial.available()) {
    command = Serial.read();
    Serial.println(command);


    if (command == 'B') {
      Serial.println("BEGIN");
      status = "ON";
      while (status == "ON") {
        long distance = getDistance();

        if (distance > 0) { // valid reading
          Serial.print("Distance: ");
          Serial.print(distance);
          Serial.println(" cm");

          // Check sudden change vs previous
          if (abs(distance - prevDistance) > delta && distance < threshold) {
            Serial.println("Trash detected - Motor ON");
            forward();
            delay(5000); // run conveyor 5 sec
            stop();
            stableCount = 0; // reset stability count
          } else {
            // If distance is almost the same as before
            if (abs(distance - prevDistance) <= delta) {
              stableCount++;
              if (stableCount >= stableLimit) {
                Serial.println("Water detected");
                stableCount = 0; // reset after reporting
              }
            }
            Serial.println("Motor OFF");
          }

          prevDistance = distance; // update previous
        }

        // Check for stop command
        if (Serial.available()) {
          command = Serial.read();
          Serial.println(command);
          if (command == 'X') {
            status = "OFF";
            stop();
          }
        }

        delay(500); // sampling delay
      }
    }
  }
}
