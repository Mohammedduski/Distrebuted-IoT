// Libraries -----------------------------------------------------------------------------------------------------------
#define BLYNK_TEMPLATE_ID "TMPLbfzEksBN"
#define BLYNK_DEVICE_NAME "READ LOAD DUHOK"
#define BLYNK_AUTH_TOKEN "ECa3cIhaKdk0dG8VU2fFEcgXIBmA_X9k"

#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <HTTPClient.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Wire.h>
#include <XTronical_ST7735.h>
#include <SPI.h>
#include<EEPROM.h>
#include "BluetoothSerial.h"
#include <PZEM004Tv30.h>
#include "DHT.h"

// DHT pin configuration
#define DHTPIN 33 
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// PZEM rx/tx pin configuration
#if !defined(PZEM_RX_PIN) && !defined(PZEM_TX_PIN)
#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17
#endif

#if !defined(PZEM_SERIAL)
#define PZEM_SERIAL Serial2
#endif

PZEM004Tv30 pzem(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN);

#define BLYNK_FIRMWARE_VERSION        "0.1.0"

#define APP_DEBUG

// Push button pin --------------------------------------------------------------------------------------------------------
int pb=25;

// TFT screen signals -------------------------------------------------------------------------------------------------
#define TFT_CS     5
#define TFT_RST    4  
#define TFT_DC     2

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST); 
 
// hosting page conection and authentecation
const char* serverName = "http://emdpu.shaxirash.com/post_data_duhok.php";
String apiKeyValue = "tPmAT5Ab3j7F9";


// WiFi parameters ---------------------------------------------------------------------------------------------------
#define WLAN_SSID       "Iot_device"
#define WLAN_PASS       "mustafa1234"

// Serial2 pins of pzem  --------------------------------------------------------------------------------------------------
#define RXD2 16
#define TXD2 17

// Buetooth ----------------------------------------------------------------------------------------------------------------
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

//Initialize blynk authentication and timer to send data 
char auth[] = BLYNK_AUTH_TOKEN;
BlynkTimer timer;

// define  computer status pin in blynk 
WidgetLED room_status(V7);

// define Room pin in esp32 
#define room 15

// define node name in sending data  
int device_no = 4;

// define var for sending data when its empty
int engy = 0;
int pof = 0;
int flag1=-1;

//define date and counter when used in program  
int cnt;
long now;

// Information structure of main program 
struct information
{
 char bt_name[50];
 char act_password[50];
 char net_id[50];
 char net_password[50];
 int flag;
} info;

// Information structure of sensor data 
struct measure
{
 float voltage=-1.0;
 float current=-1.0;
 float power=-1.0;
 float frequency=-1.0;
 float temp=-1.0;
 float hum=-1.0;
 
}meas;

// Room_On --------------------------------------------------------------------------------------------------
void Room_On()
{
 digitalWrite(room,HIGH); 
 tft.fillRect(40, 90, 83, 10, ST7735_RED);
 tft.fillRect(40, 100, 83, 10, ST7735_RED);
 tft.setCursor(7, 90); tft.print("Room.: ON");
 tft.setCursor(7, 100); tft.print("FLAG: 1");
 info.flag=1;
 save_to_eeprom();
 room_status.on();
}

// Room_Off ------------------------------------------------------------------------------------------------
void Room_Off()
{
 digitalWrite(room,LOW);  
 tft.fillRect(40, 90, 83, 10, ST7735_RED);
 tft.fillRect(40, 100, 83, 10, ST7735_RED);
 tft.setCursor(7, 90); tft.print("Room.: OFF");
 tft.setCursor(7, 100); tft.print("FLAG: 0");
 info.flag=0;
 save_to_eeprom();
 room_status.off();
}

// Read informations from EEPROM 
void read_from_eeprom()
{
 EEPROM.begin(512);
 EEPROM.get(0,info);
 EEPROM.end();   
}

// Save information to EEPROM 
void save_to_eeprom()
{
 EEPROM.begin(512);
 EEPROM.put(0,info);
 EEPROM.end(); 
}

// Clear display Screen function -----------------
void clear_screen()
{
 tft.fillRect(0, 0, 127, 127, ST7735_BLUE);
 tft.fillRect(2, 2, 123, 123, ST7735_RED); 
}

// Draw main boarders on screen ------------------
void draw_main_boarders()
{
 clear_screen();

 tft.setTextSize(1);
 tft.setCursor(7, 7); tft.print("Vol:");
 tft.setCursor(7, 19); tft.print("Cur:"); 
 tft.setCursor(7, 31); tft.print("Pwr:"); 
 tft.setCursor(7, 43); tft.print("Frq:"); 
 tft.setCursor(7, 55); tft.print("Tmp:"); 
 tft.setCursor(7, 67); tft.print("Hum:"); 
}

// Read_measurement_value from sensor -----
void  read_measure_values()
{
 meas.voltage = pzem.voltage();
 meas.current = pzem.current();
 meas.power = pzem.power();
 meas.frequency = pzem.frequency();
 
 meas.temp=dht.readTemperature();
 meas.hum=dht.readHumidity();
}

// Display meas values of sensor in secreen ---------------
void display_meas()
{
 int i;
 int col[2]={ST7735_YELLOW,ST7735_WHITE}; 

 tft.fillRect(33, 6, 90, 10, ST7735_RED); 
 tft.fillRect(33, 18, 90,10, ST7735_RED); 
  
 tft.fillRect(33, 30, 90, 10, ST7735_RED); 
 tft.fillRect(33, 42, 90, 10, ST7735_RED); 

 tft.fillRect(33, 54, 90, 10, ST7735_RED); 
 tft.fillRect(33, 66, 90, 10, ST7735_RED); 

 for(i=0;i<=1;i++)
 {
  tft.setTextColor(col[i]);
  tft.setCursor(35, 7); tft.print(meas.voltage);tft.print(" V");
  tft.setCursor(35, 19); tft.print(meas.current);tft.print(" A");
  tft.setCursor(35, 31); tft.print(meas.power);tft.print(" W");
  tft.setCursor(35, 43); tft.print(meas.frequency);tft.print(" Hz");
  tft.setCursor(35, 55); tft.print(meas.temp);tft.print(" C");
  tft.setCursor(35, 67); tft.print(meas.hum);tft.print(" %");
 
  delay(100);
 }
}

//**************set function to send data to hosting server**********************************************
void publish_em()
{
 //define clint and http protocol connection
    WiFiClient client;
    HTTPClient http;
    
    // Domain name with URL path or IP address with path
    http.begin(client, serverName);
    
    // Specify content-type header
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    // Prepare your HTTP POST request data
    String httpRequestData = "api_key=" + apiKeyValue + "&voltage=" + float(meas.voltage)
                           + "&current=" + float(meas.current) + "&power=" + float(meas.power) + "&frequency=" + float(meas.frequency) + "&temp=" + float(meas.temp) + "&hum=" + float(meas.hum) + "&device_no=" + int(device_no) + "&flag=" + int(info.flag) + "&flag1=" + int(flag1) +  "&energy=" + int(engy) + "&pof=" + int(pof) + "";
 


    Serial.print("httpRequestData: ");
    Serial.println(httpRequestData);
    
        int httpResponseCode = http.POST(httpRequestData);
    
    if (httpResponseCode>0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    
    http.end();
    
}
//********************end sent************************************************


//Publish values to host server and blynk -----------------------
void publish_values()
{
 read_measure_values();
 display_meas();
 publish_em();
 
 Blynk.virtualWrite(V0, meas.voltage);
 Blynk.virtualWrite(V1, meas.current);
 Blynk.virtualWrite(V2, meas.power);
 Blynk.virtualWrite(V3, meas.frequency);
 Blynk.virtualWrite(V4, meas.temp);
 Blynk.virtualWrite(V5, meas.hum);
  
}

// Draw main boarders of bluttoth screen -------------------------------------------
void draw_setting_boarders() // Display boarders for Bluetooth setting
{
 clear_screen();
 
 tft.setTextSize(2);
 
 tft.drawRect(4, 4, 119, 20, ST7735_GREEN);
 tft.drawRect(5, 5, 117, 18, ST7735_GREEN); tft.setCursor(12, 7); tft.print("SYS. SET.");

 tft.setTextSize(1);
 
 tft.drawRect(4, 22, 119, 15, ST7735_GREEN);
 tft.drawRect(5, 23, 117, 13, ST7735_GREEN);tft.setCursor(8, 26); tft.print("Conn. to BT: "); tft.print(info.bt_name);


 tft.drawRect(4, 40, 119, 15, ST7735_YELLOW);
 tft.drawRect(5, 41, 117, 13, ST7735_YELLOW);tft.setCursor(8, 44); tft.print("1. BT NAME: ");

 tft.drawRect(4, 57, 119, 15, ST7735_YELLOW);
 tft.drawRect(5, 58, 117, 13, ST7735_YELLOW);tft.setCursor(8, 61); tft.print("2. SYS PASS: ");
 
 tft.drawRect(4, 74, 119, 15, ST7735_YELLOW);
 tft.drawRect(5, 75, 117, 13, ST7735_YELLOW);tft.setCursor(8, 78); tft.print("3. AP SSID: ");
 
 tft.drawRect(4, 91, 119, 15, ST7735_YELLOW);
 tft.drawRect(5, 92, 117, 13, ST7735_YELLOW);tft.setCursor(8, 95); tft.print("4. AP PASS: ");
 
 tft.drawRect(4, 108, 119, 15, ST7735_YELLOW);
 tft.drawRect(5, 109, 117, 13, ST7735_YELLOW);tft.setCursor(8, 112); tft.print("5. Display info.");
}

// Try to connection Gateway -----------------------------------------------------------------------------------------------
void try_Connection() // Display in the begining on black screen
{
 tft.setCursor(4,4);
 tft.print(F("Connecting to "));
 tft.print(info.net_id);

Serial.print("***");Serial.print(info.net_id);Serial.print("***");Serial.print(info.net_password);Serial.println("***");
//delay(10000);
 WiFi.begin(info.net_id, info.net_password);
 
 
 cnt=0;
 while (WiFi.status() != WL_CONNECTED) 
 {
  delay(500);
  tft.setCursor(45+5*cnt,16);
  tft.print(F("."));
  cnt++;
  if(cnt==8)
  {
   cnt=0;
   tft.fillRect(45,16,60,12,ST7735_BLUE);
  }
 }
 
 tft.setCursor(4,28);
 tft.print(F("WiFi connected"));
 tft.setCursor(4,40); 
 tft.print(F("IP: "));tft.print(WiFi.localIP());
 // Wait for 3 seconds
 delay(3000); 
}

// System configuration ------------------------------------------------------------
void sys_config()  // receiving String of coomand from Bluetooth
{
 char ch,temp[100],mess[20];
 char sys_pass[20],ap_pass[20],ap_ssid[20];
 int i,j,num;

 clear_screen();
 draw_setting_boarders();
 SerialBT.begin(info.bt_name);
 delay(1000);

 strcpy(mess,"*");
 do
 {
  if (SerialBT.available())
  {
   strcpy(temp, "");
   i = 0;
   while (SerialBT.available())
   {
    ch = SerialBT.read();
    temp[i] = ch;
    i++;
   }
   temp[i] = '\0';
   Serial.println();
   Serial.println(temp);

   if(temp[0]=='*')
   {
    i=1;
    while(temp[i]!='*')
    {
     sys_pass[i-1]=temp[i];
     i++;
    }
    sys_pass[i-1]='\0';

    Serial.print("***");Serial.print(sys_pass);Serial.print("***");Serial.print(info.act_password);Serial.println("***");

    if(strcmp(sys_pass,info.act_password)==0)
    {
     tft.setCursor(8, 44); tft.print("1. BT NAME: ");tft.print(info.bt_name);
     tft.setCursor(8, 61); tft.print("2. SYS PASS: ");tft.print(info.act_password);
     tft.setCursor(8, 78); tft.print("3. AP SSID: ");tft.print(info.net_id);
     tft.setCursor(8, 95); tft.print("4. AP PASS: ");tft.print(info.net_password);
     tft.setCursor(8, 112); tft.print("5. Display info.");
     
     i++;
     num=temp[i]-0x30;
     
     i+=2;
     j=0;
     while((temp[i]!='#') && (temp[i]!='\0'))
     {
      mess[j]=temp[i];
      i++;
      j++;
     }
     mess[j]='\0';

     Serial.print("***");Serial.print(num);Serial.println("***");
     Serial.print("***");Serial.print(mess);Serial.println("***");

     if(num==1) //change bt name
     {
      strcpy(info.bt_name,mess);
      tft.fillRect(4,47,119,8,ST7735_GREEN);
      tft.setTextSize(1); tft.setCursor(8, 47); tft.print("   Restarting...   ");

      
      tft.fillRect(6, 42, 115, 11, ST7735_BLUE);
      tft.setCursor(8, 44); tft.print("1. BT NAME: ");tft.print(mess);
      save_to_eeprom();
      read_from_eeprom();
      delay(2000);
      ESP.restart();
     }
     else if(num==2) // change sys pass
     {
      strcpy(info.act_password,mess);
      tft.fillRect(6, 59, 115, 11, ST7735_BLUE);
      tft.setCursor(8, 61); tft.print("2. SYS PASS: ");tft.print(info.act_password);
     }
     else if(num==3) // change net SSID
     {
      strcpy(info.net_id,mess);
      tft.fillRect(6, 76, 115, 11, ST7735_BLUE);
      tft.setCursor(8, 78); tft.print("3. NET SSID: ");tft.print(info.net_id);
     }
     else if(num==4) // change net PASS
     {
      tft.fillRect(6, 93, 115, 11, ST7735_BLUE);
      strcpy(info.net_password,mess); 
      tft.setCursor(8, 95); tft.print("4. NET PASS: ");tft.print(info.net_password);
     }
     else if(num==5)  // Only display informations
     {
      tft.fillRect(6, 110, 115, 11, ST7735_BLUE);
      tft.setCursor(8, 44); tft.print("1. BT NAME: ");tft.print(info.bt_name);
      tft.setCursor(8, 61); tft.print("2. SYS PASS: ");tft.print(info.act_password);
      tft.setCursor(8, 78); tft.print("3. NET SSID: ");tft.print(info.net_id);
      tft.setCursor(8, 95); tft.print("4. NET PASS: ");tft.print(info.net_password);
      tft.setCursor(8, 112); tft.print("5. Display info.");
     }
     else  //not from 1 to 5
     {
      tft.fillRect(6, 24, 115, 11,ST7735_BLUE);
      tft.setTextSize(1); tft.setCursor(8, 26); tft.print("Invalid selection");
      delay(2000);
      tft.fillRect(6, 24, 115, 11,ST7735_RED);
      tft.setCursor(8, 26); tft.print("Conn. to BT: "); tft.print(info.bt_name);
     }
     
     save_to_eeprom();
     read_from_eeprom();
     delay(3000);
     draw_setting_boarders();
     
    }
    else  // sys pass error
    {
     tft.fillRect(6, 24, 115, 11,ST7735_BLUE);
     tft.setTextSize(1); tft.setCursor(8, 26); tft.print("Invalid password!!!");
     delay(2000);
     tft.fillRect(6, 24, 115, 11,ST7735_RED);
     tft.setCursor(8, 26); tft.print("Conn. to BT: "); tft.print(info.bt_name);
    }                                                     
   }
   else //I want to restart
   {
    tft.fillRect(6, 24, 115, 11,ST7735_BLUE);
    tft.setTextSize(1); tft.setCursor(8, 26); tft.print("   Restarting...   ");
    delay(2000);
    ESP.restart();
   }
  }
  strcpy(mess,"*");
 }
 while(1);
}

// Initialize default informations and stracture for one time only for new devise 
void default_init()
{
 strcpy(info.bt_name,"GW_0");
 strcpy(info.act_password,"8421");
 strcpy(info.net_id,"Iot_device");
 strcpy(info.net_password,"mustafa1234");
 
 info.flag=0;
 save_to_eeprom();
}
//get status of split devise from blynk
BLYNK_WRITE(V6) //split ON OFF from Blynk app
{
 int w_pinValue1;
 w_pinValue1 = param.asInt();
 if (w_pinValue1 == 1)
  Room_On();
 else Room_Off();
}


// Setup function  -------------------------------------------------------
void setup() 
{

 // Serials begin for display in Serials moniter of arduino
 Serial.begin(9600);
 Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
 delay(250);

// starting read sensor of temp and hum
 dht.begin();
 
 // Push button configuration for bluetooth
 pinMode(pb,INPUT_PULLUP);
 
 // define split display on secreen 
 pinMode(room,OUTPUT);
 digitalWrite(room,LOW);

 // cal this function one time only for new device programing 
 
//default_init();

// read information from eeprom 
read_from_eeprom();

Serial.println();Serial.println();
Serial.print("BLUETOOTH NAME   : ");Serial.println(info.bt_name);
Serial.print("SYSTEM PASSWORED : ");Serial.println(info.act_password);
Serial.print("NET SSID         : ");Serial.println(info.net_id);
Serial.print("NET PASSWORD     : ");Serial.println(info.net_password);
Serial.print("FLAG             : ");Serial.println(info.flag);

// TFT or screen initialization
 tft.init();
 delay(250);
 tft.setRotation(0);
 tft.fillScreen(ST7735_BLACK);
 tft.setTextSize(1);
 
if(digitalRead(pb)==0) sys_config();

 tft.fillScreen(ST7735_BLACK);
 tft.setTextSize(1);
 
 try_Connection(); 
 
 draw_main_boarders();

// conect to internet and blynk to send data every 30 second
 Blynk.begin(auth, info.net_id, info.net_password); 
 timer.setInterval(30000L, publish_values);

//check split status
 if(info.flag==1) Room_On();
 else Room_Off();
 

}


//blynk conected and synchronization 
BLYNK_CONNECTED()
{
 Blynk.syncAll();  
}


// Main code ------------------------------------
void loop() 
{
 Blynk.run();
 timer.run();
}
