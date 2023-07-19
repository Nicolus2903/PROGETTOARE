#include <LiquidCrystal_I2C.h>
#include <virtuabotixRTC.h>
#include <SoftwareSerial.h>
#include <DHT.h>
#include <string.h>
#include <stdlib.h>

#define LED 53
#define SENSORE_FIAMMA A1

#define HELPADMIN "Comandi Disponibili:\n - Temperatura\n - Umidita\n - Data e Ora\n - AGnumber\n - RInumber\n\nI comandi AGnumber e RInumber servono a aggiungere o rimuovere un'utente dalla lista degli utenti autorizzati.\nSostituire number con il numero della persona da aggiungere/rimuovere preceduto dal prefisso (+39)\n"
#define HELPUSER "Comandi Disponibili:\n - Temperatura\n - Umidita\n - Data e Ora\n"

SoftwareSerial sim(10, 11);
DHT dht(7, DHT11);
virtuabotixRTC myRTC(23,24,25);
LiquidCrystal_I2C lcd(0x27, 16, 2);

int _timeout;
String _buffer;
String admin = "+393426361274"; //-> change with your number
String message="";
char readedChar;
int temperature;
int umidity;
bool isHigh=false;
unsigned long startTime;
unsigned long elapsedTime;
unsigned long countdownDuration = 20000;
bool isAuthorized=false;

int numbers=0;
String* authorizedUsers=NULL;
int startIndex;
int endIndex;
String phoneNumber;
String newphoneNumber;
void setup() {
  //delay(7000); //delay for 7 seconds to make sure the modules get the signal
  Serial.begin(9600);
  _buffer.reserve(50);
  Serial.println("Sistem Started...");
  sim.begin(9600);
  dht.begin();
  delay(1000);
  Serial.println("PROGETTO ARE - SERVER SMS ARDUINO. MADE BY NICOLA FIGUS");
  pinMode(SENSORE_FIAMMA,INPUT);
  pinMode(LED,OUTPUT);
  lcd.begin();
  lcd.backlight();
}



void loop() {

  myRTC.updateTime();
  
  stampaNumeriAutorizzati(authorizedUsers);
  if(isHigh==true){
    elapsedTime = millis() - startTime;
    if(elapsedTime>countdownDuration){
      checkFlameSensor();
    }
  }else{
    checkFlameSensor();
  }

  manageLEDDISPLAY();

  RecieveMessage();

  while(sim.available() > 0){
    readedChar= sim.read();
    //Serial.write(readedChar);
    if(readedChar!='\n' && readedChar !='\0'){
      message = message+readedChar;
    }
    
  }

  Serial.println("Messaggio ricevuto: "+message);


  startIndex = message.indexOf("\"") + 1; // Trova l'indice del primo carattere dopo il primo apice
  endIndex = message.indexOf("\"", startIndex); // Trova l'indice del secondo apice
  phoneNumber = message.substring(startIndex, endIndex); // Estrae la sottostringa contenente il numero di telefono
  Serial.println("Numero di telefono: " + phoneNumber+(phoneNumber.length()));

  if(phoneNumber==admin){
    if(message.indexOf("AG")!=(-1)){
      newphoneNumber = message.substring((message.indexOf("AG")+2)); // Estrae la sottostringa che segue "Aggiungi-"
      newphoneNumber="+39"+newphoneNumber;
      Serial.println("dnasdas: "+newphoneNumber+(newphoneNumber.length()));
      AggiungiNumeroAutorizzato(&authorizedUsers,newphoneNumber);
    }else if(message.indexOf("RI")!=(-1)){
      newphoneNumber = message.substring((message.indexOf("RI")+2)); // Estrae la sottostringa che segue "Aggiungi-"
      newphoneNumber="+39"+newphoneNumber;
      Serial.println("dnasdas: "+newphoneNumber+(newphoneNumber.length()));
      RimuoviNumeroAutorizzato(&authorizedUsers,newphoneNumber);
    }else if(message.indexOf("HELPAdmin")!=(-1)){
      SendMessage(HELPADMIN,admin);
    }
  }

  if(cercaNumero(&authorizedUsers,phoneNumber)==true){
    isAuthorized=true;
  }else{
    isAuthorized=false;
  }

  if(isAuthorized==true){
    if(message.indexOf("Temperatura")!=(-1)){
      temperature = dht.readTemperature();
      SendMessage("Temperatura: "+String(temperature)+" C",phoneNumber);
    }else if(message.indexOf("Umidita")!=(-1)){
      umidity = dht.readHumidity();
      SendMessage("Umidita: "+String(umidity)+"%",phoneNumber);
    }else if(message.indexOf("Data e Ora")!=(-1)){
      SendMessage("Data: "+String(myRTC.dayofmonth)+"/"+String(myRTC.month)+"/"+String(myRTC.year)+"  Ora: "+String(myRTC.hours)+":"+String(myRTC.minutes)+":"+String(myRTC.seconds),phoneNumber);
    }else if(message.indexOf("HELP")!=(-1)){
      SendMessage(HELPUSER,phoneNumber);
    }else{
      Serial.println("messaggio non riconosciuto da numero autorizzato.");
    }
    delay(1000);
    isAuthorized=false;
    phoneNumber="";
    newphoneNumber="";
  }else{
    Serial.println("Un utente non autorizzato ha inviato un messaggio\n");
  }

  
  
  message="";
  delay(3000);
  lcd.clear();
}

void SendMessage(String message,String number){
  //Serial.println ("Sending Message");
  sim.println("AT+CMGF=1"); //Sets the GSM Module in Text Mode
  delay(200);
  //Serial.println ("Set SMS Number");
  sim.println("AT+CMGS=\"" + number + "\"\r"); //Mobile phone number to send message
  delay(200);
  sim.println(message);
  delay(100);
  sim.println((char)26);// ASCII code of CTRL+Z
  delay(200);
  _buffer = _readSerial();
}

void RecieveMessage(){
  Serial.println ("In ricezione..");
  sim.println("AT+CMGF=1");
  delay (200);
  sim.println("AT+CNMI=2,2,0,0,0"); // AT Command to receive a live SMS
  delay(200);
}

String _readSerial() {
  _timeout = 0;
  while  (!sim.available() && _timeout < 12000  ){
    delay(13);
    _timeout++;
  }
  if (sim.available()) {
    return sim.readString();
  }
}

void checkFlameSensor(){
  if(analogRead(SENSORE_FIAMMA)<=100){
    //digitalWrite(LED,HIGH);
    SendMessage("ATTENZIONE!\nRILEVATO INCENDIO! \n",admin);
    //lcd.print("ALLARME FUOCO!");
    if(isHigh==false){
      startTime = millis();
      isHigh=true;
    }else{
      isHigh=false;
    }
  }
}

void manageLEDDISPLAY(){
  if(analogRead(SENSORE_FIAMMA)<=100){
    digitalWrite(LED,HIGH);
    lcd.print("ALLARME FUOCO!");
  }else{
    digitalWrite(LED,LOW);
    lcd.print("NESSUN PERICOLO!");
  }
}

void AggiungiNumeroAutorizzato(String** array,String newNumber){
  if(numbers==0){
    authorizedUsers=(String*)calloc(numbers+1,sizeof(String));
    *(array[numbers])=newNumber;
    numbers++;
  }else{
    authorizedUsers=(String*)realloc(authorizedUsers,(numbers+1)*sizeof(String));
    *(array[numbers])=newNumber;
    numbers++;
  }
}

void RimuoviNumeroAutorizzato(String** array,String newNumber){
  int index=0;
  if(numbers>0){
    for(int i=0;i<numbers;i++){
      if(phoneNumber.equals(authorizedUsers[i])){
      index = i;
      break;
    }
    }
    if(numbers==1){
      free(&authorizedUsers);
      numbers--;
    }else{
      swap(&authorizedUsers[index],&authorizedUsers[numbers-1]);
      authorizedUsers = (String*) realloc(authorizedUsers,sizeof(String)*(numbers-1));
      numbers--;
    }
   
  }
}


bool cercaNumero(String** array,String newNumber){
  for(int i=0;i<numbers;i++){
    if(phoneNumber.equals(authorizedUsers[i])){
      return true;
    }
  }
  return false;
}

void swap(String* s1, String* s2){
  String temp = *s1;
  *s1=*s2;
  *s2=temp;
}

void stampaNumeriAutorizzati(String* array){
  Serial.println("\nSTAMPA NUMERI AUTORIZZATI:");
  for(int i=0;i<numbers;i++){
    Serial.println("N."+String(i+1)+": "+array[i]);
  }
  Serial.println("\n\n");
}

