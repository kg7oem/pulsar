
#define SERIAL_SPEED 9600
#define VOLUME_POT_PIN 5
#define BUFFER_LEN 100

static char buffer[BUFFER_LEN + 1];

void setup() {
  buffer[BUFFER_LEN] = 0;
  Serial.begin(SERIAL_SPEED);
}

void loop() {
  int val = analogRead(VOLUME_POT_PIN);    // read the value from the sensor

  snprintf(buffer, BUFFER_LEN, "vol %0.4d\n", val);
  Serial.print(buffer);
}
