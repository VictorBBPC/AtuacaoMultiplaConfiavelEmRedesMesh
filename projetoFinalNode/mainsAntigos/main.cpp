#include <Arduino.h>
#include "painlessMesh.h"
//#include "namedMesh.h"
#include <ArduinoJson.h>


#define   MESH_PREFIX     "CortesMesh"
#define   MESH_PASSWORD   "mesh2245013022393285"
#define   MESH_PORT       5555
#define   myChipNameSIZE    24
#define   LED             2       // GPIO number of connected LED, ON ESP-12 IS GPIO2
#define   BLINK_PERIOD    3000 // milliseconds until cycle repeat
#define   BLINK_DURATION  100  // milliseconds LED is on for


Scheduler userScheduler; // to control tasks
painlessMesh  mesh;

char myChipName[myChipNameSIZE]; // Chip name (to store MAC Address
//String nodeName(myChipName); // Name needs to be unique and uses the attached .h file

bool calc_delay = false;
SimpleList<uint32_t> nodes;


// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain

Task taskSendMessage( TASK_SECOND * 5 , TASK_FOREVER, &sendMessage );

// Task to blink the number of nodes
Task blinkNoNodes;
bool onFlag = false;


void sendMessage() {
  String msg = "Msg generated on ";
  msg += mesh.getNodeId();
  msg += " - myFreeMemory: " + String(ESP.getFreeHeap());
  mesh.sendBroadcast( msg );
  taskSendMessage.setInterval( random( TASK_SECOND * 4, TASK_SECOND * 8 ));

  // blink
  if (calc_delay) {
    SimpleList<uint32_t>::iterator node = nodes.begin();
    while (node != nodes.end()) {
      mesh.startDelayMeas(*node);
      node++;
    }
    calc_delay = false;
  }

  taskSendMessage.setInterval( random(TASK_SECOND * 1, TASK_SECOND * 5));  // between 1 and 5 seconds

}

void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("%s: Received from %u msg=%s\n", myChipName, from, msg.c_str());
}

//void receivedCallback_str( String &from, String &msg ) {
//  Serial.printf("%s: Received from %s msg=%s\n", myChipName, from.c_str(), msg.c_str());
//}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("%s: New Connection, nodeId = %u\n", myChipName, nodeId);
}

void changedConnectionCallback() {
  Serial.printf("%s: Changed connections\n", myChipName);

  //blink
  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
  blinkNoNodes.enableDelayed(BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD*1000))/1000);

  nodes = mesh.getNodeList();

  Serial.printf("Num nodes: %d\n", nodes.size());
  Serial.printf("Connection list:");

  SimpleList<uint32_t>::iterator node = nodes.begin();
  while (node != nodes.end()) {
    Serial.printf(" %u", *node);
    node++;
  }
  Serial.println();
  calc_delay = true;

}

void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("%s: Adjusted time %u. Offset = %d\n", myChipName, mesh.getNodeTime(),offset);
}

void setup() {
  Serial.begin(115200);

  // getting ESP32 details:
  uint64_t chipid = ESP.getEfuseMac();
  uint16_t chip = (uint16_t)(chipid >> 32);
  snprintf(myChipName, myChipNameSIZE, "SENSOR-BAT-%04X%08X", chip, (uint32_t)chipid);
  Serial.print("Hello! I am ");
  Serial.println(myChipName);
  ////////////////

  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );

 // mesh.setName(nodeName); // This needs to be an unique name!
  mesh.onReceive(&receivedCallback);
  //mesh.onReceive(&receivedCallback_str); //extra for names

  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();

  ///  Blink stuff
  pinMode(LED, OUTPUT);

    blinkNoNodes.set(BLINK_PERIOD, (mesh.getNodeList().size() + 1) * 2, []() {
      // If on, switch off, else switch on
      if (onFlag)
        onFlag = false;
      else
        onFlag = true;
      blinkNoNodes.delay(BLINK_DURATION);

      if (blinkNoNodes.isLastIteration()) {
        // Finished blinking. Reset task for next run
        // blink number of nodes (including this node) times
        blinkNoNodes.setIterations((mesh.getNodeList().size() + 1) * 2);
        // Calculate delay based on current mesh time and BLINK_PERIOD
        // This results in blinks between nodes being synced
        blinkNoNodes.enableDelayed(BLINK_PERIOD -
            (mesh.getNodeTime() % (BLINK_PERIOD*1000))/1000);
      }
  });
  userScheduler.addTask(blinkNoNodes);
  blinkNoNodes.enable();

  randomSeed(analogRead(A0));



}

void loop() {
  // it will run the user scheduler as well
  mesh.update();

  // Blink
  digitalWrite(LED, !onFlag);

}
