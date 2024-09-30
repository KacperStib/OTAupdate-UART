#include <Update.h>

#define UART_RX_PIN 12  
#define UART_TX_PIN 13  

#define UART_BUFFER_SIZE 128
char uart_buffer[UART_BUFFER_SIZE];
int uart_bytes_read = 0;
int offset = 0;

size_t totalBytesReceived = 0;  

uint32_t fileSize = 0;
uint8_t* valuePtr = (uint8_t*)&fileSize;

void setup() {
  Serial1.begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
  Serial.begin(115200);

  Serial.println("UART OTA Update ready. Send firmware via UART.");
}

void loop() {
  if (Serial1.available()) {

    // Receive update size
    if(fileSize == 0){
      for (size_t i = 0; i < sizeof(fileSize); ++i) {
        valuePtr[i] = Serial1.read();
      }
      Serial.printf("Upadte size: %d b\n", fileSize);
    }
    uart_bytes_read = Serial1.readBytes(uart_buffer, UART_BUFFER_SIZE);

    // Test
    /*
    if(totalBytesReceived > (fileSize - UART_BUFFER_SIZE))
      showBytes();
    */
    if (uart_bytes_read > 0) {
      //Serial.println("Receiving OTA data...");
      performOTAUpdate(uart_buffer, uart_bytes_read);
    }
  }
}

void performOTAUpdate(char* data, size_t len) {

  //OTA start
  if (!Update.isRunning()){
    if (!Update.begin(fileSize)) {  
      Serial.println("OTA begin failed");
      return;
    }
    Serial.println("OTA begin success");
  }

  //OTA write
  if (Update.write((uint8_t*)data, len) != len) {
    Serial.println("OTA write failed");
    return;
  }

  totalBytesReceived += len;
  Serial.printf("Received: %d b\n", totalBytesReceived);

  //OTA end
  if (Serial1.available() == 0 && totalBytesReceived >= fileSize) {  
    if (Update.end(true)) {
      if (Update.isFinished()) {
        Serial.println("OTA success, rebooting...");
        ESP.restart();  
      } else {
        Serial.println("OTA failed");
      }
    } else {
      Serial.println("OTA end failed");
    }
    Serial.printf("Total bytes received: %d\n", totalBytesReceived);
  }
}


// Show all bytes of sent package (cannot be used at every package - Serial.prinf is too slow)
void showBytes(){
  for(int i = 0 ; i < UART_BUFFER_SIZE / 16 ; i++){
    Serial.printf(" %x | ", offset);
    offset++;
    for(int j = 0 ; j < 16 ; j++){
      Serial.printf("  %02hhx  " ,uart_buffer[16 * i + j]);
    }
    Serial.println();
  }    
}
