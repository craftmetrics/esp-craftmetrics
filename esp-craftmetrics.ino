#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif
#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#endif

#include <WiFiClientSecure.h>
#include <OneWire.h>

// ---- Edit these parameters -----------
#define WIFI_SSID "MyWifi"
#define WIFI_PASSWORD "MyWifiPassword"
#define CM_ACCOUNT "mybrewery"
#define CM_DEVICE_USERNAME "mybrewery_device"
#define CM_DEVICE_PASSWORD "pA$5w0Rd!"
#define DEVICE_NAME "mysensor"
// --------------------------------------
#define CM_HOSTNAME "data.craftmetrics.ca"
#define CM_PORT 443

#define ONE_WIRE_PIN 2
OneWire  ds(ONE_WIRE_PIN);

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void loop() {
  // The metrics payload takes the form:
  //     mydevice field=value,field=value,field=value,...
  //
  // Numeric values can have a "i" appended to make them integers.
  // Otherwise they will be stored as floats.
  //
  // Start the payload, and include the device uptime as the first field.
  String metricsPayload = String(DEVICE_NAME) + " uptime=" + String(millis()) + "i";

  // We can add the device's free memory as another field.
  metricsPayload += String(",freeheap=") + String(ESP.getFreeHeap()) + "i";
  

  // If the sensor read is successful, append the value as another field
  // on the metrics payload. If it's not successful, it will be omitted.
  float temp;
  if (readDefaultSensor(ds, &temp) > 0) {
    metricsPayload += String(",probe=") + String(temp, 3);
  }


  if (WiFi.status() != WL_CONNECTED) {
    // If we have no wifi, return now. Try again during the next loop().
    Serial.println("Connecting to wifi...");
    delay(2000);
    return;
  }
  Serial.println("Wifi connected to " + WiFi.SSID() + " IP:" + WiFi.localIP().toString());


  // If we made if this far, we have wifi.
  // We can add the signal strength as another field in the payload
  metricsPayload += String(",rssi=") + String(WiFi.RSSI()) + "i";

  // Print the payload to serial console, and send it up to Craft Metrics
  Serial.println(metricsPayload);
  sendToCraftMetrics(metricsPayload);

  delay(10000);
}


// Make network connection to Craft Metrics to send the payload
#define TIMEOUT 5000
void sendToCraftMetrics(String payload) {
  WiFiClientSecure client;
  
  #ifdef ESP8266
  // For this example, we don't bother to set up X509 cert validation on ESP8266.
  // On ESP32 this is default behaviour.
  client.setInsecure();
  #endif
  
  if (!client.connect(CM_HOSTNAME, CM_PORT)) {
    Serial.println("Connection Failed");
    return;
  }
  Serial.println(String("Connected to ") + CM_HOSTNAME + ":" + CM_PORT);
  delay(50);

  String out = String("POST /write?db=") +CM_ACCOUNT+ "&u=" +CM_DEVICE_USERNAME+ "&p=" +CM_DEVICE_PASSWORD+ " HTTP/1.1\r\n";
  out += String("Host: ") + CM_HOSTNAME + ":" + CM_PORT + "\r\n";
  out += "User-Agent: esp8266-craftmetrics\r\n";
  out += "Accept: */*\r\n";
  out += String("Content-Length: ") + payload.length() + "\r\n\r\n";
  out += payload;
  client.print(out);

  for (int i=0; i<TIMEOUT; i+=50) {
    if (client.available()) {
      break;
    }
    delay(50);
  }

  while (client.available()) {
    Serial.print(client.readStringUntil('\r'));
  }
  client.stop();
}

// Reads data from the first sensor found
// Returns >0 on success, <0 on failure
#define DS18B20_RET_OK (1)
#define DS18B20_RET_CRC (-1)
#define DS18B20_RET_OUT_RANGE (-2)
#define DS18B20_RET_NO_SENSOR (-3)
int readDefaultSensor(OneWire ds, float *temp) {
  float f;
  int ret = DS18B20_RET_NO_SENSOR;
  byte addr[8];

  // Instruct all sensors to take a reading
  ds.reset();
  ds.skip();
  ds.write(0x44, 1); 
  delay(800);

  ds.reset();
  if (ds.search(addr) == true) {
    Serial.print(addrToString(addr) + " ");
    ret = readSensor(ds, addr, temp);
  }
  while (ds.search(addr) == true) {}
  Serial.println(ret);
  return ret;
}

// Reads data from sensor addr `a` and puts the temperature in temp (in celsius)
int readSensor(OneWire ds, byte a[8], float *temp) {
  ds.reset();
  ds.select(a);
  ds.write(0xBE); // Read Scratchpad

  byte data[9];
  for (int i=0; i<9; i++) {
    data[i] = ds.read();
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  byte calculatedCRC = OneWire::crc8(data, 8);
  if (calculatedCRC != data[8]) {
    Serial.print("CRC Failed: ");
    Serial.print(calculatedCRC, HEX);
    Serial.print(" != ");
    Serial.println(data[8], HEX);
    return DS18B20_RET_CRC;
  }

  uint8_t reg_msb = data[1];
  uint8_t reg_lsb = data[0];
  uint16_t TReading = reg_msb << 8 | reg_lsb;

  int SignBit, Whole, Fract;
  SignBit = TReading & 0x8000;  // test most sig bit
  if (SignBit) {
    // negative
    TReading = (TReading ^ 0xffff) + 1; // 2's comp
  }

  Whole = TReading >> 4;  // separate off the whole and fractional portions
  Fract = (TReading & 0xf) * 100 / 16;

  *temp = Whole + (TReading & 0xf) / 16.;
  if (SignBit) {
    *temp *= -1;
  }

  return DS18B20_RET_OK;
}

String addrToString(byte addr[8]) {
  char addr_hex[17];
  sprintf(
    addr_hex,
    "%02x%02x%02x%02x%02x%02x%02x%02x",
    addr[0],
    addr[1],
    addr[2],
    addr[3],
    addr[4],
    addr[5],
    addr[6],
    addr[7]
  );
  return String(addr_hex);
}
