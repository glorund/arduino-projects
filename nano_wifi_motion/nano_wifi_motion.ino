#include <WiFiNINA.h>
#include <PubSubClient.h>
#include "secrets.h"

//To run with NANO 33 IoT
//please enter your sensitive data in the Secret tab
//please enter your sensitive data in the Secret tab
char ssid[] = SECRET_SSID;                // your network SSID (name)
char pass[] = SECRET_PASS;                // your network password (use for WPA, or use as key for WEP)
char strBuf[50];
char serverHost[] = SERVER_HOST;
int serverPort = SERVER_PORT;
char clientName[] = "arduino-motion-sensor";
int status = WL_IDLE_STATUS;              // the Wi-Fi radio's status
int ledState = LOW;                       //ledState used to set the LED
unsigned long previousMillisInfo = 0;     //will store last time Wi-Fi information was updated
unsigned long previousMillisLED = 0;      // will store the last time LED was updated
const int intervalInfo = 15000;            // interval at which to update the board information 86.400 in a day. 17.280 5 sec intervals
int countDown = 500;
int blinkNumber = 1;
int sensor = 2;              // the pin that the sensor is atteched to
int state = LOW;             // by default, no motion detected
int val = 0;                 // variable to store the sensor status (value)
boolean serialEnabled = false;

void(* resetFunc) (void) = 0;//declare reset function at address 0 resetFunc(); //call reset

WiFiClient wifiClient;
PubSubClient client(wifiClient);

void esteblishWIFiConnection() {
  if (status != WL_CONNECTED) {
    // attempt to connect to Wi-Fi network:
    while (status != WL_CONNECTED) {
      logger("Attempting to connect to network: ");
      loggerLn(ssid);
      // Connect to WPA/WPA2 network:
      status = WiFi.begin(ssid, pass);
      // wait 10 seconds for connection:
      // adaptive delay      
      dinymicDelay();
    }

    // you're connected now, so print out the data:
    loggerLn("You're connected to the network");
    loggerLn("---------------------------------------");
  } 
}

void dinymicDelay() {
  int i = 0;
  do {
    delay(1000);
    i++;
    logger("i:");
    loggerLn(i);
    loggerLn(status);
  } 
  while(!(status == WL_CONNECTED || status == WL_CONNECT_FAILED) && i <= 20 );
}

void esteblishMQTTConnection() {
  // connect to MQTT
  while (!client.connected()) {
    loggerLn("Connecting MQTT broker");
    if (client.connect(clientName)) {
      // connection succeeded
      logToServer("Connected");
      //boolean r= client.subscribe("test");
      //loggerLn("subscribe ");
      //loggerLn(r);
    } 
    else {
      // connection failed
      // mqttClient.state() will provide more information
      // on why it failed.
      loggerLn("Connection failed ");
      loggerLn(client.state());
    }
    delay(5000);
  }
}

void reconnectMQTT() {
//Loop until we're reconnected*
  while (!client.connected()) {
    logger("Attempting MQTT connection...");
    // Attempt to connect*
    //  if (client.connect("ESPClient", MQTT_USERNAME, MQTT_KEY)) {
    loggerLn("connected"); 
    // Subscribe or resubscribe to a topic*_
    // You can subscribe to more topics (to control more outputs)*
    // client.subscribe("esp8266/bathroom/output"); 
    //} else {
    logger("failed, rc=");
    logger(client.state());
    loggerLn(" try again in 5 seconds");
    // Wait 5 seconds before retrying*
    delay(5000);
  }
// }
}

void setup() {
  //Initialize serial and wait for port to open:
  pinMode(sensor, INPUT);    // initialize sensor as an input
  Serial.begin(9600);
  //while (!Serial);
  client.setServer(serverHost,serverPort); 
  // set the LED as output
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {

  if (status != WL_CONNECTED) {
    esteblishWIFiConnection();
  }  
 
  if (!client.connected()) {
    esteblishMQTTConnection();
  }

  unsigned long currentMillisInfo = millis();

  // check if the time after the last update is bigger the interval
  if (currentMillisInfo - previousMillisInfo >= intervalInfo) {
    previousMillisInfo = currentMillisInfo;
    
    sprintf(strBuf, "server ping %d count down %d",previousMillisInfo, countDown);
    logToServer(strBuf);
    countDown -= 1;

    if (countDown <= 0) {
        countDown = 500;
        logToServer("Restarting. sending OFF message");
        boolean rc = publishToServer("OFF");
        logToServer("Restarting.");
        NVIC_SystemReset(); 
    }

    loggerLn("Board Information:");
    // print your board's IP address:
    IPAddress ip = WiFi.localIP();
    logger("IP Address: ");
    loggerLn(ip);

    // print your network's SSID:
    loggerLn();
    //loggerLn("Network Information:");
    logger("SSID: ");
    loggerLn(WiFi.SSID());

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    //logger("signal strength (RSSI):");
    //loggerLn(rssi);
    //loggerLn("---------------------------------------");
    logger("MQTT: ");
    loggerLn(client.connected());

  }

  unsigned long currentMillisLED = millis();
  
  // measure the signal strength and convert it into a time interval
  int intervalLED = WiFi.RSSI() * -10;
 
  // check if the time after the last blink is bigger the interval 
  if (currentMillisLED - previousMillisLED >= intervalLED) {
    previousMillisLED = currentMillisLED;
    
    // if the LED is off turn it on and vice-versa:
    if (state == HIGH ) { // if motion detected
      if (blinkNumber % 2 == 0 ) {
        logger("s");
        blinkNumber = 1;
        if (ledState == LOW) {
          ledState = HIGH;
        } else {
          ledState = LOW;
        }
      } else {
        blinkNumber+=1; 
      }
    } else { // no motion 
      blinkNumber = 1;
      if (ledState == LOW) {
        ledState = HIGH;
      } else {
        ledState = LOW;
      }
    }
    // set the LED with the ledState of the variable:
    digitalWrite(LED_BUILTIN, ledState);
  }
  val = digitalRead(sensor);   // read sensor value
  if (val == HIGH) {           // check if the sensor is HIGH
    if (state == LOW) {
      loggerLn("Motion detected!"); 
      state = HIGH;       // update variable state to HIGH
      boolean rc = publishToServer("ON");
      loggerLn(rc); 
    }
  } else {
    if (state == HIGH){
      loggerLn("Motion stopped!");
      boolean rc = publishToServer("OFF");
      loggerLn(rc); 
      state = LOW;       // update variable state to LOW
    }
  }
  delay(100);                  // delay 100 milliseconds 
}

boolean publishToServer(const char* payload) {
  return client.publish("motion/bathroom", payload);
}

boolean logToServer(const char* payload) {
  if (serialEnabled){
    Serial.println(payload);
  }
  return client.publish("motion/bathroom/logger", payload);
}


void loggerLn() {
  if (serialEnabled){
    Serial.println();
  }
}

void loggerLn(char line[]) {
  if (serialEnabled){
    Serial.println(line);
  }
}

void loggerLn(int line) {
  if (serialEnabled){
    Serial.println(line);
  }
}

void loggerLn(boolean line) {
  if (serialEnabled){
    Serial.println(line);
  }
}

void logger(char line[]) {
  if (serialEnabled){
    Serial.print(line);
  }
}

void logger(int line) {
  if (serialEnabled){
    Serial.print(line);
  }
}

void loggerLn(IPAddress& line) {
  if (serialEnabled){
    Serial.println(line);
  }
}