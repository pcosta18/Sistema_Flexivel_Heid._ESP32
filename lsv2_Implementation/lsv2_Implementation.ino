#include <string>

String ESPrx = "";

void setup() {
  /* Inicialização das portas séries
    Porta Série sem número está associada ao Tx0 e Rx0 do ESP (a que se usa para os testes e para comunicar com o PC)
    Porta Série 2 está associada ao RX2 e TX2 do ESP (a que se usa para comunicar com a CNC)
  */
  Serial.begin(9600, SERIAL_8N1);               // Tx0     Rx0 -
  // 115 200 bts, 8 data bits, 1 stop bit, paraty none
  Serial2.begin(115200, SERIAL_8N1);            // Por defeito    Rx2-16   Tx2-17
}

void loop() {

  /* Recepção pela porta série (A que vem da CNC) */
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
    Serial.println("o Resultado do bcc do recebido é: " );
    Serial.println(BCC(ESPrx), DEC);
    Serial.print("para a palavra " );
    Serial.println(ESPrx);
    ESPrx = "";

    //  Testar envio de caracteres especiais
    byte buff[2] = {16, 48};
    Serial.println("Vou mandar agora os dois bytes especiais");
    Serial.write(buff, 2);

  }

  delay(1);
}


int BCC(String strToConvert) {
/* Função para calcular o BCC do LSV2 
Uma tipica mensagem em LSV2 é algo como <DLE><STX>telegrama<DLE><ETX><BCC>
Para o cálculo do BCC os caracteres <DLE><STX> não contam
O input da função é o telegrama a ser convertido sem contar com os caracteres especiais
No incio da função convertemos tudo para Letras maiusculas para evitar erros de comparação uma vez que é case sensitive
O operador ^ (bitwise EXCLUSIVE OR) faz a comparação entre os bits do decimal dos caracteres representados pelo seu decimal
0  0  1  1    operand1
0  1  0  1    operand2
----------
0  1  1  0    ^ Result
No loop faz-se a comparação do telegrama
No final faz-se a comparação com o decimal 3 correspondente ao caracter <ETX>
A função retorna o DECIMAL do caracter correspondente à tabela ASCII
*/
  strToConvert.toUpperCase();
  int comparingResult = 0;
  for (int i = 0; i < (strToConvert.length()) ; i++) {
    comparingResult = comparingResult ^ int(strToConvert[i]);
  }
  comparingResult = comparingResult ^ 3;
  
  return comparingResult;
}
