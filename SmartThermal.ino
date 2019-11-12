//This example code is in the Public Domain (or CC0 licensed, at your option.)
//By Evandro Copercini - 2018
//
//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial

#include "EEPROM.h"
#include "BluetoothSerial.h"
#include <WiFi.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

int LED_BUILTIN = 2;
const String BLUETOOTH_NAME = "ESP32test";

int addr = 0;
#define EEPROM_SIZE 4096

BluetoothSerial SerialBT;

const char* _ssid;
const char* _password;
bool isWiFiConnected = false;
bool isBluetoothEnabled = false;
bool hasWiFiInfoInEEPROM = false;
String bluetoothData = "";
const String wifiStringDelimiter = "|||||";
const String resetWifiInfo = "8EWI";
const String restartESP32 = "7RESP";
const bool wifiConnectedBluetoothOff = false;

void setup() {
  Serial.println("Booting up...");
  pinMode (LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("Starting Serial communication at baudrate 115200...");
  Serial.begin(115200);
  Serial.println("Serial communication at baudrate 115200 started");

  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("failed to initialise EEPROM"); 
    delay(10000);
    ESP.restart();
  }
  
  //for erasing EEPROM
  /*for (int i = 0; i < EEPROM_SIZE; i++)
  {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  */

  //for writing Wifi information directly
  /*EEPROM.writeChar(0, 0);
  EEPROM.writeString(1, "<ssid>");
  EEPROM.writeString(64, "<password>");
  EEPROM.commit();
  Serial.println("[DEBUG] ssid:" + EEPROM.readString(1));
  Serial.println("[DEBUG] password:" + EEPROM.readString(64));
  */
  
  Serial.println("[DEBUG] 4096 bytes read from EEPROM. Values are:");
  for (int i = 0; i < EEPROM_SIZE; i++)
  {
    Serial.print(byte(EEPROM.read(i))); Serial.print(" ");
  }
  Serial.println("");
  readWiFiInfoFromEEPROM(); 
  if (hasWiFiInfoInEEPROM) 
  {
      isWiFiConnected = connectToWiFi(EEPROM.readString(1).c_str(), EEPROM.readString(64).c_str());
  }
}

void loop() {
  if (!isWiFiConnected && isBluetoothEnabled)
  {
    listenBluetoothData();
    digitalWrite(LED_BUILTIN, HIGH);
    delay(300);    
    digitalWrite(LED_BUILTIN, LOW);
    delay(300);
  }
  
  if (isWiFiConnected)
  {
      Serial.println("WiFi is connected!");
      listenBluetoothData();
      your_function();
      delay(1000);
  }
}

void your_function()
{
  
}

//helper functions
bool connectToWiFi(const char* ssid, const char* password)
{
    bool wifiConnected = false;
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    // Wait for connection within 10 seconds
    char wifiConnectTry = 0;
    while (WiFi.status() != WL_CONNECTED && wifiConnectTry < 30) 
    {
        delay(1000);
        Serial.print(".");
        wifiConnectTry++;
        if (WiFi.status() == WL_CONNECTED)
        {
            break;  
        }
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("");
        Serial.print("Connected to ");
        Serial.println(ssid);
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());  
        wifiConnected = true;
        digitalWrite(LED_BUILTIN, HIGH);        
    } else
    {
      Serial.println("Error connecting WiFi");
      Serial.println("Error connecting WiFi with ssid: ");
      Serial.print(_ssid);
      Serial.print(", password: ");
      Serial.print(_password);
      //clearing ssid and password in EEPROM
      eraseWifiInfoInEEPROM();
      EEPROM.commit();     
    }

    if (!wifiConnectedBluetoothOff && !isBluetoothEnabled)
    {
        Serial.println("Starting bluetooth...");     
        isBluetoothEnabled = SerialBT.begin(BLUETOOTH_NAME); //Bluetooth device name            
        Serial.println("bluetooth started...");     
    }
    return wifiConnected;
}

void readWiFiInfoFromEEPROM()
{
    //EEPROM Data info: 
    /*1st byte: info status (0: no info, 1: full info)
     * 2nd byte to 64th byte: ssid info
     * 65th byte to 128th byte: password info
     */
    if (EEPROM.readChar(0) == 0)
    {
        Serial.println("No WiFi information");  
        Serial.println("Starting bluetooth...");
        isBluetoothEnabled = SerialBT.begin(BLUETOOTH_NAME); //Bluetooth device name
        Serial.println("The device started, now you can pair it with bluetooth for WiFi information!");                      
    }

    if (EEPROM.readChar(0) == 1)
    {
        Serial.println("Have WiFi information");   
        hasWiFiInfoInEEPROM = true;   
        //const char* tempssid = (EEPROM.readString(1).c_str());
        //_ssid = tempssid;
        //const char* temppassword = (EEPROM.readString(64).c_str());
        //_password = temppassword;
        Serial.println("ssid: ");
        Serial.print(EEPROM.readString(1).c_str());
        Serial.println("");
        Serial.println("password: ");
        Serial.print(EEPROM.readString(64).c_str());
        Serial.println("");
    }
}

void parseBTDataToWiFiInfo(String btData)
{
    /*
    WiFi information format: 9<ssid>|||||<password>
    */
    
    if (btData.length() > 0)
    {
        Serial.println("Has bluetooth data");
        //Serial.println(btData.charAt(0));
        //Serial.println(btData.indexOf("|||||"));
        if (btData.charAt(0) == '9' && btData.indexOf(wifiStringDelimiter) > 0)      
        {
            Serial.println("WiFi Data Available");
            String ssid = btData.substring(1, btData.indexOf(wifiStringDelimiter));
            String password = btData.substring(btData.indexOf(wifiStringDelimiter) + wifiStringDelimiter.length());
            Serial.println("ssid: " + ssid);
            Serial.println("password: " + password);
            //Save to EEPROM
            EEPROM.writeChar(0, 1);
            EEPROM.writeString(1, ssid);
            EEPROM.writeString(64, password);
            EEPROM.commit();
            ESP.restart();
        }

        if (btData.equals(resetWifiInfo))
        {
            eraseWifiInfoInEEPROM();
        }

        if (btData.equals(restartESP32))
        {
            ESP.restart();
        }
    }
}

void listenBluetoothData()
{
    bluetoothData = "";
    while (SerialBT.available()) {
        char tmpChar = SerialBT.read();
        bluetoothData += tmpChar;
        delay(20);
    }
    if (bluetoothData.length() > 0)
    {
        Serial.println("bluetoothData: " + bluetoothData);   
        parseBTDataToWiFiInfo(bluetoothData);
    }  
}

void eraseWifiInfoInEEPROM()
{
    Serial.println("Erase WiFi Info in EEPROM");
    for (int i = 0; i < 128; i++)
    {
        EEPROM.write(i, 0);
    }
    EEPROM.commit();  
}
