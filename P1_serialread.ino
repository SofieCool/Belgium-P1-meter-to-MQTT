#include <SoftwareSerial.h> // Allows serial communication on other digital pins
#include <string.h>         // Library for the strncmp() and strlen() functions

// Constants
#define SERIAL_RX 2                     // Data pin on the ESP8266
#define MAX_TELEGRAM_LINE_LENGTH 1024   // Maximum length of one line of the telegram
#define MAX_VALUE_LENGTH 64             // Maximum length of one value and unit

// Global variables
char telegramLine[MAX_TELEGRAM_LINE_LENGTH]; 

// Communication objects
SoftwareSerial p1Port(SERIAL_RX, -1, false);          // Initializes the serial connection with the P1 port

// Function to get the value from the telegramLine
// This function is optional and only used if you want to print the value (in the loop() function)
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
}

// Function to read the P1 port in a loop and send the data to the MQTT broker
// This function is automatically called in a loop after the setup() function
void loop() {
  while (p1Port.available()) {                            // Check the connection with the P1 port
    memset(telegramLine, 0, sizeof(telegramLine));                            // Clear the telegramLine
    p1Port.readBytesUntil('\n', telegramLine, MAX_TELEGRAM_LINE_LENGTH);      // Read data from the P1 port until the end of the line (\n) and write the data in telegramLine
    yield();                                                                  // Keep background functions from the ESP8266 active during the infinite loop

    Serial.println(telegramLine);                         // Prints the full telegramLine
    // Uncomment these 4 lines if you want to print only the value from the telegramLine
    //char value[MAX_VALUE_LENGTH]; 
    //memset(value, 0, sizeof(value)); 
    //getValue(value, telegramLine);
    //Serial.println(value);
  }
}

