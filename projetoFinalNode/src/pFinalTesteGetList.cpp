#include <Arduino.h>
#include "painlessMesh.h"//painlessMesh main librarry
//#include "namedMesh.h"
#include <ArduinoJson.h>//painlessMesh dependancy
#include <list>//to deal with the <uint32_t> list
#include <functional>//to create the event handler on compleation of message or timeout
#include <vector>//to create the event handler on compleation of message or timeout

#define   MESH_PREFIX     "MobileHub2Mesh"
#define   MESH_PASSWORD   "meshpassword"
#define   MESH_PORT       5555
#define   myChipNameSIZE    24
#define   LED             2       // GPIO number of connected LED, ON ESP-12 IS GPIO2

///////////////////////////new parameters
#define IS_ACTUATOR true //Define if the node is an actuaotor to the rest of the network

#define TIMEOUT_ACTUATOR 7000 //Define the timeout of the actuation confirmation
///////////////////////////end of new parameters

Scheduler userScheduler; // to control tasks
painlessMesh  mesh;

char myChipName[myChipNameSIZE]; // Chip name (to store MAC Address
//String nodeName(myChipName); // Name needs to be unique and uses the attached .h file


///////////////////////////////////////////////////////////////////Changes//////////////////////////////////////////////////////////////////
///////////////////start creating the event handler///////////////////

typedef std::function<void(String)> messageMiddleware;//define the type of callback function


class actuationManager {
public:
    void registerActuationCallback(messageMiddleware mm) {
        callbacks.push_back(mm);
    }

    void triggerActuationEvent(String message) {
        for (auto &mm : callbacks) {
            mm(message);
        }
    }

private:
    std::vector<messageMiddleware> callbacks;
};

actuationManager actuationMan;

void myActuationCallback1(String data) {
    Serial.print("Actuation callback triggered with data: ");
    Serial.println(data);
}

////////////////////end the creation the event handler//////////////////////////////

#define ARRAY_SIZE 3 // Define the size of the node array(std::list<uint32_t> Address,std::list<bool> , std::list<uint32_t> latency)

uint32_t actionCounter=0; //always start as zero and may be altered by the middleware at some point to keep things syncronized.

std::list<uint32_t> actuatorsListArray[ARRAY_SIZE];//define the list to base arround the reliability of the netwok

unsigned long Ttimeout=millis();
unsigned long tempoDeSincronia=millis();
unsigned long tempoDeSincronia2=millis();
bool noRepeatvar=false;//not to repeat percent strings

bool onFlag = false;//setCurrentAction

// Function to print the elements of a single list
void printList(const std::list<uint32_t>& list) {
    Serial.println("Elements in the list:");
    for (uint32_t value : list) {
        Serial.println(value);
    }
}


// Function to print the elements of each list the array
void printFullLists(std::list<uint32_t> list[], int arraySize) {
  for (int i = 0; i < arraySize; i++) {
        Serial.print("List ");
        Serial.print(i);
        Serial.println(":");
        for (uint32_t value : list[i]) {
            Serial.println(value);
        }
    }
}
///////////////////////////////////
bool checkForValueInSpecificList(std::list<uint32_t> list[], int arrayIndex, uint32_t checkValue) {
  for (uint32_t value : list[arrayIndex]) {
    if(value==checkValue){
      return true;
    }
  }
  return(false);
}

int countDiffValueInSpecificList(std::list<uint32_t> list[], int arrayIndex, uint32_t checkValue) {
  int funcCounter=0;
  for (uint32_t value : list[arrayIndex]) {
    if(value!=checkValue){
      funcCounter++;
    }
  }
  return(funcCounter);
}

int countSameValueInSpecificList(std::list<uint32_t> list[], int arrayIndex, uint32_t checkValue) {
  int funcCounter=0;
  for (uint32_t value : list[arrayIndex]) {
    if(value==checkValue){
      funcCounter++;
    }
  }
  return(funcCounter);
}
////////////////////////////////////////on new connection/topology change, the list of actuators need to be checked, therefore the actuators send a broadcast to inform all other nodes
void manageActuatorList(std::list<uint32_t> list[], uint32_t address, String incomingMessage) {
  if((incomingMessage=="ActuatorConfirmation") && not(checkForValueInSpecificList(list, 0, address))){
    list[0].push_back(address);
    list[1].push_back(0);
    list[2].push_back(0);
  }
}

void sendActuatorConfirmation(bool actuatorVerification){
  if(actuatorVerification){
    mesh.sendBroadcast("ActuatorConfirmation");
  }
}


/////////////////////////////////////

void sendActionRequest1(std::list<uint32_t> list[], uint32_t thisActionCounter, bool noRepeatvar) {//function sends out action request to see which acting nodes are available
  //uint32_t thisActionCounter = (uint32_t)myUnsignedLong;
  if(noRepeatvar){
    Serial.println("Not ready to send yet...");
    return;
  }
  int specPosition = 0;
  for (uint32_t value : list[0]) {
    if(mesh.isConnected( value )){//send first message to all actuator nodes in the list and if responded change the coresponding value of of the first actuator caonfirmation
      // Check if the specPosition is valid
      if (specPosition >= 0 && specPosition < list[1].size()) { //if the specPosition is Valid....
        std::list<uint32_t>::iterator it = list[1].begin(); // Start the iterator at the beginning of the list
        std::advance(it, specPosition); // Move the iterator to the desired position(specPosition)
        *it = thisActionCounter; // Change the value at the iterator's position to the value of the current action
      }
    }
    specPosition++;//keep track of the current position in the list
  }
}

void sendActionRequest2(std::list<uint32_t> list[], uint32_t thisActionCounter, unsigned long *timeout, bool *noRepeatvar) { //function sedns out full action request to nodes that responded the fist one
  //uint32_t thisActionCounter = (uint32_t)myUnsignedLong;
  if(*noRepeatvar){
    Serial.println("Not ready to send yet...");
    return;
  }
  String currentActionmessage="currentAction-" + String(thisActionCounter);
  int specPosition = 0;
  for (uint32_t value : list[1]) {
    if(value==thisActionCounter){//send second message to all actuator nodes that responded the first message
      // Check if the specPosition is valid
      if (specPosition >= 0 && specPosition < list[0].size()) { //if the specPosition is Valid....
        std::list<uint32_t>::iterator it = list[0].begin(); // Start the iterator at the beginning of the list
        std::advance(it, specPosition); // Move the iterator to the desired position(specPosition)
        mesh.sendSingle(*it, currentActionmessage);// use the value at the iterator's position to send a message
      }
    }
    specPosition++;//keep track of the current position in the list
  }
  *timeout=millis()+TIMEOUT_ACTUATOR;//begin timeout count set to the TIMEOUT_ACTUATOR at the start of the script
  *noRepeatvar=true;
}

void respondRequest(bool *onFlag, uint32_t address, String incomingMessage, bool actuatorVerification){
  if(incomingMessage.startsWith("currentAction-") && actuatorVerification){
    int dashIndex = incomingMessage.lastIndexOf('-');
    String numStr = incomingMessage.substring(dashIndex + 1);
    //int number = numStr.toInt();
    *onFlag=!*onFlag;
    digitalWrite(LED, *onFlag);
    mesh.sendSingle(address, "ActionDone-"+ numStr);
  }
}

void manageActionsOnReceive(std::list<uint32_t> list[], int arraySize, uint32_t address, uint32_t thisActionCounter, unsigned long *timeout, String incomingMessage) {//Function to keep the values of the last confirmation of the action as messages come.
  //uint32_t thisActionCounter = (uint32_t)myUnsignedLong;
  if(incomingMessage.startsWith("ActionDone-")){
    int dashIndex = incomingMessage.lastIndexOf('-');
    String numStr = incomingMessage.substring(dashIndex + 1);
    //int number = numStr.toInt();
    char charArray[20];
    numStr.toCharArray(charArray, 20);
    uint32_t recievedAction= strtoul(charArray, NULL, 10);
    Serial.print("ActionRecieved: ");
    Serial.println(recievedAction);
    if (*timeout<millis()){return;}//doesnt do anithing if timeout has already occured
    int specPosition = 0;
    for (uint32_t value : list[0]) {
      if(value==address){
        // Check if the specPosition is valid
        if (specPosition >= 0 && specPosition < list[2].size()) { //if the specPosition is Valid....
          std::list<uint32_t>::iterator it = list[2].begin(); // Start the iterator at the beginning of the list
          std::advance(it, specPosition); // Move the iterator to the desired position(specPosition)
          *it = recievedAction; // Change the value at the iterator's position to the value of the current action
          return;//after the required action is compleate return;
        }
      }
      specPosition++;//keep track of the current position in the list
    }
  }
}

String compleateOrTimeoutActions(uint32_t *actionCounter, unsigned long timeout, bool *noRepeatvar, std::list<uint32_t> list[]){
  if((countDiffValueInSpecificList(list, 2, *actionCounter)<1 || timeout<millis()) && *noRepeatvar){//if all the values inside the current action list are the same as the current actionCounter or timeout occurs, return the percentages to the middleware
    *noRepeatvar=false;
    String firstPercent="0";
    String secondPercent="0";
    Serial.println(*actionCounter);
    Serial.println(countSameValueInSpecificList(list, 1,*actionCounter));
    Serial.println(countSameValueInSpecificList(list, 2,*actionCounter));
    if(countSameValueInSpecificList(list, 1,*actionCounter) > 0 && list[1].size() > 0){
      int firstPercentnum=100*countSameValueInSpecificList(list, 1,*actionCounter)/list[1].size();
      firstPercent=String(firstPercentnum);
    }
    if(countSameValueInSpecificList(list, 1,*actionCounter) > 0 && countSameValueInSpecificList(list, 2, *actionCounter) > 0){
      int secondPercentnum=100*countSameValueInSpecificList(list, 2, *actionCounter)/countSameValueInSpecificList(list, 1, *actionCounter);
      secondPercent=(secondPercentnum);
    }
    String razoesASeremRetornadas = "Percentual nosDisponiveis/nosTotais: "+firstPercent+"%\nnosAtuaram/nosDisponiveis" + secondPercent+"%";
    *actionCounter=*actionCounter+1;
    return razoesASeremRetornadas;
  }
  return "NaN";
}

/////////////////////////////////////////////////////////////////////////end of changes/////////////////////////////////////////////////////////



void receivedCallback( uint32_t from, String &msg ) {
  manageActuatorList(actuatorsListArray, from, msg);
  manageActionsOnReceive(actuatorsListArray, 3, from, actionCounter, &Ttimeout, msg);
  respondRequest(&onFlag, from, msg, IS_ACTUATOR);
  Serial.printf("%s: Received from %u msg=%s\n", myChipName, from, msg.c_str());
}


void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("%s: New Connection, nodeId = %u\n", myChipName, nodeId);
    sendActuatorConfirmation(IS_ACTUATOR);
}

void changedConnectionCallback() {
  Serial.printf("%s: Changed connections\n", myChipName);
  sendActuatorConfirmation(IS_ACTUATOR);

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

  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  pinMode(LED, OUTPUT);

}


void loop() {
  // it will run the user scheduler as well
  mesh.update();

  //uncomment for a test withought external request test
/*
  if (millis()>tempoDeSincronia){
    Serial.print("Current Action Counter:");
    Serial.println(actionCounter);
    printFullLists(actuatorsListArray, 3);
    String finalMessage= compleateOrTimeoutActions(&actionCounter, Ttimeout, &noRepeatvar, actuatorsListArray);
    Serial.println(finalMessage);
    tempoDeSincronia=millis()+1500;
  }


  if (millis()>tempoDeSincronia2){
    sendActionRequest1(actuatorsListArray, actionCounter, noRepeatvar);
    sendActionRequest2(actuatorsListArray, actionCounter, &Ttimeout, &noRepeatvar);
    tempoDeSincronia2=millis()+10000;
    Serial.println("Era para estar indo...");
    Serial.println(actionCounter);
  }
*/

}
