// MQTT_IoT_project created by Theocharis Theocharidis on 30.06.2021

// Define libraries
//-----------------------------
#include <WiFi.h>
#include <DHT.h>  
#include <PubSubClient.h>
//-----------------------------
//-----------------------------

// Define constants and global variables 
//-----------------------------
#define DHTPIN 23            /* The pin to which the sensor is connected in ESP32 */
#define DHTTYPE DHT22        /* The type of temperature and humidity sensor  */

// MQTT connection parameters
//------
const char* MQTT_USER = "user6"; // *** Replace with your username, e.g. user2, user3 etc. (leave it as it is for the test)
const char* MQTT_CLIENT = "user6";  // *** Must be unique, replace with your username, e.g. user2, user3 etc. (same as MQTT_USER)
const char* MQTT_TOPIC_CONTROL = "control";
const char* MQTT_TOPIC_DATA = "data";
const char* MQTT_TOPIC_SERVER_CRC = "dataCrc";
const char* MQTT_ADDRESS = "esp-32.zapto.org";
//------
//------

// WiFi connection parameters (WPA security protocol) 

//------
const char* WIFI_SSID     = "......."; // Your Wifi´s SSID here
const char* WIFI_PASSWORD = "....."; // Your Wifi Password here

//------
//------

char TopicControl[150], TopicData[150], dataCrc[150], s1[150], n2[150];
float x[5]={0.0,1.0,2.0,3.0,4.0};
float t,a,b;
float y[5]={0.0,0.0,0.0,0.0,0.0};
char *n3=NULL;

unsigned long measPreviousMillis = 0;

unsigned int number=0;
unsigned int sign=1;
unsigned int flagSample=0;
unsigned int flagCRC8=0;
unsigned int flagPubDataCrc=0;
unsigned int flagReconfig=0;
unsigned int flagFillin=0;
unsigned int delaY=0;
unsigned int d1d0=0;
unsigned int temp=0;
unsigned int humid=0;
 float humid_s=0.0;
 float temp_s=0.0;
unsigned int number1=0;
char n1;
unsigned int k=0;
unsigned int count=1;



//Functions
//-----------------------------

void callback(char*, byte*, unsigned int);
char CRC8(char);
void concat(unsigned int , unsigned int , unsigned int );
void Sample(void);
void fillin(float );
void leastSquares(void);

WiFiClient wifiClient;
PubSubClient client(MQTT_ADDRESS, 1883, callback, wifiClient);
DHT dht(DHTPIN, DHTTYPE);


//-----------------------------------------------------------------------------------------------------------------------------------//
/*       Sampling function for temperature and humidity values 
         --------------------------------------------- 
   Enabled for continuous sampling after receiving the startMeasurements command in topic user6 / control  */
//-----------------------------------------------------------------------------------------------------------------------------------//

void Sample(void)
{
   unsigned int i;
   unsigned int count;
   
   // Temperature and Humidity Sampling 

          delay(delaY*1000);
          

      //  Receiving Humidity values from the DHT22 sensor 
        float h = dht.readHumidity();
        float h_s=h/100;
        
        //  Receiving Temperature values from the DHT22 sensor 
         t = dht.readTemperature();
        t=t-2.5;
        flagFillin=1;
        
        // Check if any reads failed and exit early (to try again).
        if (isnan(h) || isnan(t)) 
        {
          Serial.println(F("Failed to read from DHT sensor!"));
          return;
        }
        Serial.print(F("Humidity: "));
        Serial.print(h);
        Serial.print(F("%  Temperature: "));
        Serial.print(t);
        Serial.println(F("°C "));
        if (t>temp_s)
        {
          Serial.println("Temperature High");
        }
        if (h>humid_s)
        {
          Serial.println("Humidity High");
        }

if (flagFillin==1)
  {
    fillin(t);
    flagFillin=0;
  } 
   
}

//-----------------------------------------------------------------------------------------------------------------------------------//
/*  Function that "fills" the table y (f (x)) with elements (temperature values) to calculate the linear interpolation  */
//-----------------------------------------------------------------------------------------------------------------------------------//
void fillin(float t)
{
  
  Serial.println(t);
  y[k]=t;
    if (count<=5){
      k++;
      count++;
    }
  if (count>5) 
  {// After the first 5 measurements are entered, each new measurement takes the place of the last temperature value in table y 
    y[4]=t;
    leastSquares();
  }  
}


//--------------------------------------------------------------------//
/*   Least squares algorithm function for linear equation  */
//--------------------------------------------------------------------//

void leastSquares(void){
        
      float sumx=0.0;
      float sumx2=0.0;
      float sumy=0.0;
      float sumyx=0.0;
      int i;
      int n=5;
      
      for(i=0;i<n;i++)
      {
        sumx=sumx+x[i];
        sumx2=sumx2+x[i]*x[i];
        sumy=sumy+y[i];
        sumyx=sumyx+y[i]*x[i];
      }
      
      //Calculation of parameters a and b for the linear equation y = ax + b 
      b=(sumx2*sumy-sumyx*sumx)/(n*sumx2-sumx*sumx);
      a=(n*sumyx-sumx*sumy)/(n*sumx2-sumx*sumx);
      
      Serial.println("The a is : ");
      Serial.println(a);
      Serial.println("The b is = ");
      Serial.println(b);
}


//--------------------------------------------------------------------------//
    /*       CRC8 coding algorithm function for generator number 6   */
//--------------------------------------------------------------------------//

char CRC8(char byteVal)
{
    char generator = 0x06;
    char crc = byteVal; /* init crc directly with input byte instead of 0, avoid useless 8 bitshifts until input byte is in crc register */

    for (int i = 0; i < 8; i++)
    {
        if ((crc & 0x80) != 0)
        { /* most significant bit set, shift crc register and perform XOR operation, taking not-saved 9th set bit into account */
            crc = (char)((crc << 1) ^ generator);        
        }
        else
        { /* most significant bit not set, go to next bit */
            crc <<= 1; 
        }
    }
    return crc;
} 

//--------------------------------------------------------------------------//
           /*  Concatenate Function */
//--------------------------------------------------------------------------//

void concat(unsigned int a, unsigned int b, unsigned int c)
{
 
    char s2[150];
    char s3[150];
     
    //Convert integers to strings and add a 0 where needed
    sprintf(s1, "%03d", a);
    sprintf(s2, "%03d", b);
    sprintf(s3, "%03d", c);
 
    // Concat alphanumerics alternately to concat into one
    strcat(s1, s2);
    strcat(s1, s3);
    Serial.println(s1);
}

//--------------------------------------------------------------------------//
                /*   Callback Function */
//--------------------------------------------------------------------------//
void callback(char* topic, byte* payload, unsigned int length) {
  char p[length + 1];
  memcpy(p, payload, length);
  p[length] = NULL;
  String message(p);
  String topic_str(topic);

  // Check received messages and topics
  //------
  
 /* Check if the word sendConfig was published in the topic Control */
  if (topic_str == TopicControl) {  
    if (message == "sendConfig")
      Serial.println("Configuration Message received.");
  }

  /* Check if the 6-digit number was received in the topic Data. If yes, CRC8 encoding is enabled  */
  
   if (topic_str == TopicData) {            
    if (message != ("Hello Server")){
     number = message.toInt();              // The 6-digit number 
    Serial.println("The 6-digit number is:");
    Serial.println(number);
    flagCRC8=1;
    } 
  }

  /* Check if the word startMeasurements was downloaded in topic Control. If yes, the sampling process is activated  */
  
  if (topic_str == TopicControl) {         
    if (message == "startMeasurements")
    {
      Serial.println("Sampling begins...");
      flagSample=1;                         // Activation of continuous sampling process 
    }
    else
    {
      flagSample=0;                        // Interruption of continuous sampling process 
    }
  }

 /* Check if the word updateConfig was downloaded in topic Control. If so, the reconfiguration process is activated  */
  if (topic_str == TopicControl) 
  {
    if (message == "updateConfig")
    {
      Serial.println(message);
      Serial.println("Reconfiguration begins...");
      sign=1;                              
      flagReconfig=1;
    }
  }
}


//-------------------------------------------------------------//
           /*  mqttReconnect Function */
//-------------------------------------------------------------//
void mqttReconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(MQTT_CLIENT, "samos", "samos21")) {
      Serial.println("MQTT connected");
      topicSubscribe();
    } 
    
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

//---------------------------------------------------------------------------------------------//
    /* Function "topicSubscribe", for subscribtion in the respective topics */
//---------------------------------------------------------------------------------------------//
void topicSubscribe() {
  if(client.connected()) {

////    Serial.println("Subscribe to MQTT topics: ");
// //   Serial.println(TopicControl);
//  //  Serial.println(TopicData);
//   // Serial.println(dataCrc);
    client.subscribe(TopicControl);
    client.subscribe(TopicData);
    client.subscribe(dataCrc);
    client.loop();
  }  
}

void setup() {
  // put your setup code here, to run once:


  // Connect to WiFi and establish serial connection
  //------
  Serial.begin(9600);
  delay(1000);
  Serial.println(F("DHT22 sensor!"));
  dht.begin();

  Serial.println();

  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Wait for WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi Connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  //------
  //------

  // Connect to MQTT broker and subscribe to topics
  //------
  client.setCallback(callback);

  // Define MQTT topic names
  sprintf(TopicControl, "%s/%s", MQTT_USER, MQTT_TOPIC_CONTROL);
  sprintf(TopicData, "%s/%s", MQTT_USER, MQTT_TOPIC_DATA);
  sprintf(dataCrc, "%s/%s", MQTT_USER, MQTT_TOPIC_SERVER_CRC);

  Serial.print("Wait for MQTT broker...");

  // Subscribe to topics and reconnect to MQTT server
  mqttReconnect();

}

void loop() {

// Field for computing Crc
//
if (flagCRC8==1){

//  / *    Calculation of the Delay        */
     
   delaY=number/10000;
   unsigned char d = (char) delaY;
   unsigned char d1=CRC8(d);
   d1d0=(unsigned int) d1;

// / *   Temperature´s Threshold Calculation       */  
   
   temp=(number-(delaY*10000))/100;             
   temp_s=(float) temp;
   unsigned char t = (char) temp; 
   unsigned char t1=CRC8(t);
   temp=(unsigned int) t1;
   
// / *   Humidity´s Threshold Calculation         */
    
  humid=number-((number/100)*100);              
  humid_s=(float) humid;
  unsigned char h = (char) humid; 
  unsigned char h1=CRC8(h);
  humid=(unsigned int) h1;

/// *    Combining the three numbers into one number with the concat function and converting it to an alphanumeric          */
   concat(d1d0, temp, humid);
   n3=&s1[0];
   flagCRC8=0;
   flagPubDataCrc=1;
}

//--------------------------------------------------------------------------------//
/*  Send the coded number to the topic dataCrc       */
      
if (flagPubDataCrc==1)
{
  client.publish(dataCrc,n3);
  flagPubDataCrc=0;
}

//--------------------------------------------------------------------------------//
/*  Start the continuous sampling function          */
if (flagSample==1)
{
  Sample();
}

//------------------------------------------------------------------
  
  unsigned long currentMillis = 0;
  currentMillis = millis();

  // Execute if block every 10000ms and send data
  //------  
  if ((currentMillis - measPreviousMillis) >=  10000) {
    if (client.connected()) { // if MQTT server coinnected

//Enable the process of sending the word sendConfig to the topic user6 / control 
        if (sign==1){ 
          client.publish(TopicControl, "sendConfig");
//Activate reconfiguration process 
          if (flagReconfig==1){ 
                count=1;
                  k=0;
                  a=0.0;
                  b=0.0;
                   for(int i=0;i<5;i++)
                    {
                      y[i]=0.0;
                      Serial.println("O pinakas y einai:");
                      Serial.println(y[i]);
                    }
                    flagReconfig=0;
                }
          sign=0;
        }
    }
    measPreviousMillis = currentMillis;
  }

  //------ 
  //------ 

  // Reconnect to WiFi if not connected
  //------
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Reconnecting to WiFi...\n");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  //------
  //------

  // Reconnect to MQTT broker if not connected
  //------
  if (!client.connected()) {
    mqttReconnect();
  }
  //------
  //------

  
  client.loop();
}
