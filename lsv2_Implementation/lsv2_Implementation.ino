String ESPrx2 = "";                                                   //Mensagem recebida no Rx do esp incluindo caracteres de controlo
String ESPrx0 = "";                                                  //Mensagem recebida no Rx do esp incluindo caracteres de controlo
String telegramaRecebido = "";                                       //Mensagem recebida no Rx do esp sem caracteres de controlo, apenas o telegrama
String MensagemEnviar = "";                                          // Mensagem a enviar no telegrama da fase de transferencia de dados
byte buffByte_tx2[1024];
bool espEmissor = false;                                             //Caso seja a vez do esp a mandar a mensagem
bool espRecetor = false;                                             //Caso seja a vez de o esp receber a mensagem
int EnviarFicheiro_Counter = 0;                                      // Counter para saber qual é a mensagem para enviar
bool EnviarFicheiro_Controlo = true;                                 //Controlo para saber se ainda há mais mensagens para enviar
String ProgramaNCTeste = "BEGIN PGM 151 MM\n"                        // Programa, em linguagem Heid. para enviar para o computador
                         "BLK FORM 0.1 Z X + 0 Y + 0 Z - 20\n"       //CUIDADO, NÃO PODE LEVAR MAIS QUE 1020 BYTES (1020 CARACTERES)
                         "BLK FORM 0.2 X + 100 Y + 100 Z + 0\n"
                         "TOOL DEF 1 L + 0 R + 4\n"
                         "TOOL CALL 1 Z S4000\n"
                         "L Z + 100 R0 F MAX\n"
                         "L X + 20 Y + 30 R0 F MAX M3\n"
                         "L Z + 2 R0 F MAX M8\n"
                         "L Z - 22 R0 F400\n"
                         "L Z + 2 R0 F MAX\n"
                         "L X + 50 Y + 70 R0 F MAX\n"
                         "L Z - 22 R0 F400\n"
                         "L Z + 2 R0 F MAX\n"
                         "L X + 75 Y + 30 R0 F MAX\n"
                         "L Z - 22 R0 F400\n"
                         "L Z + 100 R0 F MAX M2\n"
                         "END PGM 151 MM";
unsigned long previousMillis_Timer1 = 0;                            // Timer caso a receção da mensagem seja desconhecida e não compare com nada (<ACK>, mensagens de erro, etc)
unsigned long Timer1_Interval = 20000;                              // 20 segundos

void setup() {
  /* Inicialização das portas séries
    Porta Série sem número está associada ao Tx0 e Rx0 do ESP
    Esta porta é a que é usada para vizualizar o que está a acontecer no IDE
    Porta Série 2 está associada ao RX2 e TX2 do ESP (a que se usa para comunicar com a CNC)
    Esta é a que vai comunicar com a maquina a 115 200 bts, 8 data bits, 1 stop bit, paraty none*/

  Serial.begin(9600, SERIAL_8N1);               // Tx0     Rx0 -
  //Serial1.begin(9600,SERIAL_8N1,18,19);         // Por defeito    Rx1- 9   Tx1-10
  /* https://www.youtube.com/watch?v=7CQNLu5OfT8
     https://www.youtube.com/watch?v=GwShqW39jlE */
  Serial2.begin(115200, SERIAL_8N1, 17, 16);           // Por defeito    Rx2-16   Tx2-17
}

void loop() {
  //implementação do timer para eliminar mensagens não interpretadas
  if (millis() - previousMillis_Timer1 > Timer1_Interval) {
    previousMillis_Timer1 = millis();
    ESPrx2 = "";
  }

  /* Para fazer a inicialização do envio de ficheiro enquanto não há mais nenhum input
    COLOCAR NO ide : lsv2_EnviarFicheiroNC */
  int bytestoread2 = Serial.available();
  if (bytestoread2 > 0) {
    delay(200);
    Serial.print("tenho estes bytes para ler da porta 0: ");
    Serial.println(bytestoread2);
    byte rx0[bytestoread2];
    Serial.readBytes(rx0, bytestoread2);

    for (int i = 0; i < (bytestoread2) ; i++) {
      ESPrx0 += (char)rx0[i];
    }

    if ( ESPrx0 == "lsv2_EnviarFicheiroNC" ) {
      Serial.println("Eu percebi que é para começar a enviar um ficheiro ");
      espEmissor = true;
      EnviarFicheiro_Controlo = true;
      lsv2_FaseInquerito();
    }
    ESPrx0 = "";
  }

  /* Recepção pela porta série 2 (A que vem da CNC) */
  int bytestoread = Serial2.available();
  if (bytestoread > 0) {
    delay(200);
    previousMillis_Timer1 = millis();
    Serial.print("tenho estes bytes para ler da porta 2 (CNC): ");
    Serial.println(bytestoread);
    byte rx[bytestoread];
    Serial2.readBytes(rx, bytestoread);

    for (int i = 0; i < (bytestoread) ; i++) {
      ESPrx2 += (char)rx[i];
    }
    Serial.print("A mensagem que chegou é esta: ");
    Serial.println(ESPrx2);
    /* Para saber quais são as mensagens que temos que enviar */

    if (EnviarFicheiro_Controlo) {
      lsv2_EnviarFicheiroNC();
    }

    /*Ver a que corresponde a mensagem que chegou segundo o LSV2 */
    // TNC está a tentar começar ou terminar uma conversa:
    if ( int(ESPrx2[0]) == 5 ) {
      Serial.println("Eu percebi o caracter 5 ");
      espRecetor = true;
      lsv2_FaseInquerito();
    }
    else if ( int(ESPrx2[0]) == 4 ) {
      Serial.println("Eu percebi o caracter 4 ");
      ESPrx2 = "";

    }

    // TNC está a reconhecer as mensagens do PC
    if ( int(ESPrx2[0]) == 16 && int(ESPrx2[1]) == 48 ) {
      Serial.println("Eu percebi o caracter 16 48 ");
      espEmissor = true;
      lsv2_TransferenciaDeDados();
    }
    else if ( int(ESPrx2[0]) == 16 && int(ESPrx2[1]) == 49 ) {
      Serial.println("Eu percebi o caracter 16 49 ");
      espEmissor = true;
      lsv2_FaseRepouso();
    }

    // TNC está a enviar um telegrama na fase de transferência
    // Esperar pelos últimos caracteres sem ser o BCC (Assumir que BCC está sempre certo)
    if (ESPrx2.length() >= 5) {        // Só para não dar erros de tentar aceder caracteres que não existem na comparação seguinte
      if ( int(ESPrx2[ESPrx2.length() - 3]) == 16 && int(ESPrx2[ESPrx2.length() - 2]) == 3 ) {
        Serial.println("Eu percebi uma mensagem ");
        espRecetor = true;
        lsv2_TransferenciaDeDados();
      }
    }

    delay(1);
  }
}

/* PC recebe o ENQ e envia DLE 0 a confirmar que pode enviar o telegrama
   Ou
   PC quer enviar o ENQ*/
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
  ESPrx2 = "";
}

/* PC recebeu uma mensagem da CNC e quer interperta-la para depois mandar DLE 1 --> "recebei a mensagem com sucesso"
   Ou
   quer enviar um telegrama para a CNC */
void lsv2_TransferenciaDeDados() {
  if (espRecetor) {
    //Retirar os caracteres especiais e mandar a mensagem de receção sem erros
    telegramaRecebido = ESPrx2.substring(2, ESPrx2.length() - 3); // 3 -> DLE,ETX,BCC / 2 -> DLE,STX
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

      buffByte[i + 2] = buffByte_tx2[i];

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
    memset(buffByte_tx2, 0, 1024); //limpar o array para evitar confusões apesar de não ser necessário
  }
  ESPrx2 = "";
}

/* Se PC recebeu EOT da CNC quer dizer que a comunicação vai terminar e não precisa de fazer nada, apenas apagar o ESPrx2 (a mensagem)
   Se recebeu DLE 1 então deve que mandar para a CNC EOT para terminar a comunicação */
void lsv2_FaseRepouso() {

  if (espEmissor) {
    // Enviar o caracter EOT
    Serial2.write(4);
    Serial.println("Mandei o caracter 4");
    espEmissor = false;
  }

  ESPrx2 = "";

}


void lsv2_EnviarFicheiroNC() {

  // Saber qual é a primeira mensagem a ser enviada, neste caso, login para fazer tratamento de ficheiros
  if (EnviarFicheiro_Counter == 0 ) {
    telegramaRecebido = "";
    MensagemEnviar = "A_LGFILE";
    MsgToByteArray(MensagemEnviar);
    EnviarFicheiro_Counter++;
  }

  // Enviar o Nome do Ficheiro
  if (EnviarFicheiro_Counter == 1 && telegramaRecebido == "T_OK" ) {
    telegramaRecebido = "";
    String identificador = "C_FL";
    String nomeFicheiro = "PECATESTE.H\0";                           // Cuidado, não pode levar mais que 80 bytes (80 caracteres)!
    MensagemEnviar = identificador + nomeFicheiro;
    MsgToByteArray(MensagemEnviar);
    EnviarFicheiro_Counter++;
  }

  // Enviar o FileBlock ou o texto do program. Não esquecer que o tamanho máximo é de 1024 - 4 = 1020
  if (EnviarFicheiro_Counter == 2 && telegramaRecebido == "T_OK" ) {
    telegramaRecebido = "";
    String identificador = "S_FL";
    MensagemEnviar = identificador + ProgramaNCTeste;
    MsgToByteArray(MensagemEnviar);
    EnviarFicheiro_Counter++;
  }

  // Dizer que acabmos de mandar a mensagem. Não esquecer que este identifcador não requeres reposta T_OK por parte da CNC
  if (EnviarFicheiro_Counter == 3 && telegramaRecebido == "T_OK" ) {
    telegramaRecebido = "";
    MensagemEnviar = "T_FD" + ProgramaNCTeste;
    MsgToByteArray(MensagemEnviar);
    // Passagem dos \n para \0
    for (int i = 0; i < MensagemEnviar.length(); i++) {
      if (buffByte_tx2[i] == 10) {
        buffByte_tx2[i] = 0;
      }
    }
    EnviarFicheiro_Counter++;
  }

  // Se a CNC reconhecer a mensagem "T_FD" que o PC enviou, então podemos passar para o logout
  // Tem que se sobrepor o processo automático do lsv2 uma vez que esta mensagem não requer resposta T_OK
  if (EnviarFicheiro_Counter == 4 && int(ESPrx2[0]) == 16 && int(ESPrx2[1]) == 49  ) {
    espEmissor = true;
    lsv2_FaseRepouso();
    delay(1000);
    Serial2.write(5);
    MensagemEnviar = "A_LO";
    MsgToByteArray(MensagemEnviar);
    EnviarFicheiro_Counter++;
  }

  // Acabou a comunicação
  if (EnviarFicheiro_Counter == 5 && telegramaRecebido == "T_OK" ) {
    telegramaRecebido = "";
    EnviarFicheiro_Counter = 0;
    EnviarFicheiro_Controlo = false;
  }

  // Isto está aqui porque quando recebemos o T_OK queremos voltar a inciar outra comunicação para enviar o telegrama seguinte
  if (int(ESPrx2[0]) == 4 & EnviarFicheiro_Controlo == true) {
    delay(200);
    Serial2.write(5);
  }
}


/* Função para calcular o BCC do LSV2
    Uma tipica mensagem em LSV2 é algo como <DLE><STX>telegrama<DLE><ETX><BCC>
    Para o cálculo do BCC os caracteres <DLE><STX> não contam
    O input da função é o telegrama a ser convertido sem contar com os caracteres especiais
    No incio da função convertemos tudo para letras maiusculas para evitar erros de comparação uma vez que é case sensitive
    O operador ^ (bitwise EXCLUSIVE OR) faz a comparação entre os respetivos bits, neste caso, de dois caracteres
    0  0  0  0  0  0  1  1    caracter 1
    0  0  0  0  0  1  0  1    caracter 2
    ----------------------
    0  0  0  0  0  1  1  0    ^ Result
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

void MsgToByteArray(String strToConvert) {
  for (int i = 0; i < MensagemEnviar.length(); i++) {
    buffByte_tx2[i] = int(MensagemEnviar[i]);
  }
}
