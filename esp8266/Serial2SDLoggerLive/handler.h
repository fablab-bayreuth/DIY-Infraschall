
String getContentType(String filename) { // determine the filetype of a given filename, based on the extension
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  return "text/plain";
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  //Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File SPIFFSfile = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(SPIFFSfile, contentType);    // Send it to the client
    SPIFFSfile.close();                                          // Close the file again
    //Serial.println(String("\tSent file: ") + path);
    return true;
  }
  //Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

void sendNotFound(void) {
  server.send(404, "text/plain; charset=utf-8", "404: File Not Found");
}



void handleNotFound() { // if the requested file or page doesn't exist, return a 404 not found error
  if (!handleFileRead(server.uri())) {        // check if the file exists in the flash memory (SPIFFS), if so, send it
    sendNotFound();
  }
}

void sendDownload(bool active); //pull in function from socket.h

void download(void){
  char f[13];
  server.arg(0).toCharArray(f, 13);
  if (startSD()){
    sendNotFound();
    return;
  }
  file = SD.open(f, FILE_READ);
  if(! file){
    sendNotFound();
    return;
  }
  sendDownload(true,file.size(),file.position());
  server.client().write("HTTP/1.1 200\r\n", 14);
  server.client().write("Content-Type: application/octet-stream\r\n", 40);
  server.client().write("Content-Description: ", 21);
  server.client().write(f, strlen(f));
  server.client().write("\r\n", 2);
  server.client().write("Content-Transfer-Encoding: binary\r\n", 35);
  server.client().write("Expires: 0\r\n", 12);
  server.client().write("Cache-Control: must-revalidate, post-check=0, pre-check=0\r\n", 59);
  server.client().write("Content-disposition: inline; filename=", 38);
  server.client().write(f, strlen(f));
  server.client().write("\r\n", 2);
  server.client().write("Content-Length: ", 16);
  char len[12];
  mes="";
  mes+=file.size();
  mes.toCharArray(len,12);
  server.client().write(len, strlen(len));
  server.client().write("\r\n", 2);
  server.client().write("Connection: close\r\n", 19);
  server.client().write("\r\n", 2);
  unsigned long data_len;
  unsigned long last_connect_check;
  while(data_len=file.available()){
    if(data_len>1024) data_len=1024;
    file.read(sd_buffer,data_len);
    server.client().write(sd_buffer,data_len);
    if((millis()-last_connect_check)>1000){
      last_connect_check=millis();
      if(! server.client().connected()) break;
      sendDownload(true,file.size(),file.position());
    }
  }
  sendDownload(false,file.size(),file.position());
  file.close();
 
}
