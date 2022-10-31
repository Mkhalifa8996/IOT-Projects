// define pins numbers
const int pump_pin = 8;
void setup() {
  pinMode(pump_pin, OUTPUT);
  digitalWrite(pump_pin, LOW);
  Serial.begin(9600);      // start the serial communication
}

void loop() {
  digitalWrite(pump_pin, HIGH);       // Turn off water pump
  delay(3000);
  digitalWrite(pump_pin, LOW);        // Turn on water pump
  delay(3000);
}
