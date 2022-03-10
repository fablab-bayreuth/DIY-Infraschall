#include <EEPROM.h>
#include <WString.h>
#define SAMPLING_BME 8.0

#define SAMPLING_F15 25.92405
#define SAMPLING_F14 51.8481
#define SAMPLING_F13 91.0222
#define SAMPLING_F12 151.7037

#define SAMPLING2_F15 25.0
#define SAMPLING2_F14 50.0
#define SAMPLING2_F13 90.9091
#define SAMPLING2_F12 142.8571

String mes; //global String!
uint8_t sd_buffer[1024];
char char_buffer[50]; //global Buffer for chars


struct MyCONFIG
{
  char ssid[20];
  char password[20];
  long baud;
  long max_runtime;
  float bat_factor;
  float bat_full;
  float bat_empty;
  float live_freq;
  long live_interval;
  uint8_t atmega;
} cfg;

struct DeviceStatus
{
  bool send_error;
  char error[50];
  bool send_msg;
  char msg[50];
  uint8_t logging; //0 off, 1: waiting for \n; 2: logging: 3: waiting for \n 
  uint8_t live; //0 off, 1: live
  char logging_file[13];
  unsigned long runtime; //in ms - we convert on message
  unsigned long logging_started;
  unsigned long last_logging_update;
  unsigned long last_logging_flush;
  unsigned long last_bat;
} device;

void error(const String& s){
  s.toCharArray(device.error, 50);
  device.send_error=true;
}
void message(const String& s){
  s.toCharArray(device.msg, 50);
  device.send_msg=true;
}



void eraseConfig() {
  // Reset EEPROM bytes to '0' for the length of the data structure
  strcpy(cfg.ssid,"DIY-Infraschall");  
  strcpy(cfg.password,"fablab24");  
  cfg.baud=38400;
  cfg.max_runtime=7*24*3600;
  cfg.bat_factor=(100+6.8)/6.8/1024;
  cfg.bat_full=4.2;
  cfg.bat_empty=3.7; 
  cfg.live_freq=SAMPLING_F14;
  cfg.live_interval=10;
  cfg.atmega=0; //RTC-Board
  EEPROM.put( 0, cfg );
  delay(200);
  EEPROM.commit();                      // Only needed for ESP8266 to get data written
}

void saveConfig() {
  EEPROM.put( 0, cfg );
  delay(200);
  EEPROM.commit();                      // Only needed for ESP8266 to get data written
}

void loadConfig() {
  // Loads configuration from EEPROM into RAM
  EEPROM.get( 0, cfg );
}


void sendConfig();

char getResolution(void){
  switch((int) cfg.live_freq){
  case (int) SAMPLING_BME:
      return '6';
      break;
  case (int) SAMPLING_F15:
      return '5';
      break;
  case (int) SAMPLING_F14:
  case (int) SAMPLING2_F14:
      return '4';
      break;
  case (int) SAMPLING_F13:
  case (int) SAMPLING2_F13:
      return '3';
      break;
  case (int) SAMPLING_F12:
  case (int) SAMPLING2_F12:
      return '2';
      break;
  default:
	  if(cfg.atmega) cfg.live_freq=SAMPLING2_F14;
	  else cfg.live_freq=SAMPLING_F14;
      saveConfig();
      sendConfig();
      return '4';
      break;
  }
}


void setResolution(void){
  while(Serial.available()) Serial.read();
	Serial.print(getResolution());
}
