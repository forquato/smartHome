#include "DHT.h"

#include <ESP8266WiFi.h>
#include <MQTTClient.h> //https://github.com/256dpi/arduino-mqtt

//#define WIFI_SSID       "FRITZ!Box 7490"
//#define WIFI_PASS       "1337skillz"

#define WIFI_SSID       "FritzInKa"
#define WIFI_PASS       "04935557248837671843"

WiFiClient WiFiclient;
MQTTClient client;


#define MQTT_SERVER      "192.168.178.99"
#define MQTT_SERVERPORT  1883                   // use 8883 for SSL
#define MQTT_USERNAME  "openhabian"
#define MQTT_KEY       "openhabian"

#define DHTPIN 4     // what digital pin the DHT22 is conected to
#define DHTTYPE DHT22   // there are multiple kinds of DHT sensors

int repeat = 240;

//int ledG = 15;
//int ledR = 13;

DHT dht(DHTPIN, DHTTYPE);


void setup() {
  Serial.begin(9600);
  Serial.setTimeout(2000);
  delay(10);

  // Wait for serial to initialize.
  while(!Serial) { }

  Serial.println("Device Started");
  Serial.println("-------------------------------------");
  Serial.println("Running DHT!");
  Serial.println("-------------------------------------");


  //pinMode(ledG, OUTPUT);
  //pinMode(ledR, OUTPUT);

  // connect to Wifi
  connect();

  //mqtt
  //see also https://github.com/256dpi/arduino-mqtt
  client.begin(MQTT_SERVER, WiFiclient);

  client.setWill("/dht_22/service","Connection interrupted");
  client.setOptions(repeat*2+100, true, 1000);

  Serial.print("\nconnecting to mqqt broker...");
  while (!client.connect("dht22_sensor", "openhabian", "openhabian")) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nconnected!");


}



int interval = repeat*1000;
int timeSinceLastRead = interval;
void loop() {

  // Report every x seconds.
  if(timeSinceLastRead > interval) {

    // check whether wifi is connected
    if(WiFi.status() != WL_CONNECTED){
      connect();
      client.publish("/dht_22/service", "Wifi Reconnected", true, 1);        
    }

    if (!client.connected()){  
      while (!client.connect("dht22_sensor", "openhabian", "openhabian")) {
        Serial.print(".");
        delay(1000);
    }
      client.publish("/dht_22/service", "Mqtt Reconnected", true, 1);
    }
    
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    
    // Read temperature as Fahrenheit (isFahrenheit = true)
    //float f = dht.readTemperature(true);

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t)) {
      //Serial.println("Failed to read from DHT sensor!");
      timeSinceLastRead = interval-2000;
      //led(100,ledR);
      return;
    }
    

    //led(30,ledG);
  
    // Compute heat index in Fahrenheit (the default)
    //float hif = dht.computeHeatIndex(f, h);
    // Compute heat index in Celsius (isFahreheit = false)
    float hic = dht.computeHeatIndex(t, h, false);

    //Serial.print("Humidity: ");
    //Serial.print(h);
    //Serial.print(" %\t");
    //Serial.print("Temperature: ");
    //Serial.print(t);
    //Serial.print(" *C ");
    //Serial.print(f);
    //Serial.print(" *F\t");
    //Serial.print("Heat index: ");
    //Serial.print(hic);
    //Serial.print(" *C ");
    //Serial.print(hif);
    //Serial.println(" *F");

    //Serial.println("");

    client.publish("/dht_22/temp", String(t));
    client.publish("/dht_22/humidity", String(h));
    client.publish("/dht_22/heatindex", String(hic));

     client.publish("/dht_22/service", "Succesfully Read Sensor.", true, 1);


    timeSinceLastRead = 0;

  }
  delay(100);
  timeSinceLastRead += 100;
  
}


// blink a led
void led(int del, int led){
    digitalWrite(led, HIGH);
    delay(del);
    digitalWrite(led, LOW);
  
}

// conect to a wifi network
void connect() {

  // Connect to Wifi.
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  // WiFi fix: https://github.com/esp8266/Arduino/issues/2186
  //WiFi.persistent(false);
  //WiFi.mode(WIFI_OFF);
  //WiFi.mode(WIFI_STA);
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
