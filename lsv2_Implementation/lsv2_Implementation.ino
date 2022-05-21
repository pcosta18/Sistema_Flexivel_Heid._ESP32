#include <string>

String ESPrx = "";

void setup() {
  Serial.begin(9600, SERIAL_8N1);               // Tx0     Rx0 -
  // 115 200 bts, 8 data bits, 1 stop bit, paraty none
  Serial2.begin(115200, SERIAL_8N1);            // Por defeito    Rx2-16   Tx2-17
}

void loop() {

  /* Recepção pela porta série 2 (A que vem da CNC) */
  int bytestoread = Serial.available();
  if (bytestoread > 0) {
    Serial.println("tenho estes bytes para ler: ");
    Serial.println(bytestoread);
    byte rx[bytestoread];
    Serial.readBytes(rx, bytestoread);

    for (int i = 0; i < (bytestoread) ; i++) {
      ESPrx += (char)rx[i];
    }
   // Para ver o que foi recebido nesta fase de testes, mais tarde terá que ser removido
   Serial.println(ESPrx);
   ESPrx = "";

   //  Testar envio de caracteres especiais
   byte buff[2] = {16,48};
   Serial.println("Vou mandar agora os dois bytes especiais");
   Serial.write(buff,2);

  }

  delay(1);
}
