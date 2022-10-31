// code for water tank level measurement

// define pins numbers
const int trigPin = 8;
const int echoPin = 9;
const int pump = 10;
// define variables
long duration;
int distance;
void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(pump, OUTPUT);

  Serial.begin(9600);      // start the serial communication
}

void loop() {
  digitalWrite(trigPin, LOW); // Clears the trigPin
  delayMicroseconds(2);

  // send ultrasonic signal
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculate the distance.
  distance = duration * 0.034 / 2;

  // Print the distance on the serial monitor.
  Serial.print("Distance: ");
  Serial.println(distance);
  if (distance <= 5 ) {
    digitalWrite(pump, HIGH);     // Turn off the water pump
    delay(1000);
  }
  else if (distance >= 50 ) {
    digitalWrite(pump, LOW);     // Turn on the water pump
    delay(1000);
  }
}
