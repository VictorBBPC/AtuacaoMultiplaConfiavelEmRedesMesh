//************************************************************
// this is a simple example that uses the painlessMesh library
//
// 1. sends a silly message to every node on the mesh at a random time between 1 and 5 seconds
// 2. prints anything it receives to Serial.print
//
//
//************************************************************
#include <Arduino.h>
#include "painlessMesh.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <BLEUtils.h>
#include <string.h>

#define MAX_STRINGS 10 // Maximum number of strings
#define MAX_STRING_LENGTH 50 // Maximum length of each string
char buffer[MAX_STRINGS][MAX_STRING_LENGTH]; // Create a 2D array to hold the strings
int bufferIndex=0;


// Define the UUID for your service and characteristic
#define SERVICE_UUID "2fc03570-8ae7-407f-a375-3d2d74d8fc0f"
#define CHARACTERISTIC_UUID "1fc03570-8ae7-407f-a375-3d2d74d8fc0f"

//BLEServer* pServer;
BLECharacteristic* pCharacteristic;
bool deviceConnected = false;
const int pin = 5;
unsigned long duration;
unsigned long starttime;
unsigned long sampletime_ms = 2000;//sampe 30s ;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;
//char pm25[30];
String pm25="Hello information!\n";
String pm26="";
unsigned int contBLEmessage=0;
bool tokenMensageMesh=false;


class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};


#define   MESH_SSID     "CortesMesh"
#define   MESH_PASSWORD   "mesh2245013022393285"
#define   MESH_PORT       5555

// Prototypes
void sendMessage();
void receivedCallback(uint32_t from, String & msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void delayReceivedCallback(uint32_t from, int32_t delay);

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;


bool calc_delay = false;
SimpleList<uint32_t> nodes;

// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain

Task taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, &sendMessage );

void sendMessage() {
  String msg = "Hello from node ";
  msg += mesh.getNodeId();
  msg +=" And with the information: ";
  msg += String(random(0, 10));
  mesh.sendBroadcast( msg );
  taskSendMessage.setInterval( random( TASK_SECOND * 1, TASK_SECOND * 5 ));
}

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
  pm25=String(from)+","+msg.c_str()+","+mesh.getNodeId();

  if (pm25.length() < MAX_STRING_LENGTH) {
    pm25.toCharArray(buffer[bufferIndex], MAX_STRING_LENGTH); // Save the string into the buffer
    if(bufferIndex<49){bufferIndex++;}
    else{bufferIndex=0;}
    tokenMensageMesh=true;
  }
  else {
      Serial.println("The string is too large...");
  }

}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

void setup() {
  Serial.begin(115200);

//mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
 // mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages
    mesh.setDebugMsgTypes(ERROR | DEBUG);  // set before init() so that you can see error messages
  Serial.println("setou o debugger....");
  mesh.init( MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT );
  Serial.println("ate o init foi....");
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();


  //begin bluetooth LE setup----------------------------------------------------------------------------------------------------------------------
    pinMode(pin, INPUT);

    // Initialize the BLE device
    BLEDevice::init("ESP32 BLE");

    // Create a BLE server
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create a BLE service
    BLEService* pService = pServer->createService(SERVICE_UUID);

    pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE
                    );

    pCharacteristic->addDescriptor(new BLE2902());

    pCharacteristic->setValue("Hello World");

    // Start the service
    pService->start();

    // Start advertising
    pServer->getAdvertising()->start();
  //end bluetooth LE setup-----------------------------------------------------------------------------------------------------------------------
}

void loop() {
  // it will run the user scheduler as well
  mesh.update();


    if (deviceConnected) {//Device connected to BLE
        /*
      double density = 1.65*pow(10,12);
      double K = 3531.5;
      double r25 = 0.44*pow(10,-6);
      double vol25 = (4.0/3.0)*M_PI*pow(r25,3);
      double mass25 = density*vol25;
      float concSmall = (concentration)*K*mass25;
      */
      //sprintf(pm25, "%f", concSmall);

      /*
      contBLEmessage=contBLEmessage+1;
      pm26=String(contBLEmessage);
      pm25=pm26;
      */

      if(tokenMensageMesh){
      tokenMensageMesh=false;

/*
      int commaIndex = 0;
      int lastCommaIndex = 0;

      while (commaIndex != -1) {
        commaIndex = pm25.indexOf(',', lastCommaIndex);
        String substring;
        if (commaIndex == -1) {
          substring = pm25.substring(lastCommaIndex);
        } else {
          substring = pm25.substring(lastCommaIndex, commaIndex);
          lastCommaIndex = commaIndex + 1;
        }

      int str_len = substring.length() + 1;
      char char_array[str_len];
      substring.toCharArray(char_array, str_len);

      pCharacteristic->setValue(char_array);
      pCharacteristic->notify();
      Serial.println("Enviando mensagem BLE");
      Serial.println(substring);
      delay(750); // Adjust the delay to control the update rate
      }
      */

      int str_len = pm25.length() + 1;
      char char_array[str_len];
      pm25.toCharArray(char_array, str_len);

      pCharacteristic->setValue(char_array);
      pCharacteristic->notify();
      Serial.println("Enviando mensagem BLE");
      Serial.println(pm25);
      delay(750); // Adjust the delay to control the update rate
      }
    }
}
