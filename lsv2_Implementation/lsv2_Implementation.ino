String ESPrx = ""; //Mensagem recebida no Rx do esp incluindo caracteres de controlo
String ESPrx0 = ""; //Mensagem recebida no Rx do esp incluindo caracteres de controlo
String telegramaRecebido = ""; //Mensagem recebida no Rx do esp sem caracteres de controlo, apenas o telegrama
String MensagemEnviar = ""; // Mensagem a enviar no telegrama da fase de transferencia de dados
bool FaseInquerito = false;
bool FaseTransferenciaDados = false;
bool FaseRepouso = false;
bool espEmissor = false;
bool espRecetor = false;
int EnviarFicheiro_Counter = 0;
bool EnviarFicheiro_Controlo = true;
String ProgramaNCTeste = "BEGIN PGM 151 MM\0"
                         "BLK FORM 0.1 Z X + 0 Y + 0 Z - 20\0"
                         "BLK FORM 0.2 X + 100 Y + 100 Z + 0\0"
                         "TOOL DEF 1 L + 0 R + 4\0"
                         "TOOL CALL 1 Z S4000\0"
                         "L Z + 100 R0 F MAX\0"
                         "L X + 20 Y + 30 R0 F MAX M3\0"
                         "L Z + 2 R0 F MAX M8\0"
                         "L Z - 22 R0 F400\0"
                         "L Z + 2 R0 F MAX\0"
                         "L X + 50 Y + 70 R0 F MAX\0"
                         "L Z - 22 R0 F400\0"
                         "L Z + 2 R0 F MAX\0"
                         "L X + 75 Y + 30 R0 F MAX\0"
                         "L Z - 22 R0 F400\0"
                         "L Z + 100 R0 F MAX M2\0"
                         "END PGM 151 MM\0";

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

  int bytestoread2 = Serial.available();
  if (bytestoread2 > 0) {
    delay(200);
    Serial.print("tenho estes bytes para ler da porta 0: ");
    Serial.println(bytestoread2);
    byte rx0[bytestoread2];
    Serial.readBytes(rx0, bytestoread2);

    for (int i = 0; i < (bytestoread2) ; i++) {
      ESPrx2 += (char)rx0[i];
    }

    if ( ESPrx2 == "lsv2_EnviarFicheiroNC" ) {
      Serial.println("Eu percebi que é para começar a enviar um ficheiro ");
      espEmissor = true;
      EnviarFicheiro_Controlo = true;
      lsv2_FaseInquerito();
    }
    ESPrx2 = "";
  }

  int bytestoread = Serial2.available();
  if (bytestoread > 0) {
    delay(200);
    Serial.print("tenho estes bytes para ler da porta 2 (CNC): ");
    Serial.println(bytestoread);
    byte rx[bytestoread];
    Serial2.readBytes(rx, bytestoread);

    for (int i = 0; i < (bytestoread) ; i++) {
      ESPrx += (char)rx[i];
    }
    Serial.print("A mensagem que chegou é esta: ");
    Serial.println(ESPrx);
    /* Para saber quais são as mensagens que temos que enviar */

    if (EnviarFicheiro_Controlo) {
      lsv2_EnviarFicheiroNC();
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
    byte buff[] = {16, 48};    // DLE 0
    Serial2.write(buff, 2);
    Serial.println("Enviei o caracter 16 48");
    espRecetor = false;
  }
  if (espEmissor) {
    Serial2.write(5);
    Serial.println("Eu enviei o caracter 5 ");
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
    Serial2.write(buff, 2);
    Serial.println("Eu mandei os caracteres 16 49 ");
    espRecetor = false;
  }

  if (espEmissor) {
    int tamanhoMensagem = MensagemEnviar.length();
    byte buffByte[tamanhoMensagem + 5];
    for (int i = 0; i < tamanhoMensagem; i++) {
      if (i == 0) {
        buffByte[0] = 16;
        buffByte[1] = 2;
      }

      buffByte[i + 2] = int(MensagemEnviar[i]);

      if (i == (tamanhoMensagem - 1)) {
        buffByte[tamanhoMensagem + 2] = 16;
        buffByte[tamanhoMensagem + 3] = 3;
        buffByte[tamanhoMensagem + 4] = BCC(MensagemEnviar);
      }
    }
    
    Serial2.write(buffByte, (tamanhoMensagem + 5));
    Serial.println("Mensagem que enviei foi");
    Serial.println(MensagemEnviar);
    espEmissor = false;
    MensagemEnviar = "";
  }
  ESPrx = "";
}

/* Se recebi um EOT da CNC quer dizer que a comunicação vai terminar e não preciso de fazer nada, apenas apagar o ESPrx
   Se recebi DLE 1 então tenho que mandar para a CNC EOT para terminar a comunicação */
void lsv2_FaseRepouso() {

  //Se eu receber o EOT não preciso de fazer nada, só se quiser enviar
  if (espEmissor) {
    // Enviar o caracter EOT
    Serial2.write(4);
    Serial.println("Mandei o caracter 4");
    espEmissor = false;
  }

  ESPrx = "";

}


void lsv2_EnviarFicheiroNC() {

  // Enviar ENQ para estabelecer uma comunicação e estabelecer a primeira mensagem a ser enviada
  if (EnviarFicheiro_Counter == 0 ) {
    telegramaRecebido = "";
    MensagemEnviar = "A_LGFILE";
    EnviarFicheiro_Counter++;
  }
  // Enviar o Nome do Ficheiro
  if (EnviarFicheiro_Counter == 1 && telegramaRecebido == "T_OK" ) {
    telegramaRecebido = "";
    String identificador = "C_FL";
    String nomeFicheiro = "PECATESTE.H\0";  // Cuidado que isto tudo não pode levar mais que 80 bytes (80 caracteres)
    MensagemEnviar = identificador + nomeFicheiro;
    EnviarFicheiro_Counter++;
  }

  // Enviar o FileBlock ou o texto do program. Não esquecer que o tamanho máximo é de 1024 - 4 = 1020
  if (EnviarFicheiro_Counter == 2 && telegramaRecebido == "T_OK" ) {
    telegramaRecebido = "";
    String identificador = "S_FL";
    MensagemEnviar = identificador + ProgramaNCTeste;
    EnviarFicheiro_Counter++;
  }

  // Dizer que acabmos de mandar a mensagem. Não esquecer que este identifcador não requeres reposta T_OK por parte da CNC
  if (EnviarFicheiro_Counter == 3 && telegramaRecebido == "T_OK" ) {
    telegramaRecebido = "";
    MensagemEnviar = "T_FD";
    EnviarFicheiro_Counter++;
  }

  // Se ele reconhecer a mensagem "T_FD" que enviei, então podemos passar para o logout.
  if (EnviarFicheiro_Counter == 4 && int(ESPrx[0]) == 16 && int(ESPrx[1]) == 49  ) {
    espEmissor = true;
    lsv2_FaseRepouso();
    delay(400);
    Serial2.write(5);
    MensagemEnviar = "A_LO";
    EnviarFicheiro_Counter++;
  }

  // Acabou a comunicação
  if (EnviarFicheiro_Counter == 5 && telegramaRecebido == "T_OK" ) {
    telegramaRecebido = "";
    EnviarFicheiro_Counter = 0;
    EnviarFicheiro_Controlo = false;
  }

  // Isto está aqui porque quando recebemos o T_OK queremos voltar a inciar outra comunicação para enviar o telegrama seguinte
  if (int(ESPrx[0]) == 4) {
    delay(200);
    Serial2.write(5);
  }
}


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
int BCC(String strToConvert) {

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
