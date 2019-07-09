#include <Arduino.h>
#include <OneWire.h>
#include <SDS011.h>
#include <DallasTemperature.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <InfluxDb.h>

// Network SSID
const char *ssid = "dune.iot";
const char *password = "4YFtCDoaHguiQtkmyypWjKcFt44gBnV)>>opojbj";

// Corlysis Setting - click to the database to get those info
#define INFLUXDB_HOST "nas.dune.uk"
#define INFLUXDB_PORT 8086
#define INFLUXDB_DATABASE "weather"
#define INFLUXDB_USER "root"
#define INFLUXDB_PASS "root"

HTTPClient http;
WiFiServer server(80);

// Data wire is plugged into pin D1 on the ESP8266 12-E - GPIO 5
#define ONE_WIRE_BUS 4

#define TX_PIN 12
#define RX_PIN 13

SDS011 mySDS;

// Setup a oneWire instance to communicate with any OneWire devices (not just
// Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature DS18B20(&oneWire);
char temperatureCString[7];
char temperatureFString[7];

Influxdb influx(INFLUXDB_HOST, INFLUXDB_PORT);

void connect_to_influxdb() {
  influx.setDbAuth(INFLUXDB_DATABASE, INFLUXDB_USER, INFLUXDB_PASS);
}

void setup() {
   Serial.begin(9600);
  delay(10);

  // Connect WiFi
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.hostname("Name");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Print the IP address
  Serial.print("IP address: ");
  Serial.print(WiFi.localIP());
  // Connect WiFi
  WiFi.hostname("WeatherStation");
  WiFi.begin(ssid, password);

  DS18B20.begin(); // IC Default 9 bit. If you have troubles consider upping
                   // it 12. Ups the delay giving the IC more time to process
                   // the temperature measurement

  mySDS.begin(TX_PIN, RX_PIN);
  connect_to_influxdb();
}

void send_packet_to_influxdb(uint8_t temp, float pm25, float pm10) {
  String series = "air,loc=1 " + String("temp=") + String(temp) +
                  ",pm25=" + String(pm25) + ",pm10=" + String(pm10);
  // write it into db
  Serial.println(series);
  if (!influx.write(series)) {
    Serial.println("Error sending");
  }
}

float hum, temp = 0;
float pm10, pm25 = 0;

float getTemperature() {
  float tempC;
  float tempF;
  do {
    DS18B20.requestTemperatures();
    tempC = DS18B20.getTempCByIndex(0);
    tempF = DS18B20.getTempFByIndex(0);
    delay(100);
  } while (tempC == 85.0 || tempC == (-127.0));
  return tempC;
}

void loop() {
  // put your main code here, to run repeatedly:
  // delay(2000);
  // getTemperature();

  // // Serial.print(" %, Temp: ");
  // // Serial.print(temperatureCString);
  // // Serial.println(" Celsius");
  // int error = 0;
  // int error = mySDS.read(&pm25, &pm10);
  // if (error != 0) {
  //   // Serial.print("Error: ");
  //   // Serial.println(error);
  // } else {
  //   // Serial.print("PM 2.5: ");
  //   // Serial.println(pm25);
  //   // Serial.print("PM 10: ");
  //   // Serial.println(pm10);
  // }
  delay(10);
  float temp_c = getTemperature();
  int error = mySDS.read(&pm25, &pm10);
  if (error) {
    Serial.println("Error retrieving data from PM");
  } else {

    send_packet_to_influxdb(temp_c, pm25, pm10);
  }
  delay(5 * 60 * 1000); // give the web browser time to receive the data
}
