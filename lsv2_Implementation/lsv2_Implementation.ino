void setup() {
  // put your setup code here, to run once:
  // SERIE
  pinMode(2, OUTPUT); //Pino 2 passa a ser saída
  pinMode(4, INPUT); //Pino 4 passa a ser entrada
}

int analogValue;
void loop() {

  varA = digitalRead(4); // Lê o Pino 4 (0 ou 3,3 V)
  digitalWrite(2, varA); // ativa/desativa Pino 2 (3,3 ou 0 V)
}
