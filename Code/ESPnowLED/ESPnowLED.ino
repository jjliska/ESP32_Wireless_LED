/* Joshua Liska
 * 
 * References:
 * https://randomnerdtutorials.com/esp-now-esp32-arduino-ide/
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html
 * 
 */

//MAC Address Left: 7C:9E:BD:E3:1A:F8
//MAC Address Right: 7C:9E:BD:E3:06:D0

#include <esp_now.h>
#include <WiFi.h>

//Uncomment the address of the opposing device (IE code on the left needs the rights broadcast address)
//Left:
//uint8_t broadcastAddress[] = {0x7C, 0x9E, 0xBD, 0xE3, 0x1A, 0xF8};
//Right:
uint8_t broadcastAddress[] = {0x7C, 0x9E, 0xBD, 0xE3, 0x06, 0xD0};

// Pin Declaration
const int buttonPin = 15;

const int ledBlue = 13; 
const int ledWhite = 12;
const int ledRed = 14;

// Button Change variables
int lastButtonState = HIGH;
int buttonState;

// Timer delays and initilization
const int buttonDelay = 20;
const int ledDelay = 250;
const int connectionDelay = 1000;

unsigned long connectionTimer = millis();
unsigned long buttonTimer = millis();
unsigned long ledTimer = millis();

// Determines if the system is on  or off: true = on/ false = off
bool systemState = false;
int pingPongState = 0;

// Other system variables and system state change checks
bool otherSystemState = false;
bool otherSystemStateLast = false;
int otherPingPongState = 0;

// Required object type for ESP_now
typedef struct struct_message {
    bool systemState;
    int pingPongState;
} struct_message;

//Status of current system and other system
struct_message currentSystem;
struct_message otherSystem;

// Send data, ie other system is on/ no errors
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&otherSystem, incomingData, sizeof(otherSystem));
  Serial.print("Bytes received: ");
  Serial.println(len);
  otherSystemState = otherSystem.systemState;
  otherPingPongState = otherSystem.pingPongState;

  Serial.println(otherSystemState);
  Serial.println(otherPingPongState);

  // Checks for resetting on systemChange
  // (This seem to help marginally but there will occasionally be some lag in the system that causes a significant offset).
  if(otherPingPongState != pingPongState) pingPongState = otherPingPongState;

  if(otherSystemState != otherSystemStateLast){
    Serial.println("otherSystemState change!");
    otherSystemStateLast = otherSystemState;
    reset();
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledBlue, OUTPUT);
  pinMode(ledWhite, OUTPUT);
  pinMode(ledRed, OUTPUT);

  WiFi.mode(WIFI_STA);

  // Initializing esp_now to allow for reg/add a peer
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  statusProbe();
  buttonReader();
  ledState();
}


void statusProbe(){
  if(millis()-connectionTimer >= connectionDelay){
    
    currentSystem.systemState = systemState;
    currentSystem.pingPongState = pingPongState;
    
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &currentSystem, sizeof(currentSystem));
    
    connectionTimer = millis();
  }
}

// Reads button state change and stops bouncing.
void buttonReader(){
  buttonState = digitalRead(buttonPin);

  if(millis()-buttonTimer >= buttonDelay){
    if (buttonState != lastButtonState) {
      if (buttonState == HIGH) {
        if(systemState) systemState = false;
        else systemState = true;
        reset();
      }
    }
    lastButtonState = buttonState;
    buttonTimer = millis();
  }
}

// 
void ledState(){
  if(millis()-ledTimer >= ledDelay){
    if(systemState){
      if(!otherSystemState) singleDevice();
      else dualDevice();
    }
    else reset();
    ledTimer = millis();
  }
}

// Resets the system
void reset(){
  turnOff();
  pingPongState = 0;
  ledTimer = millis();
}

// Frequently used led reset+
void turnOff(){
  digitalWrite(ledBlue, LOW);
  digitalWrite(ledWhite, LOW);
  digitalWrite(ledRed, LOW);
}

// Flips the leds left, right, off
void singleDevice(){
  if(pingPongState < 3)pingPongState++;
  else pingPongState = 0;
  
  switch(pingPongState){
    case 0:
      digitalWrite(ledBlue, HIGH);
      digitalWrite(ledWhite, HIGH);
      digitalWrite(ledRed, LOW);
      break;
    case 1:
      turnOff();
      break;
    case 2:
      digitalWrite(ledBlue, LOW);
      digitalWrite(ledWhite, HIGH);
      digitalWrite(ledRed, HIGH);
      break;
    default:
      turnOff();
      break;
  }
}

// Flips the leds based on device
void dualDevice(){
  if(pingPongState < 1) pingPongState++;
  else pingPongState = 0;
  
  switch(pingPongState){
    case 0:
      // Uncomment when uploading to left object
      
      digitalWrite(ledBlue, HIGH);
      digitalWrite(ledWhite, HIGH);
      digitalWrite(ledRed, LOW);
      
      //turnOff();
      break;
    default:
      // Uncomment when uploading to right object
      /*
      digitalWrite(ledBlue, LOW);
      digitalWrite(ledWhite, HIGH);
      digitalWrite(ledRed, HIGH);
      */
      turnOff();
      break;
  }
}
