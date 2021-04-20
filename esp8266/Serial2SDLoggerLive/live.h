
void setLive(void);
void sendLive(void);

void startLive(void) {
  if (device.logging) return;
  setResolution();
  device.live=1;
  setLive();
}

void stopLive(void){
//	Serial.end();
	device.live=0;
	setLive();
}

void handleLive(void){
  unsigned long data_length;
  if( Serial.available()<50) return;
  uint16_t pos=0;
  while(Serial.available()){
	  sd_buffer[pos]=(char) Serial.read();
	  if(sd_buffer[pos]=='\n'){
		  sd_buffer[pos]=',';
		  if(Serial.available()<10 || pos>1000){
			  sd_buffer[pos]=0;
			  sendLive();
			  return;
		  }
	  }
	  pos++;
  }

}
