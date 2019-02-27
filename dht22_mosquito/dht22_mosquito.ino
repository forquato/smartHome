#include "DHT.h"

#include <ESP8266WiFi.h>
#include <MQTTClient.h> //https://github.com/256dpi/arduino-mqtt

// Adafruit si7021
#include "Adafruit_Si7021.h" //si7021 sensor
Adafruit_Si7021 sisensor = Adafruit_Si7021();

// Adafruit BME280
#include <Wire.h>
#include <SPI.h>  // kann womöglich auskomentiert werden
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
// ebenso Adafruit BME280
#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
#define SEALEVELPRESSURE_HPA (1027.25)
Adafruit_BME280 bme; // I2C


//#define WIFI_SSID       "FRITZ!Box 7490"
//#define WIFI_PASS       "1337skillz"
#define WIFI_SSID       "FritzInKa"
#define WIFI_PASS       "04935557248837671843"

WiFiClient WiFiclient;
MQTTClient client;

int eingang = A0;

#define MQTT_SERVER      "192.168.178.99"
#define MQTT_SERVERPORT  1883                   // use 8883 for SSL
#define MQTT_USERNAME  "openhabian"
#define MQTT_KEY       "openhabian"

#define DHTPIN 0     // what digital pin the DHT22 is conected to
#define DHTTYPE DHT22   // there are multiple kinds of DHT sensors

DHT dht(DHTPIN, DHTTYPE);


void setup() {

  Serial.begin(9600);

  // initialize si7021
  if (!sisensor.begin())
  {
    Serial.println("Did not find Si7021 sensor!");
    while (true)
      ;
  }


  // initialize BME280
  bool status;
  status = bme.begin();  
    if (!status) {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        while (1);
    }

  /**
   * To try to reduce this, let’s switch off the WiFi radio at the beginning 
   * of the setup() function, keep it off while we’re reading the sensors, 
   * and switch it back on when we are ready to send the results to the server.
   * */

  WiFi.mode( WIFI_OFF );
  WiFi.forceSleepBegin();
  delay(1);
  
  
  Serial.setTimeout(2000);
  delay(1);

  // Wait for serial to initialize.
  while(!Serial) { }
  
  // connect to Wifi
  connect();

  //mqtt
  //see also https://github.com/256dpi/arduino-mqtt
  client.begin(MQTT_SERVER, WiFiclient);

  //client.setWill("/dht_22/service","Connection interrupted");
  //client.setOptions(200, true, 1000);

  Serial.print("\nconnecting to mqqt broker...");
  while (!client.connect("dht22_sensor", "openhabian", "openhabian")) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nconnected!");


  checkTemperature();
  checkLight();
  
  
  // Sleep
  Serial.println("ESP8266 in sleep mode");

  WiFi.disconnect( true );
  delay( 1 );
  // WAKE_RF_DISABLED to keep the WiFi radio disabled when we wake up
  ESP.deepSleep( 240 * 1000000, WAKE_RF_DISABLED );


}

void loop() {

}

void checkLight(){

  client.publish("/dht_22/light", String(analogRead(eingang)), true, 1);

}

void checkTemperature(){

    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t)) {
      return;
    }
    
    float hic = dht.computeHeatIndex(t, h, false);

    //dht22
    client.publish("/dht_22/temp", String(t));
    client.publish("/dht_22/humidity", String(h));
    client.publish("/dht_22/heatindex", String(hic));

    // si 7021
    client.publish("/si7021/temp", String(sisensor.readTemperature()));
    client.publish("/si7021/humidity", String(sisensor.readHumidity()));

    //bme280
    client.publish("/bme280/temp", String(bme.readTemperature()));
    client.publish("/bme280/pressure", String(bme.readPressure() / 100.0F));
    client.publish("/bme280/altitude", String(bme.readAltitude(SEALEVELPRESSURE_HPA)));
    client.publish("/bme280/humidity", String(bme.readHumidity()));

    //Serial.println("Temp: " + String(bme.readTemperature()));

    client.publish("/dht_22/service", "Succesfully Read Sensor.");
  
}

void checkThings(){
    
    client.publish("/dht_22/service", "Start Checks for Wifi and Mqtt."); 
    
    // check whether wifi is connected
    if(WiFi.status() != WL_CONNECTED){
      connect();
      client.publish("/dht_22/service", "Wifi Reconnected");        
    }
    
    checkBrokerConnection();
  
}


// conect to a wifi network
void connect() {

  IPAddress ip( 192, 168, 178, 44 );
  IPAddress gateway( 192, 168, 178, 1 );
  IPAddress subnet( 255, 255, 255, 0 );


  WiFi.forceSleepWake();
  delay( 1 );

  // Disable the WiFi persistence.  The ESP8266 will not load and save WiFi settings in the flash memory.
  WiFi.persistent( false );

  // Connect to Wifi.
  //Serial.println();
  //Serial.print("Connecting to ");
  //Serial.println(WIFI_SSID);

  // WiFi fix: https://github.com/esp8266/Arduino/issues/2186
  //WiFi.persistent(false);
  //WiFi.mode(WIFI_OFF);

  WiFi.mode(WIFI_STA);
  WiFi.config( ip, gateway, subnet );
  WiFi.begin(WIFI_SSID, WIFI_PASS);



  unsigned long wifiConnectStart = millis();

  while (WiFi.status() != WL_CONNECTED) {
    // Check to see if
    if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println("Failed to connect to WIFI. Please verify credentials: ");
      Serial.println();
      Serial.print("SSID: ");
      Serial.println(WIFI_SSID);
      Serial.print("Password: ");
      Serial.println(WIFI_PASS);
      Serial.println();
    }

    delay(500);
    Serial.print(".");
    // Only try for 5 seconds.
    if(millis() - wifiConnectStart > 50000) {
      Serial.println("Failed to connect to WiFi");
      Serial.println("Please attempt to send updated configuration parameters.");
      return;
    }
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void checkBrokerConnection(){

     if (!client.connected()){  
      client.publish("/dht_22/service", "Mqtt not Connected. Try to reconnect.", true, 1);
      while (!client.connect("dht22_sensor", "openhabian", "openhabian")) {
        delay(1000);
    }
      client.publish("/dht_22/service", "Mqtt Reconnected");
    }
  
}
