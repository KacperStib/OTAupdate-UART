#include "SPIFFS.h"
#include "FS.h"
#include <WiFi.h>
#include <WebServer.h>

#define UART_RX_PIN 22  
#define UART_TX_PIN 21  
#define FIRMWARE_FILE "/firmware.bin" 

//Webserver
const char* ssid = "qlab_goscie";         
const char* password = "qlab2023"; 
WebServer server(80);

int totalBytes = 0;

//Webserver upload form
String uploadForm() {
  return "<form method='POST' action='/upload' enctype='multipart/form-data'>"
         "<input type='file' name='firmware'><br><br>"
         "<input type='submit' value='Upload Firmware'><br><br>"
         "</form>"
         "<form method='GET' action='/send-firmware'>"
         "<input type='submit' value='Send Firmware Over UART'>"
         "</form>";
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    return;
  }

  //Start Webserver
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", uploadForm());
  });
  server.on("/upload", HTTP_POST, []() {
    server.send(200, "text/plain", "File Uploaded Successfully");
  }, handleFileUpload);

  server.on("/send-firmware", HTTP_GET, []() {
    if (SPIFFS.exists(FIRMWARE_FILE)) {
      File firmware = SPIFFS.open(FIRMWARE_FILE, "r");
      if (firmware) {
        sendFirmwareOverUART(firmware);
        firmware.close();
        server.send(200, "text/plain", "Firmware sent over UART");
      } else {
        server.send(500, "text/plain", "Failed to open firmware file");
      }
    } else {
      server.send(404, "text/plain", "Firmware file not found");
    }
  });

  server.begin();
  Serial.println("HTTP server started");
  Serial.println(WiFi.localIP());
}

void loop() {
  server.handleClient();
}

void sendFirmwareOverUART(File file) {
  const size_t bufferSize = 128;  
  uint8_t buffer[bufferSize];
  size_t bytesRead;

  Serial.println("Starting firmware upload over UART in 3 sec!"); 
  delay(3000);

  // Sending firmware file size to RX
  if(totalBytes == 0){
    uint32_t fileSize = file.size();
    Serial.printf("Update size: %d b \n",fileSize);
    Serial1.write((uint8_t*)&fileSize, sizeof(fileSize)); 
  }

  delay(50);
  
  // Sending firmware file in bufferSize packets
  while ((bytesRead = file.read(buffer, bufferSize)) > 0) {
    Serial1.write(buffer, bytesRead); 
    Serial.print("."); 
    totalBytes += bytesRead;
    //baud rate and/or delay between packages to adjust
    delay(200);
  }

  Serial.printf("\nFirmware sent over UART. Sent %d bytes\n", totalBytes);
  totalBytes = 0;
}

// Write firmware file on SPIFFS
void handleFileUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Uploading: %s\n", upload.filename.c_str());
    File file = SPIFFS.open(FIRMWARE_FILE, "w");
    if (!file) {
      Serial.println("Failed to open file for writing");
      return;
    }
    file.close();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    File file = SPIFFS.open(FIRMWARE_FILE, "a");
    if (file) {
      file.write(upload.buf, upload.currentSize);
      file.close();
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    Serial.printf("Upload successful: %s, %u bytes\n", upload.filename.c_str(), upload.totalSize);
  }
}
