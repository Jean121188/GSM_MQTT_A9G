#include<stdio.h>
#include<string.h>
#define DEBUG true

#include <SoftwareSerial.h>

#define BAUD_RATE  19200// 115200
#define A9_BAUD_RATE  19200// 115200

#define A9G_PON     16  //ESP12 GPIO16 A9/A9G POWON
#define A9G_POFF    15  //ESP12 GPIO15 A9/A9G POWOFF
#define A9G_WAKE    13  //ESP12 GPIO13 A9/A9G WAKE
#define A9G_LOWP    2  //ESP12 GPIO2 A9/A9G ENTER LOW POWER MODULE

void Init_A9G();
void AT_Inicializa_A9G();

int A9GPOWERON();
int A9GMQTTCONNECT();
int callback_A9G(String msg);
String sendData(String command, const int timeout, boolean debug);
String leStringSerial();
String leStringSerialA9G();
String Recive_Subscrible_Loop();
void Send_Publish_Loop();
void Serial_live();

int trataMsg(String msg);
int count_erros = 0;      //
int max_tent_error = 5;   // (pode alterar)
unsigned long previousMillis = 0;
String msg_return = "";
String Error_code = "";

String MQTTsub = "\"/api/subscribe\",";                 // Subscrible (pode alterar)
String MQTTfin = "1,0";                                 // Obrigatorio
String MQTTSubscrible = "AT+MQTTSUB="+MQTTsub+MQTTfin;  // Obrigatorio

String MQTTid = "\"1\"";                                                       // Obrigatorio
String MQTTip = "\"52.90.107.233\"";                                           // IP          (pode alterar)
String MQTTporta = "1883";                                                     // Porta       (pode alterar)
String MQTTConexao = "AT+MQTTCONN="+MQTTip+","+MQTTporta+","+MQTTid+",120,1";  // Obrigatorio

String ATCMD  ="AT+MQTTPUB=";         // Obrigatorio
String Topic  = "\"/api/publish\",";  // Topico                 (pode alterar)
String TMP    = "\"vai po\"";         // mensagem a ser enviada (pode alterar)
String Final  = ", 0, 0, 0";          // Obrigatorio
String command = ATCMD+Topic+TMP+Final;

SoftwareSerial swSer(14, 12);

void setup()
{
  Init_A9G();
  AT_Inicializa_A9G();
}

void loop()
{
  unsigned long currentMillis = millis();
  
  if ((unsigned long)(currentMillis - previousMillis) >= 30000) // 30 segundos
  {
    Send_Publish_Loop();
    previousMillis = currentMillis;
  }
  Recive_Subscrible_Loop();
}


void Init_A9G()
{
  Serial.begin(BAUD_RATE);
  swSer.begin(A9_BAUD_RATE);
  for (char ch = ' '; ch <= 'z'; ch++) {
    swSer.write(ch);
  }
  swSer.println("");

  pinMode(A9G_PON, OUTPUT);//LOW LEVEL ACTIVE
  pinMode(A9G_POFF, OUTPUT);//HIGH LEVEL ACTIVE
  pinMode(A9G_LOWP, OUTPUT);//LOW LEVEL ACTIVE

  digitalWrite(A9G_PON, HIGH); 
  digitalWrite(A9G_POFF, LOW); 
  digitalWrite(A9G_LOWP, HIGH); 

  Serial.println("After 2s A9G will POWER ON.");
  delay(2000);
  if(A9GPOWERON()==1)
  {
       Serial.println("A9G POWER ON.");
  }
}

void AT_Inicializa_A9G()
{
  String comandosAT[] = {"AT", "AT+IPR?", "AT+CGATT=1", "AT+CGDCONT=1,\"IP\",\"CMNET\"", "AT+CGACT=1,1", "AT+MQTTDISCONN", MQTTConexao, MQTTSubscrible}; //5
  // "AT+MQTTCONN=\"52.90.107.233\",1883,\"1\",120,1"
  size_t quantAT = (sizeof(comandosAT)/sizeof(comandosAT[0]));
  
    int i = 0;
    while(i < quantAT)
    {
      int est = A9GMQTTCONNECT(comandosAT[i]);
//      Serial.print("est: "); Serial.println(est);
//      Serial.print("i inicial: "); Serial.println(i);
      if(est == 1)
      {
        // Se ele retornar TRUE, é que deu certo o comando AT
        // complementa a contagem, passando para o proximo comando
        i++;
      }
      else if(est == 0)
      {
        // Se ele retornar FALSE, é que deu errado o comando AT
        // A contagem deve se manter a mesma para que repita o comando AT
        i = i;
      }
//      Serial.print("i depois dos complementos: "); Serial.println(i);

      if(est == 0) // Se houver MSG de Erro, Exibe o código
      {
        Serial.print("Erro persistente: ");Serial.println(Error_code);
      }
      
    }
    Serial.println("Sai do While, SUCESSO!");
}

void Send_Publish_Loop()
{
  // AT+MQTTPUB=<topic>,<sub>,<qos>
  // Exemplo:
  // AT+MQTTPUB="/api/publish", "Deu BOM", 0, 0, 0

  Serial.println("=====================");
  Serial.println(command);
  Serial.println("=====================");
  
  msg_return = sendData(command,1000,0);
  int return_data = trataMsg(msg_return);
  if(return_data == 1)
  {
    Serial.println("Publicado...");
  }
  else if(return_data == 0)
  {
    Serial.println("Nao publicado...");
    Serial.println(Error_code);
  }
}

void Serial_live()
{
  if(Serial.available() > 0)
  {
    String recebido = leStringSerial();
    msg_return = sendData(recebido,1000,DEBUG);
    Serial.print("[return]: ");Serial.println(msg_return);
  }
}
String Recive_Subscrible_Loop()
{
  String SCute = "";
  if(swSer.available() > 0)
  {
    String recebido = leStringSerialA9G();
//    msg_return = sendData(recebido,1000,DEBUG);
    int valie = callback_A9G(recebido);
    Serial.print("[Posicao de Rele]: ");Serial.println(valie); //32
    SCute = recebido.substring(valie, (valie + 8));
    Serial.print("[Nova String]: ");Serial.println(SCute);
  }
  return SCute;
}

String sendData(String command, const int timeout, boolean debug)
{
    String response = "";    
    swSer.println(command); 
    long int time = millis();   
    while( (time+timeout) > millis())
    {
      while(swSer.available())
      {       
        char c = swSer.read(); 
        response+=c;
      }  
    }    
    if(debug)
    {
      Serial.print(response);
    }    
    return response;
}

int A9GPOWERON()
{
      digitalWrite(A9G_PON, LOW);
      delay(3000);
      digitalWrite(A9G_PON, HIGH);
      delay(5000);
      String msg = String("");
      msg=sendData("AT",1000,DEBUG);
      if( msg.indexOf("OK") >= 0 ){
          Serial.println("GET OK");
          return 1;
      }
      else {
         Serial.println("NOT GET OK");
         return 0;
      }
}

int A9GPOWEROFF()
{
      digitalWrite(A9G_POFF, HIGH);
      delay(3000);
      digitalWrite(A9G_POFF, LOW);
      delay(5000);
      String msg = String("");
      msg=sendData("AT",1000,DEBUG);
      if( msg.indexOf("OK") >= 0 ){
          Serial.println("GET OK");
          return 1;
      }
      else {
          Serial.println("NOT GET OK");
          return 0;
      }
}

int A9GMQTTCONNECT(String command)
{
  int return_msg;
  msg_return = sendData(command,1000,0);

  delay(7000); 
  if(trataMsg(msg_return) == 1)
  {
    return_msg = 1;
  } 
  else if(trataMsg(msg_return) == 0)
  {
    return_msg = 0;
  }
  return return_msg;
}

String leStringSerial(){
  String conteudo = "";
  char caractere;
  while(Serial.available() > 0) 
  {
    caractere = Serial.read();
      if (caractere != '\n'){
        conteudo.concat(caractere);
      }
    delay(10);
  }
  Serial.print("Recebi: ");
  Serial.println(conteudo);
  return conteudo;
}

String leStringSerialA9G(){
  String conteudo = "";
  char caractere;
  while(swSer.available() > 0) 
  {
    caractere = swSer.read();
      if (caractere != '\n'){
        conteudo.concat(caractere);
      }
    delay(10);
  }
  Serial.print("Recebi: ");
  Serial.println(conteudo);
  return conteudo;
}

int callback_A9G(String msg)
{
  int return_msg, pos;
//  Serial.print("Callback MSG: ");Serial.println(msg);
  pos = msg.indexOf("RELE");
  return pos;
}

int trataMsg(String msg)
{
  int return_msg;
  Serial.print("msg: ");Serial.println(msg);
  int pos1, pos2;
  pos1 = msg.indexOf("ERR"); // Se encontrar um erro retorna a posicao
  if(pos1 < 0)
  {
    // Nao retornou posição, portanto Nao ha erro (pos1 = -1)
    Serial.println("Nao foi encontrado ERRO");
    count_erros = 0;
    return_msg = 1;
  }
  else
  {
    // Encontrou uma posição, há um erro (pos1 > 0)
    count_erros++;
    Serial.println("Foi encontado ERROR ");
    int Error_valid = msg.indexOf("ERROR");
    Error_code = msg.substring(Error_valid, (Error_valid + 10));
    Serial.print("Contagem de Erro: ");Serial.println(count_erros);
    
    if(count_erros == max_tent_error)
    {
      Serial.println("Reiniciando Modulo...");
      A9GPOWEROFF();  // Desliga o Modulo
      delay(1000);
      Serial.println("Inicializando Modulo...");
      Serial.println("After 2s A9G will POWER ON.");
      delay(2000);
      if(A9GPOWERON()==1)
      {
           Serial.println("A9G POWER ON.");
      }
      AT_Inicializa_A9G();
      
    }
    
    return_msg = 0;
  }
  return return_msg;
}
