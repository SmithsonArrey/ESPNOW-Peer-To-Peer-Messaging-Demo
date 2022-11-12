/*
IOT Hello and Response via ESPNOW Peer 2 Peer Communication 
Author: Smithson Arrey

Hardware: 2 uPesy ESP32 WROOM Dev Kits, 4 Buttons, 4 LEDs (2 Green, 2 Yellow), Jumper wires, breadboard

Credit to Rui Santos for the project template
  Complete project details at https://RandomNerdTutorials.com/esp-now-two-way-communication-esp32/
*/

#include <esp_now.h>
#include <WiFi.h>

#include <Wire.h>

///move to ESP32_Device_Info.h
// REPLACE WITH THE MAC Address of your receiver 
uint8_t broadcastAddress[] = {0xEC, 0x94, 0xCB, 0x6D, 0x02, 0x00};

//MUSHROOM -> {0xEC, 0x94, 0xCB, 0x6D, 0x02, 0x00}
//BB-8 -> {0xEC, 0x94, 0xCB, 0x6D, 0xD5, 0x34}

/* issues with device hardware atm
 * program does not read input at pin2. might be reading but not on time 
 * - suggested: increase loop frequency
 * - resolved, I/O pins were mislabeled in code
 * Response LED and signal remains HIGH
 * - suggested: add a pull down resistor to this pin
 * - resolved, pull down resistors worked
*/
// Label Local Device
String device_name = "BB-8";

// Label I/O Pins

int tx_pin = 2; //transmit
int rx_pin = 4; //recieve
int rs_pin = 5; //respond

// Variable to store current digital state of pins
int rx_data;
int tx_data;
int rs_data;
int expiration_data;
// Variable to store the name of the sender device
String sender_name;

// Variable to store if sending data was successful
String success;

//Structure example to send data
//Must match the receiver structure
typedef struct struct_message {
    String sender_value;
    int tx_value; //the reciever will set value of rx to this value
    int rs_value; //the reciever will not modify any values but will print to the terminal
    int expiration_value; //time it takes for the message to disappear, commands no longer have effect, return to default state
} struct_message;

// Create a struct_message to hold incoming message
struct_message incoming_message;

// Create a struct_message to hold outgoing message
struct_message outgoing_message;

esp_now_peer_info_t peerInfo;

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status ==0){
    success = "Delivery Success :)";
  }
  else{
    success = "Delivery Fail :(";
  }
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incoming_message, incomingData, sizeof(incoming_message));
  Serial.print("Bytes received: ");
  Serial.println(len);
  
  rx_data = incoming_message.tx_value;
  rs_data = incoming_message.rs_value;
  sender_name = incoming_message.sender_value;
  expiration_data = incoming_message.expiration_value;

  digitalWrite(rx_pin, rx_data);
  //printMessageStruct("INCOMING", incoming_message);


  if (rs_data == 1){
     // Display Readings in Serial Monitor
     Serial.print(sender_name);
     Serial.println(" just responded to me!");
     rs_data = 0;
    }
}
 
void setup() {
  // Init Serial Monitor
  Serial.begin(115200);

  //configure all the I/O Pins

  pinMode(tx_pin, INPUT);
  pinMode(rx_pin, OUTPUT);
  pinMode(rs_pin, INPUT);

  digitalWrite(tx_pin, LOW);
  digitalWrite(rx_pin, LOW);
  digitalWrite(rs_pin, LOW);
    
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
}
 
void loop() {
  //create outgoing message
  outgoing_message = {device_name, digitalRead(tx_pin), digitalRead(rs_pin), 10}; //expires after 10 cycles
  
  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &outgoing_message, sizeof(outgoing_message));
   
  if (result == ESP_OK) {
    Serial.print(device_name);
    Serial.println(": Message sent with success");
    //printMessageStruct("OUTGOING", outgoing_message);
  }
  else {
    Serial.print(device_name);
    Serial.println(": Error sending the message");
  }
  updateSerialMonitor();
  if(expiration_data > 0){
    expiration_data--;
    }
  else if(expiration_data <= 0){
    rx_data = 0;
    digitalWrite(rx_pin, rx_data);
    rs_data = 0;
    }
  delay(10000);
}

void updateSerialMonitor(){
  //nothing happened, tx, rx, rs = 0, 0, 0 -- return
  if(digitalRead(tx_pin) == digitalRead(rs_pin) == digitalRead(rx_pin) == LOW){
    return;
    }
  
  //Saying Hello, tx, rx, rs = 1, x, x
  if(digitalRead(tx_pin) == HIGH){
    Serial.print(device_name);
    Serial.println(": Saying Hello!");
    }
  //Hearing Hello, tx, rx, rs = x, 1, 0
  else if((rx_data == 1) && (rs_data == 1)){
    Serial.print(device_name);
    Serial.print(": Nice to meet you, ");
    Serial.println(sender_name);
    }
  else if(rx_data == 1){
    Serial.print(sender_name);
    Serial.print(": Hello, ");
    Serial.println(device_name);
   }
  //Responding to Hello, tx, rx, rs = x, 1, 1

  //Recieving a Response tx, rx, rs = x, x, x -- special case, incoming_message.rs_value == 1 will autoprint an a response acknowledgement
}

void printMessageStruct(String msg_type, struct_message &MSG){
    Serial.println(msg_type);
    
    Serial.print(MSG.sender_value);
    Serial.print(",");
    Serial.print(MSG.tx_value);
    Serial.print(",");
    Serial.print(MSG.rs_value);
    Serial.print(",");
    Serial.println(MSG.expiration_value);

    Serial.println("END OF MESSAGE");
    
    Serial.print("Device State of ");
    Serial.println(device_name);

    Serial.print(tx_data);
    Serial.print(",");
    Serial.print(rx_data);
    Serial.print(",");
    Serial.println(rs_data);
    
    Serial.println("END OF DEVICE STATE");
  }
