#include <SoftwareSerial.h>
#include <SPI.h>
#include <MFRC522.h>
#include <AccelStepper.h>

// Pin definitions for MFRC522
#define RST_PIN 9
#define SS_PIN 10

// Define the motor pins
#define MP1  4 // IN1 on the ULN2003
#define MP2  5 // IN2 on the ULN2003
#define MP3  6 // IN3 on the ULN2003
#define MP4  7 // IN4 on the ULN2003

// Create instances
SoftwareSerial reyaxSerial(3, 2);
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Create stepper motor instance
#define MotorInterfaceType 8
AccelStepper stepper = AccelStepper(MotorInterfaceType, MP1, MP3, MP2, MP4);
const int SPR = 2048;

String keySlots[3] = {"", "", ""};
const int maxSlots = 3;

unsigned long lastCode = 0;  // Last used code
unsigned long codeIncrement = 1;  // Increment value for rolling code
const unsigned long maxCode = 4294967295;  // Maximum value for unsigned long

void setup() {
  Serial.begin(9600);
  reyaxSerial.begin(9600);
  delay(500);
  
  SPI.begin();
  mfrc522.PCD_Init();
  
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(230);

  initializeReyax();
}

void loop() {
  // Check for incoming data from Reyax
  if (reyaxSerial.available()) {
    String response = reyaxSerial.readString();
    Serial.print("Reyax response: ");
    Serial.println(response);

    if (response.startsWith("+RCV=")) {
      Serial.println("Parsing LoRa message...");
      parseLoRaMessage(response);
    } else if (response.startsWith("REQUEST_REGISTERS")) {
      sendRegistersToReyax();
    }
  }
  
  // RFID tag detection
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String uid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      uid += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "") + String(mfrc522.uid.uidByte[i], HEX);
    }

    bool keyFound = false;
    for (int i = 0; i < maxSlots; i++) {
      if (keySlots[i] == uid) {
        Serial.print("Access granted: Key is designated in slot ");
        Serial.println(i == 0 ? "EXX" : (i == 1 ? "EYX" : "EZX"));
        keyFound = true;
        startStepperMotorForDuration(10);
        break;
      }
    }

    if (!keyFound) {
      String sendCommand = generateSendCommand(uid);
      reyaxSerial.print(sendCommand);
      Serial.print("Sent to Reyax: ");
      Serial.println(sendCommand);
    }

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }
}

String generateSendCommand(String uid) {
  lastCode += codeIncrement;
  if (lastCode > maxCode) {
    lastCode = 0;  // Roll back to 0 if max reached
  }
  return "AT+SEND=0,100," + uid + "," + String(lastCode) + "\r\n";
}

// Function to start the stepper motor for a given duration in seconds
void startStepperMotorForDuration(unsigned long duration) {
  unsigned long startTime = millis();
  stepper.moveTo(3 * SPR); // Move to a position equivalent to 3 revolutions
  stepper.runToPosition(); // Run the motor to the target position
  
  while (millis() - startTime < duration * 1000) {
    stepper.run(); // Keep the motor running
  }

  stepper.moveTo(-3 * SPR); // Move back to the original position
  stepper.runToPosition(); // Run the motor to the target position
}

// Function to send the contents of the registers to the other Reyax
void sendRegistersToReyax() {
  String message = "REGISTER_VALUES:";
  for (int i = 0; i < maxSlots; i++) {
    if (i > 0) {
      message += ", ";
    }
    message += (i == 0 ? "EXX-" : (i == 1 ? "EYX-" : "EZX-")) + (keySlots[i].length() == 0 ? "EMPTY" : keySlots[i]);
  }

  String sendCommand = "AT+SEND=0,100," + message + "\r\n";
  reyaxSerial.print(sendCommand);
  Serial.print("Sent register values to Reyax: ");
  Serial.println(sendCommand);
}

// Function to parse the LoRa message and handle commands
void parseLoRaMessage(String message) {
  Serial.println("Inside parseLoRaMessage function.");
  
  message.trim();
  
  // Adjust parsing to exclude rolling code
  int startDataIndex = message.indexOf("EXX-");
  if (startDataIndex == -1) {
    startDataIndex = message.indexOf("EYX-");
  }
  if (startDataIndex == -1) {
    startDataIndex = message.indexOf("EZX-");
  }
  if (startDataIndex == -1) {
    startDataIndex = message.indexOf("remove EXX");
  }
  if (startDataIndex == -1) {
    startDataIndex = message.indexOf("remove EYX");
  }
  if (startDataIndex == -1) {
    startDataIndex = message.indexOf("remove EZX");
  }

  if (startDataIndex != -1) {
    String action;
    if (message.indexOf("EXX-") == startDataIndex) {
      action = "EXX";
      startDataIndex += strlen("EXX-");
    } else if (message.indexOf("EYX-") == startDataIndex) {
      action = "EYX";
      startDataIndex += strlen("EYX-");
    } else if (message.indexOf("EZX-") == startDataIndex) {
      action = "EZX";
      startDataIndex += strlen("EZX-");
    } else if (message.indexOf("remove EXX") == startDataIndex) {
      action = "remove EXX";
      startDataIndex += strlen("remove EXX");
    } else if (message.indexOf("remove EYX") == startDataIndex) {
      action = "remove EYX";
      startDataIndex += strlen("remove EYX");
    } else if (message.indexOf("remove EZX") == startDataIndex) {
      action = "remove EZX";
      startDataIndex += strlen("remove EZX");
    }
    
    // Find the rolling code separator (comma) and parse UID correctly
    int rollingCodeIndex = message.indexOf('-', startDataIndex);
    if (rollingCodeIndex != -1) {
      // Move start index to the character after '-'
      startDataIndex = rollingCodeIndex + 1;

      // Find the end of the UID (next comma or end of message)
      int uidEndIndex = message.indexOf(',', startDataIndex);
      if (uidEndIndex == -1) {
        uidEndIndex = message.indexOf('\r', startDataIndex);
      }
      if (uidEndIndex == -1) {
        uidEndIndex = message.indexOf('\n', startDataIndex);
      }
      
      // Extract UID, excluding the rolling code
      String uid = message.substring(startDataIndex, uidEndIndex);
      uid.trim();

      Serial.print("Command: ");
      Serial.println(action);

      if (action == "EXX") {
        setKeySlot(0, uid);
      } else if (action == "EYX") {
        setKeySlot(1, uid);
      } else if (action == "EZX") {
        setKeySlot(2, uid);
      } else if (action == "remove EXX") {
        clearKeySlot(0);
      } else if (action == "remove EYX") {
        clearKeySlot(1);
      } else if (action == "remove EZX") {
        clearKeySlot(2);
      } else {
        Serial.println("Unknown command.");
      }

      sendAcknowledgment(uid + " processed in slot " + action);
      sendRegistersToReyax();
    } else {
      Serial.println("Rolling code not found.");
    }
  } else {
    Serial.println("Data does not contain a recognizable command.");
  }
}

// Function to set a key in a specific slot
void setKeySlot(int slot, String uid) {
  if (slot >= 0 && slot < maxSlots) {
    keySlots[slot] = uid;
  }
}

// Function to clear a key from a specific slot
void clearKeySlot(int slot) {
  if (slot >= 0 && slot < maxSlots) {
    keySlots[slot] = "";
  }
}

// Function to send acknowledgment via LoRa
void sendAcknowledgment(String message) {
  String sendCommand = "AT+SEND=0,100," + message + "\r\n";
  reyaxSerial.print(sendCommand);
  Serial.print("Sent acknowledgment to Reyax: ");
  Serial.println(sendCommand);
}

// Function to initialize Reyax settings
void initializeReyax() {
  reyaxSerial.println("AT+BAND?");  // Sending a basic AT command
  delay(2000);
  reyaxSerial.println("AT+CPIN?");
  delay(2000); 
  reyaxSerial.println("AT+NETWORKID=13");
  delay(2000);
  reyaxSerial.println("AT+NETWORKID?");
  delay(2000);
  reyaxSerial.println("AT+ADDRESS=111");
  delay(2000);
  reyaxSerial.println("AT+ADDRESS?");
  delay(2000);
}
