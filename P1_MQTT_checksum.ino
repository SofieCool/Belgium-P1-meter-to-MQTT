#include <SoftwareSerial.h> // Allows serial communication on other digital pins
#include <ESP8266WiFi.h>    // Handles Wifi communication
#include <PubSubClient.h>   // Handles MQTT communication
#include <string.h>         // Library for the strncmp() and strlen() functions

// Constants
#define SERIAL_RX 2                     // Data pin on the ESP8266
#define MAX_OBISCODE_LENGTH 16          // Maximum length of one OBIS code
#define MAX_VALUE_LENGTH 64             // Maximum length of one value and unit
#define MAX_TELEGRAM_LINE_LENGTH 1024   // Maximum length of one line of the telegram
#define MAX_MQTT_TOPIC_LENGTH 64        // Maximum length of one MQTT topic
#define NUMBER_OF_DATAOBJECTS 23        // Number of different data objects in one telegram

// Global variables
char telegramLine[MAX_TELEGRAM_LINE_LENGTH]; 
unsigned int currentCRC = 0;

// Structure definition for a data object
struct DataObject {
  char obisCode[MAX_OBISCODE_LENGTH];     // String for OBIS code from the data object  
  char mqttTopic[MAX_MQTT_TOPIC_LENGTH];  // String for MQTT topic to publish the value on
  char value[MAX_VALUE_LENGTH];           // String for the value from the data object
};

// Array of all data objects from a telegram
// Value is initialized with "" and will be filled while running the program.
struct DataObject dataObjects[NUMBER_OF_DATAOBJECTS] = {
  {"0-0:96.1.4", "meter/id", ""},
  {"1-0:1.8.1", "meter/electricity/consumption/day", ""},
  {"1-0:1.8.2", "meter/electricity/consumption/night", ""},
  {"1-0:2.8.1", "meter/electricity/production/day", ""},
  {"1-0:2.8.2", "meter/electricity/production/night", ""},
  {"0-0:96.14.0", "meter/electricity/current_rate", ""},
  {"1-0:1.7.0", "meter/electricity/consumption/all_phases", ""},
  {"1-0:2.7.0", "meter/electricity/production/all_phases", ""},
  {"1-0:21.7.0", "meter/electricity/consumption/phase1", ""},
  {"1-0:22.7.0", "meter/electricity/production/phase1", ""},
  {"1-0:32.7.0", "meter/electricity/voltage/phase1", ""},
  {"1-0:52.7.0", "meter/electricity/voltage/phase2", ""},
  {"1-0:72.7.0", "meter/electricity/voltage/phase3", ""},
  {"1-0:31.7.0", "meter/electricity/current/phase1", ""},
  {"1-0:51.7.0", "meter/electricity/current/phase2", ""},
  {"1-0:71.7.0", "meter/electricity/current/phase3", ""},
  {"0-0:96.3.10", "meter/electricity/switch_position", ""},
  {"0-0:17.0.0", "meter/electricity/max_allowed_power_phase", ""},
  {"1-0:31.4.0", "meter/electricity/max_allowed_current_phase", ""},
  {"0-0:96.13.0", "meter/message", ""},
  {"0-1:24.1.0", "meter/other_devices_on_bus", ""},
  {"0-1:24.4.0", "meter/gas/switch_position", ""},
  {"0-1:24.2.3", "meter/gas/reading", ""}
};

// Communication objects
SoftwareSerial p1Port(SERIAL_RX, -1, false);          // Initializes the serial connection with the P1 port
WiFiClient wifiClient;                                // Initializes the WiFi client
PubSubClient mqttClient(wifiClient);                  // Initializes the MQTT client

// Communication constants
const char* WIFI_SSID = "YOUR WIFI SSID"; 
const char* WIFI_PASSWORD = "YOUR WIFI PASSWORD";
const char* MQTT_BROKER = "YOUR MQTT BROKER IP ADRESS";  
const char* MQTT_CLIENT = "ESP8266Client";            // If you have multiple ESPs running, change this client name to a unique name in your network
const char* MQTT_USERNAME = "YOUR MQTT USERNAME";     // IF you have no username on the broker, write NULL
const char* MQTT_PASSWORD = "YOUR MQTT PASSWORD";     // IF you have no password on the broker, write NULL


// Function to connect the ESP8266 with the router over WiFi
void setupWiFi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

// Function to connect the ESP8266 with the MQTT broker
void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect(MQTT_CLIENT, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("connected");  
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Function to calculate the CRC16 code
unsigned int calculateCRC16(unsigned int crc, unsigned char *buf, int len) {
	for (int pos = 0; pos < len; pos++) {
		crc ^= (unsigned int)buf[pos];

		for (int i = 8; i != 0; i--) {
			if ((crc & 0x0001) != 0) {
				crc >>= 1;
				crc ^= 0xA001;
			} else {
				crc >>= 1;
      }
		}
	}

	return crc;
}

// Function to check the calculate the CRC for every line from the telegram
bool checkLine(int len) {
  char* startLine = strchr(telegramLine, '/');  // Check if the telegramLine is the first line of the telegram
  char* endLine = strchr(telegramLine, '!');    // Check if the telegramLine is the last line of the telegram
  bool validCRC = false;

  if (startLine) {
    // Clear all values in dataObjects 
    for (int i = 0; i < NUMBER_OF_DATAOBJECTS; i++) {
      memset(dataObjects[i].value, 0, sizeof(dataObjects[i].value));     
    }

    currentCRC = calculateCRC16(0x0000, (unsigned char*) telegramLine, len);       // Start CRC calculation
  } else if (endLine) {
    currentCRC = calculateCRC16(currentCRC, (unsigned char*) telegramLine, 1);    // Finish CRC calculation

    char messageCRC[5];                         
    strncpy(messageCRC, endLine + 1, 4);                                          // Save checksum code from telegram in hex
    messageCRC[4] = 0; 

    validCRC = (strtol(messageCRC, NULL, 16) == currentCRC);                      // Compare checksum code from telegram with CRC calculation
    if (validCRC) {
      Serial.println("VALID CRC FOUND!"); 
    } else {
      Serial.println("INVALID CRC FOUND!");
    }
      
    currentCRC = 0;
  } else {
    currentCRC = calculateCRC16(currentCRC, (unsigned char*) telegramLine, len);   // Add CRC calculation with every incoming telegramLine 
  }

  return validCRC;
}

// Function to get the value from the telegramLine
void getValue(char* value, char* telegramLine) {
  char* start = strchr(telegramLine, '(');        // Find the start position from the value

  char* end = strchr(telegramLine, '*');          // Find the end position from the value
  if (end == NULL) {
    end = strchr(telegramLine, ')');
  }

  if (start == NULL || end == NULL) {             // If no start or end is found, return and do nothing
    return;
  }

  strncpy(value, start+1, end-start-1);           // Write the value into the value variable 
}

// Function to publish all the values from the telegram to the MQTT broker
void publishTelegram() {
  for (int i = 0; i < NUMBER_OF_DATAOBJECTS; i++) {
    if (dataObjects[i].value[0]) {
      mqttClient.publish(dataObjects[i].mqttTopic, dataObjects[i].value);
    }
  }
}

// Function to setup the communication with the P1 port, the router and MQTT broker
// This function is called automatically when the ESP8266 starts up
void setup() {
  Serial.begin(115200);
  p1Port.begin(115200, SWSERIAL_8N1, SERIAL_RX, -1, false, MAX_TELEGRAM_LINE_LENGTH);

  if (!p1Port) {
    Serial.println("Invalid EspSoftwareSerial pin configuration, check config"); 
    while (1) { 
      delay(1000);
    }
  } 
  setupWiFi();
  mqttClient.setServer(MQTT_BROKER, 1883);
}

// Function to read the P1 port in a loop and send the data to the MQTT broker
// This function is automatically called in a loop after the setup() function
void loop() {
  while (p1Port.available()) {                            // Check the connection with the P1 port
    if (!mqttClient.connected() || !mqttClient.loop()) {  // Check the connection with the MQTT broker
      connectMQTT();
    }

    memset(telegramLine, 0, sizeof(telegramLine));                                      // Clear the telegramLine
    int len = p1Port.readBytesUntil('\n', telegramLine, MAX_TELEGRAM_LINE_LENGTH);      // Read data from the P1 port until the end of the line (\n), write the data in telegramLine and save the length of the data in len
    telegramLine[len] = '\n';                                                           // Add \n at the end of the telegramLine (for the CRC calculation)
    telegramLine[len+1] = 0;                                                            // Add 0 at the end of the telegramLine (to signify the new end of the array)
    yield();                                                                            // Keep background functions from the ESP8266 active during the infinite loop

    // Compare telegramLine with OBIS code and store the value in dataObjects
    for (int i = 0; i < NUMBER_OF_DATAOBJECTS; i++) {
      if (strncmp(telegramLine, dataObjects[i].obisCode, strlen(dataObjects[i].obisCode)) == 0) {
        getValue(dataObjects[i].value, telegramLine);

        //Serial.println(dataObjects[i].mqttTopic);
        //Serial.println(dataObjects[i].value);
        break;
      }
    }

    // If this is the last line and the telegram is correct, publish the values to the MQTT broker  
    if (checkLine(len+1)) {
      publishTelegram();
    }
  }
}

