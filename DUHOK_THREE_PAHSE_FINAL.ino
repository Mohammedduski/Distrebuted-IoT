// Libraries -----------------------------------------------------------------------------------------------------------
#define BLYNK_TEMPLATE_ID "TMPLeRpUHFia"
#define BLYNK_DEVICE_NAME "DUHOKBULDINGB"
#define BLYNK_AUTH_TOKEN "ngi0uwz6vVYH7iO0qvI3zMx9ifIVf4Nb"

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

// PZEM rx/tx pin configuration
#if !defined(PZEM_RX_PIN) && !defined(PZEM_TX_PIN)
#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17
#endif

#if !defined(PZEM_SERIAL)
#define PZEM_SERIAL Serial2
#endif

#define NUM_PZEMS 3

PZEM004Tv30 pzems[NUM_PZEMS];

// Push button pin ---------------------------------------------------------
int pb=25;

// TFT screen signals ----------------------------------------------------------
#define TFT_CS     5
#define TFT_RST    4  
#define TFT_DC     2

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST); 
 
// hosting page conection and authentecation
const char* serverName = "http://emdpu.shaxirash.com/post_data_duhok.php";
String apiKeyValue = "tPmAT5Ab3j7F9";

// Serial2 pins --------------------------------------------------------------------------------------------------
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

int ind=3;
float av_voltage,av_current,av_power,av_energy,av_pf,av_frequency;

// define node name in sending data  
int device_no = 5;

// define var for sending data when its empty
int flag1=-1;
int flag=-1;
int temp=0;
int hum=0;

// Information structure ---------------------------------------------------------------------------------------------------
struct information
{
 char bt_name[50];
 char act_password[50];
 char net_ssid[50];
 char net_password[50];
 
} info;

struct measure
{
 float voltage=-1.0;
 float current=-1.0;
 float power=-1.0;
 float energy=-1.0;
 float pf=-1.0;
 float frequency=-1.0;
 
}meas[3];

// Clear Screen -----------------------------------------------------------------------------------------------------------------
void clear_screen()
{
 tft.fillRect(0, 0, 127, 127, ST7735_BLUE);
 tft.fillRect(2, 2, 123, 123, ST7735_RED); 
}

// Display meas values ---------------------------------------------------------------------------------------------------------------------
void display_meas()
{
 int i;
 int col[2]={ST7735_YELLOW,ST7735_WHITE}; 

 tft.fillRect(33, 49, 90, 10, ST7735_RED); 
 tft.fillRect(33, 61, 90,10, ST7735_RED); 
  
 tft.fillRect(33, 73, 90, 10, ST7735_RED); 
 tft.fillRect(33, 85, 90, 10, ST7735_RED); 

 tft.fillRect(33, 97, 90, 10, ST7735_RED); 
 tft.fillRect(33, 109, 90, 10, ST7735_RED); 

 if(digitalRead(pb)==0) ind++;
  
 if(ind>3) ind=0;
 
 if(ind==0)
 {
  tft.fillCircle(21,18,6,ST7735_YELLOW);
  tft.fillCircle(64,18,5,ST7735_RED);
  tft.fillCircle(106,18,5,ST7735_RED); 
 }
 else if(ind==1)
 {
  tft.fillCircle(21,18,5,ST7735_RED);
  tft.fillCircle(64,18,6,ST7735_YELLOW);
  tft.fillCircle(106,18,5,ST7735_RED);
 }
 else if(ind==2)
 {
  tft.fillCircle(21,18,5,ST7735_RED);
  tft.fillCircle(64,18,5,ST7735_RED);
  tft.fillCircle(106,18,6,ST7735_YELLOW);
 }
 else if(ind==3)
 {
  tft.fillCircle(21,18,6,ST7735_YELLOW);
  tft.fillCircle(64,18,6,ST7735_YELLOW);
  tft.fillCircle(106,18,6,ST7735_YELLOW);
 }
  
 if(ind<3)
 {
  for(i=0;i<=1;i++)
  {
   tft.setTextColor(col[i]);
   tft.setCursor(35, 50); tft.print(meas[ind].voltage);tft.print(" V");
   tft.setCursor(35, 62); tft.print(meas[ind].current);tft.print(" A");
   tft.setCursor(35, 74); tft.print(meas[ind].power);tft.print(" W");
   tft.setCursor(35, 86); tft.print(meas[ind].energy);tft.print(" KWH");
   tft.setCursor(35, 98); tft.print(meas[ind].pf);tft.print(" KVA");
   tft.setCursor(35, 110); tft.print(meas[ind].frequency);tft.print(" Hz");
  }
  delay(100);
 }
 else
 {
  for(i=0;i<=1;i++)
  {
   
   tft.setTextColor(col[i]);
   tft.setCursor(35, 50); tft.print(av_voltage);tft.print(" V");
   tft.setCursor(35, 62); tft.print(av_current);tft.print(" A");
   tft.setCursor(35, 74); tft.print(av_power);tft.print(" W");
   tft.setCursor(35, 86); tft.print(av_energy);tft.print(" KWH");
   tft.setCursor(35, 98); tft.print(av_pf);tft.print(" KVA");
   tft.setCursor(35, 110); tft.print(av_frequency);tft.print(" Hz");
  }
  delay(2000);
 }
}

// Read_measurement_value ------------------------------------------------------------------------------------
void  read_measur_values()
{
 int i;
 
 for(int i = 0; i < NUM_PZEMS; i++)
 {
  meas[i].voltage = pzems[i].voltage();
  meas[i].current = pzems[i].current();
  meas[i].power = pzems[i].power();
  meas[i].energy = pzems[i].energy();
  meas[i].pf = pzems[i].pf();
  meas[i].frequency = pzems[i].frequency();
 }

 av_voltage=0.0;
 av_current=0.0;
 av_power=0.0;
 av_energy=0.0;
 av_pf=0.0;
 av_frequency=0.0; 
 
 for(i=0;i<3;i++)
 {
  av_voltage=av_voltage+meas[i].voltage;
  av_current=av_current+meas[i].current;
  av_power=av_power+meas[i].power;
  av_energy=av_energy+meas[i].energy;
  av_pf=av_pf+meas[i].voltage;
  av_frequency=av_frequency+meas[i].frequency;
 }
  
 av_voltage=av_voltage/3.0;
 av_pf=av_power/(av_voltage*av_current);
 av_frequency=av_frequency/3.0;
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
    String httpRequestData = "api_key=" + apiKeyValue + "&voltage=" + float(av_voltage)
                           + "&current=" + float(av_current) + "&power=" + float(av_power) + "&frequency=" + float(av_frequency) + "&temp=" + int(temp) + "&hum=" + int(hum) + "&device_no=" + int(device_no) + "&flag=" + int(flag) + "&flag1=" + int(flag1) +  "&energy=" + float(av_energy) + "&pof=" + float(av_pf) + "";
 


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

// Publish values -----------------------------------------------------------------------------------------------------
void publish_values()
{
 publish_em();
 
 Blynk.virtualWrite(V0, av_voltage);
 Blynk.virtualWrite(V1, av_current);
 Blynk.virtualWrite(V2, av_power);
 Blynk.virtualWrite(V3, av_energy);
 Blynk.virtualWrite(V4, av_frequency);
 Blynk.virtualWrite(V5, av_pf);
 
}

// Draw main boarders -----------------------------------------------------------------------------------------------------------------------
void draw_main_boarders()
{
 clear_screen();
 tft.drawRect(4,4,119,40,ST7735_YELLOW);
 tft.drawRect(5,5,117,38,ST7735_YELLOW);

 tft.drawCircle(21,18,6,ST7735_YELLOW);
 tft.drawCircle(64,18,6,ST7735_YELLOW);
 tft.drawCircle(106,18,6,ST7735_YELLOW);
 tft.setCursor(14, 30);tft.print(" R      S      T");
 
 tft.setTextSize(1);
 tft.setTextColor(ST7735_GREEN);
 tft.setCursor(7, 50); tft.print("Vol:");
 tft.setCursor(7, 62); tft.print("Cur:"); 
 tft.setCursor(7, 74); tft.print("Pwr:"); 
 tft.setCursor(7, 86); tft.print("Enr:"); 
 tft.setCursor(7, 98); tft.print("PoF:"); 
 tft.setCursor(7, 110); tft.print("Frq:"); 
}

// Read informations from EEPROM ----------------------------------------------------------------------------------------------------
void read_from_eeprom()
{
 EEPROM.begin(512);
 EEPROM.get(0,info);
 EEPROM.end();   
}

// Save to EEPROM -------------------------------------------------------------------------------------------------------------------
void save_to_eeprom()
{
 EEPROM.begin(512);
 EEPROM.put(0,info);
 EEPROM.end(); 
}

// Draw main boarders -----------------------------------------------------------------------------------------------------
void draw_setting_boarders()
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

// Try to connection Gateway -----------------------------------------------------------------------------------------------------
void try_Connection()
{
 int cnt;
 
 tft.setCursor(4,4);
 tft.print(F("Connecting to "));
 tft.print(info.net_ssid);

Serial.print("***");Serial.print(info.net_ssid);Serial.print("***");Serial.print(info.net_password);Serial.println("***");
//delay(10000);
 WiFi.begin(info.net_ssid, info.net_password);
 //WiFi.begin("moon", "mustafa1234");
 
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

// System configuration -----------------------------------------------------------------------------------------------------------
void sys_config()
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
     tft.setCursor(8, 78); tft.print("3. AP SSID: ");tft.print(info.net_ssid);
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

     if(num==1) 
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
     else if(num==2) 
     {
      strcpy(info.act_password,mess);
      tft.fillRect(6, 59, 115, 11, ST7735_BLUE);
      tft.setCursor(8, 61); tft.print("2. SYS PASS: ");tft.print(info.act_password);
     }
     else if(num==3) 
     {
      strcpy(info.net_ssid,mess);
      tft.fillRect(6, 76, 115, 11, ST7735_BLUE);
      tft.setCursor(8, 78); tft.print("3. NET SSID: ");tft.print(info.net_ssid);
     }
     else if(num==4) 
     {
      tft.fillRect(6, 93, 115, 11, ST7735_BLUE);
      strcpy(info.net_password,mess); 
      tft.setCursor(8, 95); tft.print("4. NET PASS: ");tft.print(info.net_password);
     }
     else if(num==5)
     {
      tft.fillRect(6, 110, 115, 11, ST7735_BLUE);
      tft.setCursor(8, 44); tft.print("1. BT NAME: ");tft.print(info.bt_name);
      tft.setCursor(8, 61); tft.print("2. SYS PASS: ");tft.print(info.act_password);
      tft.setCursor(8, 78); tft.print("3. NET SSID: ");tft.print(info.net_ssid);
      tft.setCursor(8, 95); tft.print("4. NET PASS: ");tft.print(info.net_password);
      tft.setCursor(8, 112); tft.print("5. Display info.");
     }
     else
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
    else 
    {
     tft.fillRect(6, 24, 115, 11,ST7735_BLUE);
     tft.setTextSize(1); tft.setCursor(8, 26); tft.print("Invalid password!!!");
     delay(2000);
     tft.fillRect(6, 24, 115, 11,ST7735_RED);
     tft.setCursor(8, 26); tft.print("Conn. to BT: "); tft.print(info.bt_name);
    }                                                     
   }
   else 
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

// Initialize default informations --------------------------------------------------------------------------------------------------
void default_init()
{
 strcpy(info.bt_name,"GW_0");
 strcpy(info.act_password,"8421");
 strcpy(info.net_ssid,"Iot_device");
 strcpy(info.net_password,"mustafa1234");

 save_to_eeprom();
}

void configure_pzem()
{
 int i;
 for(i = 0; i < NUM_PZEMS; i++)
 {
  pzems[i] = PZEM004Tv30(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN, i+0x10);
 } 
}

// Setup function  -------------------------------------------------------------------------------------------------
void setup() 
{
 // Serials begin
 Serial.begin(9600);
 Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
 delay(250);

 // Push button configuration for change phase and bluetooth
 pinMode(pb,INPUT_PULLUP);
 
 //default_init();
 
 read_from_eeprom();

 Serial.println();Serial.println();
 Serial.print("BLUETOOTH NAME   : ");Serial.println(info.bt_name);
 Serial.print("SYSTEM PASSWORED : ");Serial.println(info.act_password);
 Serial.print("NET SSID         : ");Serial.println(info.net_ssid);
 Serial.print("NET PASSWORD     : ");Serial.println(info.net_password);
 

 // TFT initialization
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

 configure_pzem();

 // conect to internet and blynk to send data every 30 second
 Blynk.begin(auth, info.net_ssid, info.net_password); 
 timer.setInterval(30000L, publish_values);

}

//blynk conected and synchronization 
BLYNK_CONNECTED()
{
 Blynk.syncAll();  
}


// Main code --------------------------------------------------------------------------------------------------
void loop() 
{
 Blynk.run();
 timer.run();
 
 read_measur_values();
 display_meas();
 delay(1000);
 
}
