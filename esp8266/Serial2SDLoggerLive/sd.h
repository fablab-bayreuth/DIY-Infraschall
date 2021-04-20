#include <SD.h>
File file;
File root;

const uint8_t chipSelect = 15;

#include <BayEOSBuffer.h>
RTC_Millis myRTC;

/*
  void dateTimeCB(uint16_t* date, uint16_t* time) {
  // Get time from clock
  DateTime d=myRTC.now();
  // return date using FAT_DATE macro to format fields
   date = FAT_DATE(d.year(), d.month(), d.day());

  // return time using FAT_TIME macro to format fields
   time = FAT_TIME(d.hour(), d.minute(), d.second());
  }
*/

void sendLogging(void); //Declaration from socket.h

bool startSD(void) {
  if (device.logging) {
    sendLogging();
    error(String(F("Logging in progress. Cannot reopen SD-Card.")));
    return 1;
  }
  if (! SD.begin(chipSelect)) {
    sendLogging();
    error(String(F("Open SD Card failed")));
    return 1;
  }
  // File::dateTimeCallback(dateTimeCB);
  return 0;
}


void startLogging(char* f) {
  if (startSD()) return;
  Serial.begin(cfg.baud);
  Serial.flush();
  setResolution();
  strncpy(device.logging_file, f, 12);
  uint16_t msec;
  DateTime d = myRTC.get(&msec);
  mes = "## ";
  mes += d.year();
  mes += "-";
  if (d.month() < 10) mes += '0';
  mes += d.month();
  mes += "-";
  if (d.day() < 10) mes += '0';
  mes += d.day();
  mes += " ";
  if (d.hour() < 10) mes += '0';
  mes += d.hour();
  mes += ":";
  if (d.minute() < 10) mes += '0';
  mes += d.minute();
  mes += ":";
  if (d.second() < 10) mes += '0';
  mes += d.second();
  mes += '.';
  mes += msec;
  mes += " GMT ## ";
  mes += String(cfg.live_freq, 5);
  mes.toCharArray(char_buffer, 50);
  file = SD.open(device.logging_file, FILE_WRITE);
  if (! file) {
    mes = F("Failed to open ");
    mes += f;
    error(mes);
    return;
  }
  file.println(char_buffer);
  device.logging = 1;
  device.logging_started = myRTC.now().get();
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, HIGH);
  sendLogging();
}

void sendSDContent(void); //prototype from socket.h

void stopLogging(void) {
  device.logging = 3;

}

unsigned long last_d0_high;



void handleSerial(void) {
  unsigned long data_length;
  switch (device.logging) {
    case 2:
      while (data_length = Serial.available()) {
        if (data_length > 512) data_length = 512;
        Serial.readBytes(sd_buffer, data_length);
        file.write(sd_buffer, data_length);
      }
      break;
    case 1:
      while (Serial.available()) {
        if (Serial.read() == '\n') {
          device.logging = 2;
          return;
        }
      }
      break;
    case 3:
      while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n') {
          digitalWrite(LED_GREEN, HIGH);
          digitalWrite(LED_RED, LOW);
          file.close();
       //   Serial.end();
          device.logging = 0;
          sendLogging();
          if (startSD()) return;
          sendSDContent();
          return;
        } else {
          file.write(c);
        }
      }
      break;
  }

  if ((millis() - device.last_logging_update) > 1000) {
    device.last_logging_update = millis();
    sendLogging();
  }
  if ((millis() - device.last_logging_flush) > 60000) {
    device.last_logging_flush = millis();
    file.flush();
  }

  if ((myRTC.sec() - device.logging_started) > device.runtime) {
    stopLogging();
  }
}
