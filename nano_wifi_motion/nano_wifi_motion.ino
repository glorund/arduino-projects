#include <WiFiNINA.h>
#include <PubSubClient.h>
#include "secrets.h"

//To run with NANO 33 IoT
//please enter your sensitive data in the Secret tab
//please enter your sensitive data in the Secret tab
char ssid[] = SECRET_SSID;                // your network SSID (name)
char pass[] = SECRET_PASS;                // your network password (use for WPA, or use as key for WEP)
char serverHost[] = SERVER_HOST;
int serverPort = SERVER_PORT;
char clientName[] = "arduino-motion-sensor";
int status = WL_IDLE_STATUS;              // the Wi-Fi radio's status
int ledState = LOW;                       //ledState used to set the LED
unsigned long previousMillisInfo = 0;     //will store last time Wi-Fi information was updated
unsigned long previousMillisLED = 0;      // will store the last time LED was updated
const int intervalInfo = 5000;            // interval at which to update the board information
int sensor = 2;              // the pin that the sensor is atteched to
int state = LOW;             // by default, no motion detected
int val = 0;                 // variable to store the sensor status (value)

WiFiClient wifiClient;
PubSubClient client(wifiClient);

void esteblishWIFiConnection() {
  if (status != WL_CONNECTED) {
    // attempt to connect to Wi-Fi network:
    while (status != WL_CONNECTED) {
      Serial.print("Attempting to connect to network: ");
      Serial.println(ssid);
      // Connect to WPA/WPA2 network:
      status = WiFi.begin(ssid, pass);

      // wait 10 seconds for connection:
      delay(10000);
    }

    // you're connected now, so print out the data:
    Serial.println("You're connected to the network");
    Serial.println("---------------------------------------");
  } 
}

void esteblishMQTTConnection() {
  // connect to MQTT
  boolean mqttc = false; 
  while (!mqttc) {
    Serial.println("Connecting MQTT broker");
    mqttc = client.connect(clientName);
    if (mqttc) {
      // connection succeeded
      Serial.println("Connected ");
      //boolean r= client.subscribe("test");
      //Serial.println("subscribe ");
      //Serial.println(r);
    } 
    else {
      // connection failed
      // mqttClient.state() will provide more information
      // on why it failed.
      Serial.println("Connection failed ");
      Serial.println(client.state());
    }
    delay(5000);
  }
  
}

void setup() {
  //Initialize serial and wait for port to open:
  pinMode(sensor, INPUT);    // initialize sensor as an input
  Serial.begin(9600);
  while (!Serial);
  client.setServer(serverHost,serverPort); 

  // set the LED as output
  pinMode(LED_BUILTIN, OUTPUT);

  esteblishWIFiConnection();

  esteblishMQTTConnection();
}

void loop() {
  unsigned long currentMillisInfo = millis();

  // check if the time after the last update is bigger the interval
  if (currentMillisInfo - previousMillisInfo >= intervalInfo) {
    previousMillisInfo = currentMillisInfo;

    Serial.println("Board Information:");
    // print your board's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print your network's SSID:
    Serial.println();
    //Serial.println("Network Information:");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    //Serial.print("signal strength (RSSI):");
    //Serial.println(rssi);
    //Serial.println("---------------------------------------");
  }
  
  unsigned long currentMillisLED = millis();
  
  // measure the signal strength and convert it into a time interval
  int intervalLED = WiFi.RSSI() * -10;
 
  // check if the time after the last blink is bigger the interval 
  if (currentMillisLED - previousMillisLED >= intervalLED) {
    previousMillisLED = currentMillisLED;

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
    // set the LED with the ledState of the variable:
    digitalWrite(LED_BUILTIN, ledState);
    val = digitalRead(sensor);   // read sensor value
      delay(500);                // delay 100 milliseconds 
    if (val == HIGH) {           // check if the sensor is HIGH
      if (state == LOW) {
        char out_msg[100] ="Motion detected!";
        Serial.println("Motion detected!"); 
        state = HIGH;       // update variable state to HIGH
        boolean rc = client.publish("test", out_msg );
        Serial.println(rc); 
      }
    } 
    else {
       if (state == HIGH){
          Serial.println("Motion stopped!");
          char out_msg[100] ="Motion stopped!";
          boolean rc = client.publish("test", out_msg );
          Serial.println(rc); 
          state = LOW;       // update variable state to LOW
      }
    }
  }
}
