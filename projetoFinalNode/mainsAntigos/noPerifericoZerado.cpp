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

bool dia=true;
bool movimento=false;
static int pinoLDR = 34;
const int pinoPIR = 35; //PINO DIGITAL UTILIZADO PELO SENSOR DE PRESENCA
/*
//begin classic bluetooth setup-----------------------------------------------------------------------------------------------------------------------
#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

//unsigned int lastOCC=0;
//uint8_t messageBL;
//end classic bluetooth setup-----------------------------------------------------------------------------------------------------------------------
*/
#define   MESH_SSID     "CortesMesh"
#define   MESH_PASSWORD   "mesh2245013022393285"
#define   MESH_PORT       5555

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;

// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain

Task taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, &sendMessage );

int messageNum=0;

void sendMessage() {

  /*
  String msg = "Hello from node ";
  msg += mesh.getNodeId();
  msg +=" And with the information: ";
  msg += String(random(0, 10));
  */
  //inicio da mod----------------------------------------------------
  /*
    String msg = "Hello from node!";
    if (messageNum==1){
      messageNum=2;
      msg=String(1);
    }
    else{
      if (messageNum==2){
        messageNum=3;
        msg=String(random(0, 10));
      }
      else{
        messageNum=1;
        msg=mesh.getNodeId();
      }
    }
    */
    //final da mod-----------------------------------

    //Inicio da mod nova --------------------------
    String msg = "Hello from node!";
      msg=String(1)+","+String(0)+","+String(0)+","+String(0);
    //final da mod nova-----------------------------------
  mesh.sendBroadcast( msg );
  Serial.printf("Enviando mensagem pela rede MESH\n");
  Serial.printf("Enviando dia: %s\n", dia ? "true" : "false");
  taskSendMessage.setInterval( random( TASK_SECOND * 1, TASK_SECOND * 5 ));
}

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
/*  //messageBL= *msg.c_str();
    if (SerialBT.available()) {
      Serial.write(*msg.c_str());
    }
    */
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
  Serial.println("STARTUP...");

//mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();


  pinMode(pinoLDR, INPUT); //DEFINE O PINO COMO ENTRADA
  pinMode(pinoPIR, INPUT); //DEFINE O PINO COMO ENTRADA
/*
//begin classic bluetooth setup-----------------------------------------------------------------------------------------------------------------------
  SerialBT.begin("ESP32test"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");
//end classic bluetooth setup-----------------------------------------------------------------------------------------------------------------------
*/

}

void loop() {
  // it will run the user scheduler as well
  //Serial.println(analogRead(pinoLDR));
  if(analogRead(pinoLDR) < 600){ //SE O VALOR LIDO FOR MAIOR QUE 600, FAZ
    dia=true;
  }
  else{ //SENÃO, FAZ
    dia=false;
  }

  if(digitalRead(pinoPIR) == HIGH){ //SE A LEITURA DO PINO FOR IGUAL A HIGH, FAZ
    movimento=true; //ACENDE O LED
  }else{ //SENÃO, FAZ
    movimento=false; //APAGA O LED
  }


  mesh.update();

/*
  if (millis()>lastOCC+2000){//if the "time now" is greater than last ocurrence plus 2 seconds....
    lastOCC=millis();

    if (Serial.available()) {
      SerialBT.write(messageBL);
    }
    if (SerialBT.available()) {
      Serial.write(messageBL);
    }
  }
*/
}
