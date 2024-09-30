#define ETH_PHY_TYPE ETH_PHY_LAN8720
#define ETH_PHY_ADDR 0
#define ETH_PHY_MDC 23
#define ETH_PHY_MDIO 18
#define ETH_PHY_POWER 12  //GATEWAY - 5
#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT

#include "SPIFFS.h"
#include "FS.h"
#include <ETH.h>             // Include Ethernet library
#include <WebServer.h>
#include <Wire.h>

#define UART_RX_PIN 4  
#define UART_TX_PIN 5 
#define FIRMWARE_FILE "/firmware.bin" 

WebServer server(80);

int totalBytes = 0;
static bool eth_connected = false;

// Handle Ethernet Events:
void NetworkEvent(arduino_event_id_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      ETH.setHostname("esp32-ethernet");
      break;

    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;

    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("Got an IP Address: ");
      Serial.println(ETH.localIP());
      eth_connected = true;
      break;

    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;

    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;

    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  //Serial1.begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

  Wire.begin(5, 4, 400000);
  Wire.setTimeOut(20);

  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    return;
  }

  // Register event handler for Ethernet events
  Network.onEvent(NetworkEvent);

  // Start Ethernet
  ETH.begin();

  // Set up the web server
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

  Serial.println("Waiting for Ethernet connection...");
}

void loop() {
  if (eth_connected) {
    server.handleClient();
  }
}

void sendFirmwareOverUART(File file) {
  const size_t bufferSize = 128;  
  uint8_t buffer[bufferSize];
  size_t bytesRead;

  
  for(int i = 1; i <= 6 ; i ++){
    Wire.beginTransmission(i);
    Wire.write(1);
    Wire.write(2);
    Wire.endTransmission();
  }
  
  Wire.end();
  delay(10);
  Serial1.begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

  Serial.println("Starting firmware upload over UART in 3 sec!"); 
  delay(3000);

  if (totalBytes == 0) {
    uint32_t fileSize = file.size();
    Serial.printf("Update size: %d b \n", fileSize);
    Serial1.write((uint8_t*)&fileSize, sizeof(fileSize)); 
  }

  delay(50);

  while ((bytesRead = file.read(buffer, bufferSize)) > 0) {
    Serial1.write(buffer, bytesRead); 
    Serial.print("."); 
    totalBytes += bytesRead;
    delay(200);
  }

  Serial.printf("\nFirmware sent over UART. Sent %d bytes\n", totalBytes);
  totalBytes = 0;
  Serial.end();
  delay(10);
  Wire.begin(5, 4, 400000);
  Wire.setTimeOut(20);
}

String uploadForm() {
  return "<form method='POST' action='/upload' enctype='multipart/form-data'>"
         "<input type='file' name='firmware'><br><br>"
         "<input type='submit' value='Upload Firmware'><br><br>"
         "</form>"
         "<form method='GET' action='/send-firmware'>"
         "<input type='submit' value='Send Firmware Over UART'>"
         "</form>";
}

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
