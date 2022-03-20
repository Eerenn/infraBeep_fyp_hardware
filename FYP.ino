#include <WiFi.h>
#include <Preferences.h>
#include "BluetoothSerial.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>

String ssids_array[50];
String network_string;
String connected_string;

const char* pref_ssid = "";
const char* pref_pass = "";
const char* pref_topic = "";
String client_wifi_ssid;
String client_wifi_password;
String client_topic = "";

const char* bluetooth_name = "robot01";

long start_wifi_millis;
long wifi_timeout = 10000;
bool bluetooth_disconnect = false;

enum wifi_setup_stages { NONE, SCAN_START, SCAN_COMPLETE, SSID_ENTERED, WAIT_PASS, WAIT_TOPIC, TOPIC_ENTERED, PASS_ENTERED, WAIT_CONNECT, LOGIN_FAILED, CONNECTED };
enum wifi_setup_stages wifi_stage = NONE;

BluetoothSerial SerialBT;
Preferences preferences;

//API call with json
char jsonOutput[1024];
String serverNamePayload = "https://5v4wl41qb6.execute-api.ap-southeast-1.amazonaws.com/test/payload";
String serverNameDevice = "https://5v4wl41qb6.execute-api.ap-southeast-1.amazonaws.com/test/device/";
DynamicJsonDocument dynamicJsonDoc(1024);

//MQTT receiver declaration
#include <IRremote.h>
#include <PubSubClient.h>
int status = WL_IDLE_STATUS;

const int port = 1883;
const char* mqttUser = "user";
const char* mqttPassword = "";
const char* broker = "broker.mqtt-dashboard.com";

WiFiClient espClient;
PubSubClient client(espClient);
IRsend irsend(4);

const int RECV_PIN = 15;
IRrecv irrecv(RECV_PIN);
decode_results results;
bool isReceive = false; //check if mqtt is in receive state
bool isTopicSubbed = false; //check if already subscribe to topic
String message;

int led = 2;
//getArray function variables
int sentencePost =0;
String val;
int count = 0;
unsigned int arr[120];

//mqtt callback for subscriber
void callback(char* topic, byte* payload, unsigned int length) {

  Serial.print("Message received in topic: ");
  Serial.print(topic);
  Serial.print("   length is:");
  Serial.println(length);

  Serial.print("Data Received From Broker:");
  for (int i = 0; i < length; i++) {
    message+=((char)payload[i]);
    Serial.print((char)payload[i]);
  }
  if(message == "1") { //if received message is 1
    isReceive = true;  //set mode to receiver mode
    Serial.println("");
    Serial.println("Receiver mode");
  } else if(message.indexOf("payload")> 0 ) { //if received message consist of "payload" keyword
    //sample payload "1c867c19-6407-42a7-8b83-628193fdb539/payload/button01"
    HTTPClient http;

    String serverPath = serverNameDevice + message; //construct http string
    http.begin(serverPath.c_str());                 //begin http with constructed string
    Serial.println(serverPath);
    int httpResponseCode = http.GET();              //launch http get

    if(httpResponseCode > 0){
      Serial.print("HTTP Response Code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();            //call back value from http get request
      
      deserializeJson(dynamicJsonDoc, payload);     //json deserialize
      JsonObject obj = dynamicJsonDoc.as<JsonObject>(); //convert into json object
      String rawlen  = obj[String("rawlen")];       //set json object into variable
      int khz  = obj[String("frequency")];
      getArray(rawlen);                             //invoke function from d_ConstructIR
      irsend.sendRaw(arr, sizeof(arr) / sizeof(arr[0]), khz); //send IR signal with IR transmitter
    } else {                                        //else if http fails
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
      
      String payload = http.getString();
    }
    http.end();
  }
  Serial.println();
  message="";
}

//default setup function
void setup()
{
  Serial.begin(115200);
  Serial.println("Booting...");

  pinMode(led, OUTPUT);
  //set mqtt
  client.setServer(broker, 1883 ); //default port for mqtt is 1883
  client.setCallback(callback);

  //check if instantiate wifi successful
  preferences.begin("wifi_access", false);
  if (!init_wifi()) { // Connect to Wi-Fi fails
    Serial.println("wifi failed");
    SerialBT.register_callback(callback);
    SerialBT.begin(bluetooth_name);
  } else {
    Serial.println("wifi connected");
    delay(1000);
    irrecv.enableIRIn();  //enable IR receiver
  }
}

//default loop funciton
void loop()
{
  if (bluetooth_disconnect)
  {
    disconnect_bluetooth();
  }

  switch (wifi_stage)  //enum class
  {
    case SCAN_START: //start scan for netowrk
      SerialBT.println("Scanning Wi-Fi networks");
      Serial.println("Scanning Wi-Fi networks");
      scan_wifi_networks();
      SerialBT.println("Please enter the number for your Wi-Fi");
      wifi_stage = SCAN_COMPLETE;  //set stage to scan complete, invoke callback function in b_setup
      break;

    case SSID_ENTERED: //after user selected SSID, wait for user enter password
      SerialBT.println("Please enter your Wi-Fi password");
      Serial.println("Please enter your Wi-Fi password");
      wifi_stage = WAIT_PASS; //set stage to wait password, invoke callback function in b_setup
      break;

    case WAIT_TOPIC: //after user input password, wait for user enter topic for MQTT
      SerialBT.println("Please enter your device topic (TOPIC MUST BE UNIQUE)");
      Serial.println("Please enter your device topic (TOPIC MUST BE UNIQUE)");
      wifi_stage = TOPIC_ENTERED; //set stage to topic entered, invoke callback function in b_setup
      break;

    case PASS_ENTERED: //after user entered all input
      SerialBT.println("Please wait for Wi-Fi connection...");
      Serial.println("Please wait for Wi_Fi connection...");
      wifi_stage = WAIT_CONNECT;
      preferences.putString("pref_ssid", client_wifi_ssid); //put String into const variable
      preferences.putString("pref_pass", client_wifi_password);
      preferences.putString("pref_topic", client_topic);
      if (init_wifi()) { // Connected to WiFi
        connected_string = "ESP32 IP: ";
        connected_string = connected_string + WiFi.localIP().toString();
        SerialBT.println(connected_string);
        Serial.println(connected_string);
        bluetooth_disconnect = true;

        //MQTT receiver
        Serial.println("Start Receiving");
        wifi_stage = CONNECTED; //set stage to connected
        
      } else { // try again
        wifi_stage = LOGIN_FAILED;  //set stage to login failed
      }
      break;

    case LOGIN_FAILED:
      SerialBT.println("Wi-Fi connection failed");
      Serial.println("Wi-Fi connection failed");
      delay(2000);
      wifi_stage = SCAN_START; //set stage to scan start, invoke callback function in b_setup
      break;

    case CONNECTED:
    if ( !client.connected() )
      {
      reconnect();
      }

      if(isReceive) {
          digitalWrite(led, HIGH);
          if (irrecv.decode(&results)){                             //if IR receiver receives any signal
              HTTPClient http;
              http.begin(serverNamePayload);                        //launch http request 
              http.addHeader("Content-Type", "application/json");   //add header to the request

              const size_t CAPACITY = JSON_OBJECT_SIZE(6)+768;      // declare a size for Json document
              StaticJsonDocument<CAPACITY> doc;
              
              String rawVal = dumpCode(&results);                   // Output the results as source code, raw data of the signal
              Serial.println(rawVal);
              JsonObject object = doc.to<JsonObject>();
              object["name"] = "from esp32";                        //insert name to the object
              object["rawlen"] = rawVal;                            //insert raw data to the object
              object["hexVal"] = String(results.value, HEX);        //insert hex value of the signal to the object
              object["decodeType"] = String(results.decode_type);   //insert the type of signal to the object
              object["frequency"] = String(results.bits);           //insert the frequency used by the signal to the object
              
              serializeJson(doc, jsonOutput);                       //the object into JSON string
              int httpCode = http.POST(String(jsonOutput));         //POST the JSON string to cloud server
              if(httpCode > 0) {                                    //httpCode > 0 equals post successful
                  String uuid = http.getString();                   //the callback value from the http request is a uuid of the post object
                  Serial.println("\nStatuscode:" + String(httpCode));
                  Serial.println(uuid);
                  http.end();                                       //close the http connection

                  delay(500);
                  char attributes[40];
                  uuid.toCharArray( attributes, 40 );               //converts the uuid into a char array 
                  client.publish("getUUID", attributes);            //publish the uuid to MQTT broker via topic "getUUID"
                }
                else {
                  Serial.println("ERROR HTTP REQUEST");
                }
              irrecv.resume();                                      //resume in receiver mode
        }
      } else {
        digitalWrite(led, LOW);
        isTopicSubbed = subTopic(isTopicSubbed); //invoke function from c_MQTT
        client.loop();
      }
        break;
  }
}
