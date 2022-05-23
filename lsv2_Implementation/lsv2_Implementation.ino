
String ESPrx = ""; //Mensagem recebida no Rx do esp incluindo caracteres de controlo
String telegramaRecebido = ""; //Mensagem recebida no Rx do esp sem caracteres de controlo, apenas o telegrama
String MensagemEnviar = ""; // Mensagem a enviar no telegrama da fase de transferencia de dados
bool FaseInquerito = false;
bool FaseTransferenciaDados = false;
bool FaseRepouso = false;
bool espEmissor = false;
bool espRecetor = false;
int EnviarFicheiro_Counter = 0;
bool EnviarFicheiro = true;

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
    Serial.print("tenho estes bytes para ler: ");
    Serial.println(bytestoread);
    byte rx[bytestoread];
    Serial.readBytes(rx, bytestoread);

    for (int i = 0; i < (bytestoread) ; i++) {
      ESPrx += (char)rx[i];
    }

    /* Passagem de caracter especiais para string faz-se assim:
       Ver a que corresponde a mensagem que chegou segundo o LSV2 */
    // TNC está a tentar começar ou terminar uma conversa:
    if ( int(ESPrx[0]) == 5 ) {
      Serial.println("Eu percebi o caracter 5 ");
      espRecetor = true;
      lsv2_FaseInquerito();
    }
    else if ( int(ESPrx[0]) == 4 ) {
      Serial.println("Eu percebi o caracter 4 ");
      ESPrx = "";

    }

    // TNC está a reconhecer as mensagens do PC
    if ( int(ESPrx[0]) == 16 && int(ESPrx[1]) == 48 ) {
      Serial.println("Eu percebi o caracter 16 48 ");
      espEmissor = true;
      lsv2_TransferenciaDeDados();
    }
    else if ( int(ESPrx[0]) == 16 && int(ESPrx[1]) == 49 ) {
      Serial.println("Eu percebi o caracter 16 49 ");
      espEmissor = true;
      lsv2_FaseRepouso();
    }

    // TNC está a enviar um telegrama na fase de transferência
    // Esperar pelos últimos caracteres sem ser o BCC (Assumir que BCC está sempre certo)
    if (ESPrx.length() >= 5) {        // Só para não dar erros de tentar aceder caracteres que não existem na comparação seguinte
      if ( int(ESPrx[ESPrx.length() - 3]) == 16 && int(ESPrx[ESPrx.length() - 2]) == 3 ) {
        Serial.println("Eu percebi uma mensagem ");
        espRecetor = true;
        lsv2_TransferenciaDeDados();
      }
    }

    delay(1);
  }
}

/* Recebi o ENQ e vou enviar DLE 0 a confirmar que pode enviar o telegrama
  Ou então sou eu que quero enviar o ENQ*/
void lsv2_FaseInquerito() {

  if (espRecetor) {
    byte buff[5] = {16, 48, 2, 3, 4};    // DLE 0
    Serial.write(buff, 5);
    espRecetor = false;
  }
  if (espEmissor) {
    Serial.write(5);
    espEmissor = false;
  }
  ESPrx = "";
}

/* Recebi uma mensagem da CNC e quero interperta-la para depois mandar DLE 1 a dizer que recebi a mensagem com sucesso
   Ou então quero enviar um telegrama para a CNC
*/
void lsv2_TransferenciaDeDados() {
  if (espRecetor) {
    //Retirar os caracteres especiais e mandar a mensagem de receção sem erros
    telegramaRecebido = ESPrx.substring(2, ESPrx.length() - 3); // -3 -> DLE,ETX,BCC / -2 -> DLE,STX
    Serial.print("A mensagem recebida foi: ");
    Serial.println(telegramaRecebido);
    byte buff[2] = {16, 49};
    Serial.write(buff, 2);
    espRecetor = false;
  }

  if (espEmissor) {
    String ESPtx = "T_OK" ;
    int tamanhoMensagem = ESPtx.length();
    byte buffByte[tamanhoMensagem + 5];
    for (int i = 0; i < tamanhoMensagem; i++) {
      if (i == 0) {
        buffByte[0] = 16;
        buffByte[1] = 2;
      }

      buffByte[i + 2] = int(ESPtx[i]);

      if (i == (tamanhoMensagem - 1)) {
        buffByte[tamanhoMensagem + 2] = 16;
        buffByte[tamanhoMensagem + 3] = 3;
        buffByte[tamanhoMensagem + 4] = BCC(ESPtx);
      }

    }
    Serial.println("Mandei a mensagem");
    Serial.write(buffByte, (tamanhoMensagem + 5));
    espEmissor = false;
  }
  ESPrx = "";
}

/* Se recebi um EOT da CNC quer dizer que a comunicação vai terminar e não preciso de fazer nada, apenas apagar o ESPrx
   Se recebi DLE 1 então tenho que mandar para a CNC EOT para terminar a comunicação
*/
void lsv2_FaseRepouso() {

  //Se eu receber o EOT não preciso de fazer nada, só se quiser enviar
  if (espEmissor) {
    // Enviar o caracter EOT
    Serial.write(4);
    espEmissor = false;
  }

  ESPrx = "";

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
  Serial.println(strToConvert);
  strToConvert.toUpperCase();
  int comparingResult = 0;
  for (int i = 0; i < (strToConvert.length()) ; i++) {
    comparingResult = comparingResult ^ int(strToConvert[i]);
  }
  comparingResult = comparingResult ^ 3;
  Serial.print("Resultado final da conversão do BCC: ");
  Serial.println(comparingResult);

  return comparingResult;
}

void lsv2_EnviarFicheiroNC() {


  

}
