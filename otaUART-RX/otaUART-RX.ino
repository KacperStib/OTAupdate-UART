#include <Update.h>  

#define UART_RX_PIN 12  
#define UART_TX_PIN 13  

#define UART_BUFFER_SIZE 128  // Zmniejszony rozmiar bufora do 256 bajtów
char uart_buffer[UART_BUFFER_SIZE];
int uart_bytes_read = 0;
unsigned long totalBytesReceived = 0;
int offset = 0;

void setup() {
  Serial1.begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
  Serial.begin(115200); 

  //clear
    while (Serial1.available()) {
      Serial1.read();

    }
  Serial.println("UART OTA Update ready. Send firmware via UART.");

  
}

void loop() {
  if (Serial1.available()) {
    uart_bytes_read = Serial1.readBytes(uart_buffer, UART_BUFFER_SIZE);
    Serial.println("rec");
    showBytes();
    
    if (uart_bytes_read > 0) {
      Serial.println("Receiving OTA data...");
      performOTAUpdate(uart_buffer, uart_bytes_read);
    }
  }
}


void performOTAUpdate(char* data, size_t len) {
  if (!Update.isRunning()) {
    // Zakładamy większy rozmiar firmware'u. Update.begin(UPDATE_SIZE_UNKNOWN) oznacza, że OTA będzie się dostosowywać
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {  
      Serial.println("OTA begin failed");
      return;
    }
    Serial.println("OTA begin success");
  }

  // Zapis danych do pamięci flash
  if (Update.write((uint8_t*)data, len) != len) {
    Serial.println("OTA write failed");
    return;
  }

  totalBytesReceived += len;
  Serial.print("Total bytes received: ");
  Serial.println(totalBytesReceived);

  // Sprawdź, czy OTA się zakończyło (gdy Update.end() zostanie wywołane po ostatnich danych)
  if (Serial1.available() == 0 && totalBytesReceived > 0) {  
    if (Update.end(true)) {
      if (Update.isFinished()) {
        Serial.println("OTA success, rebooting...");
        ESP.restart();  // Uruchom ponownie ESP
      } else {
        Serial.println("OTA failed");
      }
    } else {
      Serial.println("OTA end failed");
    }
  }
}

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
