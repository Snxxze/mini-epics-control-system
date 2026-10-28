#include <DHT.h>

const int DHT_PIN = 4;
const int DHT_TYPE = DHT22;

DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();

  delay(2000);
  Serial.println("ESP32 DHT22 ready");
}

void loop() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("ERROR=DHT_READ_FAILED");
    delay(2000);
    return;
  }

  Serial.print("TEMP=");
  Serial.print(temperature, 1);
  Serial.print(" ,HUM=");
  Serial.println(humidity, 1);

  delay(2000);
}
