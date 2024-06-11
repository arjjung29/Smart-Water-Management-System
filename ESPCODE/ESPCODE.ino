/*
PROJECT TITLE:

-----------------------SMART WATER MANAGEMENT----------------------------------------------- 

-----------------------CORE CONTRIBUTORS----------------------------------------------------
- |Arjjun G            |B.Tech (Mechtronics Engineering) | Puducherry Technological University|
- |Arjun Christopher S |B.Tech (Information Technology)  | Puducherry Technological University|
- |Aaryan M            |B.Tech (Information Technology)  | Puducherry Technological University|
--------------------------------------------------------------------------------------------
Required Library & their Versions: Firebase ESP8266 Client by Mobizt --version:4.0.0
                                   ArduinoJson by Benoit             --version:5.11.1
                                   Adafruit GFX by Adafruit          --version:1.11.9
                                   Adafruit SSD1306 by Adafruit      --version:2.5.10

Note : "The Above mentioned libraries are critical so install them in the above mentioned version for proper working"
*/

//Header Files inclusion

#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#include <FirebaseESP32.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#elif defined(PICO_RP2040)
#include <WiFi.h>
#include <FirebaseESP8266.h>
#endif

#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

//Defenitions 

#define SCREEN_WIDTH 128    
#define SCREEN_HEIGHT 64    
#define OLED_RESET -1       
#define LED_BUILTIN 16
#define SENSOR  12
#define API_KEY "AIzaSyDSh0I9WcifMky1_XOLiIRY0HcVGC1nfWg"
#define DATABASE_URL "https://smart-water-management-b0aab-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define USER_EMAIL "arjjungobalakrishnan@gmail.com"
#define USER_PASSWORD "29012005arjjun"

//Necessary Variables

const char *ssid = "ARJJUN-HALL";     
const char *pass = "29012005@Lucky&John!";
long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
boolean ledState = LOW;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1Sec = 0;
float flowRate;
unsigned long flowMilliLitres;
unsigned int totalMilliLitres;
float flowLitres;
float totalLitres;
int sensor = 14;
int val = 0;
const long utcOffsetInSeconds = 19800;
int currentMonth;
int currentDay;
float previousMonthData;
int adult ;
int children ;
int waterLimit;
int remaining;
bool agentloginstatus;
int counter;

//Creating Objects

WiFiClient client;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds, 60000);

//ISR (Interrupt Subroutine Function)

void IRAM_ATTR pulseCounter()
{
  pulseCount++;
}

//Function to Connect to Wi-Fi

void setupWiFi() {
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi connected");
}

//Initializing Variables and other critical object 

void setup()
{
  //Setup the Serial Monitor

  Serial.begin(115200);
  setupWiFi();
  timeClient.begin();
  timeClient.update();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //initialize with the I2C addr 0x3C (128x64)
  display.clearDisplay();
  delay(10);
  
  //Initializing Firebase and authenticate

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Firebase.setDoubleDigits(5);
 
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SENSOR, INPUT_PULLUP);
 
  //Parameters Calculated
  
  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  previousMillis = 0;
  counter = 0;

  //Attaching the interrupt
 
  attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING);

  //Setting up Critical variables in Firebase

  Firebase.RTDB.setInt(&fbdo, "User1/adults", 0);
  Firebase.RTDB.setInt(&fbdo, "User1/children", 0);
  Firebase.RTDB.setFloat(&fbdo, "User1/Total-Litres", 0);
  Firebase.RTDB.setInt(&fbdo, "User1/Limit", 0);
  Firebase.RTDB.setInt(&fbdo, "User1/Remaining", 0);
  
  //Setting up Variables in Firebase for Previous month usage
  
  Firebase.RTDB.setFloat(&fbdo, "User1/History/Total-Litres-January", 0);
  Firebase.RTDB.setFloat(&fbdo, "User1/History/Total-Litres-February", 0);
  Firebase.RTDB.setFloat(&fbdo, "User1/History/Total-Litres-March", 0);
  Firebase.RTDB.setFloat(&fbdo, "User1/History/Total-Litres-April", 0);
  Firebase.RTDB.setFloat(&fbdo, "User1/History/Total-Litres-May", 0);
  Firebase.RTDB.setFloat(&fbdo, "User1/History/Total-Litres-June", 0);
  Firebase.RTDB.setFloat(&fbdo, "User1/History/Total-Litres-July", 0);
  Firebase.RTDB.setFloat(&fbdo, "User1/History/Total-Litres-August", 0);
  Firebase.RTDB.setFloat(&fbdo, "User1/History/Total-Litres-September", 0);
  Firebase.RTDB.setFloat(&fbdo, "User1/History/Total-Litres-October", 0);
  Firebase.RTDB.setFloat(&fbdo, "User1/History/Total-Litres-November", 0);
  Firebase.RTDB.setFloat(&fbdo, "User1/History/Total-Litres-December", 0);


}
 
void loop()
{ 
  timeClient.update();
  printCurrentTime();
  
  //Fetch number of adults from Firebase

  Firebase.RTDB.getInt(&fbdo, "User1/adults");
    if (fbdo.dataType() == "int") {
        adult = fbdo.intData();
        Serial.println(adult);
    }
    else {
      Serial.println(fbdo.errorReason());
    }
  delay(1000);

  //Fetch number of children from Firebase

  Firebase.RTDB.getInt(&fbdo, "User1/children");
    if (fbdo.dataType() == "int") {
        children = fbdo.intData();
        Serial.println(children);
    }
    else {
      Serial.println(fbdo.errorReason());
    }
  delay(1000);

  //Calculate Water Limit for the User from fetched data

  waterLimit = (((adult * 110) + (children *70))*30);

  //Print the Waterlimit
  
  Serial.print("Water Limit: ");
  Serial.print(waterLimit);
  Serial.println();

  //Calculate the Remaining water limit for the month

  remaining = (waterLimit - int(totalLitres));

  //Print the Remaing water limit

  Serial.print("Remaining Water: ");
  Serial.print(remaining);
  Serial.println();

  //Firebase.RTDB.setFloat(&fbdo, "User1/Total-Litres", totalLitres); 
  
  //If current day if 1 (First Day of the Months) store the calculated data and update it accordingly

  if ((currentDay == 1) && (counter==0)) {
        // Store current month's data to previousMonthData
        previousMonthData = totalLitres;

        // Reset the sensorData
        pulseCount = 0;
        flowRate = 0.0;
        flowMilliLitres = 0;
        totalMilliLitres = 0;
        previousMillis = 0;
        
        //Set Counter 1 (Used to prevent resetting all day 1)

        counter = 1;

        //Using Switch Case statement to store the history accordingly (using current month's number)

        switch(currentMonth){
          case 1:
             Firebase.RTDB.setFloat(&fbdo, "User1/History/Total-Litres-December", previousMonthData);
             break;
          case 2:
             Firebase.RTDB.setFloat(&fbdo, "User1/History/Total-Litres-Januray", previousMonthData);
             break;
          case 3:
             Firebase.RTDB.setFloat(&fbdo, "User1/History/Total-Litres-February", previousMonthData);
             break;
          case 4:
             Firebase.RTDB.setFloat(&fbdo, "User1/History/Total-Litres-March", previousMonthData);
             break;
          case 5:
             Firebase.RTDB.setFloat(&fbdo, "User1/History/Total-Litres-April", previousMonthData);
             break;
          case 6:
             Firebase.RTDB.setFloat(&fbdo, "User1/History/Total-Litres-May", previousMonthData);
             break;
          case 7:
             Firebase.RTDB.setFloat(&fbdo, "User1/History/Total-Litres-June", previousMonthData);
             break;
          case 8:
             Firebase.RTDB.setFloat(&fbdo, "User1/History/Total-Litres-July", previousMonthData);
             break;
          case 9:
             Firebase.RTDB.setFloat(&fbdo, "User1/History/Total-Litres-August", previousMonthData);
             break;
          case 10:
             Firebase.RTDB.setFloat(&fbdo, "User1/History/Total-Litres-September", previousMonthData);
             break;
          case 11:
             Firebase.RTDB.setFloat(&fbdo, "User1/History/Total-Litres-October", previousMonthData);
             break;
          case 12:
             Firebase.RTDB.setFloat(&fbdo, "User1/History/Total-Litres-November", previousMonthData);
             break;
          default:
             Serial.println("Error in Switch Case");
        }

        // Optional: Add delay to prevent multiple resets in a day
        delay(60000);
  }
  
  //Reset Counter back to Zero on Day 2 on the current month 

  if(currentDay==2){

    counter = 0;

  }
  
  //Look out for Tampering of Device using PIR (Passive Infrared) Sensor

  val = digitalRead(sensor);   
  delay(100);  
  if (val == HIGH){
    Serial.println("MOTION DETECTED !");
    //delay(1000);
  }

  //Below Logic is used to calculate the flow rate and Volume of water consumed

  currentMillis = millis();
  if (currentMillis - previousMillis > interval) 
  {
    
    pulse1Sec = pulseCount;
    pulseCount = 0;
    flowRate = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
    previousMillis = millis();
    flowMilliLitres = (flowRate / 60) * 1000;
    flowLitres = (flowRate / 60);
    totalMilliLitres += flowMilliLitres;
    totalLitres += flowLitres;
    
    //Display Flow Rate in Serial Monitor
    
    Serial.print("Flow rate: ");
    Serial.print(float(flowRate));  
    Serial.print("L/min");
    Serial.print("\t");       
    display.clearDisplay();
    display.setCursor(10,0);  
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.print("Water Flow Meter");

    //Display FLow Rate in OLED Display
    
    display.setCursor(0,20);  
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.print("R:");
    display.print(float(flowRate));
    display.setCursor(100,28);  
    display.setTextSize(1);
    display.print("L/M");

    //Display total consumption (in litres) in Serial Monitor

    Serial.print("Output Liquid Quantity: ");
    Serial.print(totalMilliLitres);
    Serial.print("mL / ");
    Serial.print(totalLitres);
    Serial.println("L");

    //Display total consumption (in litres) OLED Display
 
    display.setCursor(0,45);  
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.print("V:");
    display.print(totalLitres);
    display.setCursor(100,53); 
    display.setTextSize(1);
    display.print("L");
    display.display();
    Firebase.RTDB.setFloat(&fbdo, "User1/Total-Litres", totalLitres); 
  }

  
}


//Get current month and day's number 

void printCurrentTime() {
    
    currentMonth = getCurrentMonth();
    currentDay = timeClient.getDay();
}

//Code to get Current month from NTP

int getCurrentMonth() {
    time_t rawTime = timeClient.getEpochTime();
    struct tm *timeInfo = localtime(&rawTime);
    return timeInfo->tm_mon + 1; 
}

