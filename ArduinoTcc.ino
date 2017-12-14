
#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <Wire.h>
#include <math.h>


SoftwareSerial portOneGps(4, 3); // RX, TX
SoftwareSerial portTwoGprs(9, 8);  // RX, TX Pins
TinyGPS gps1;


int count = 0;
float latitudeOld;// = -16.828821; //-16.671333;  //-16.671333;//casa -16.828821 // Huapa -16.816534 // distancia casa / huapa = 1,41km
float logitudeOld;// = -49.281757; //-49.238678;  //-49.238678;//casa -49.281757 // Huapa -49.278120 //-16.671333, -49.238678

long latitude, longitude;

//unsigned char Buff[50];

void setup() {
  pinMode (13,OUTPUT);
  Serial.begin(9600);
  portOneGps.begin(9600);
  portTwoGprs.begin(9600);
  delay(5000);
  Serial.println("Conectando...");
  connect();
  Serial.println("Le GSM...");
  leGSM();
  portOneGps.listen();

}
// sera que aquelas da apn nao pde passar direto? Acho que sim

void loop() {
  bool recebido = false;
  unsigned long idadeInfo;
  while (portOneGps.available()) {
    char cIn = portOneGps.read();
    Serial.print(cIn);
    recebido = gps1.encode(cIn);
  }

  if (recebido) {
    Serial.println("---------------------------------------");
    //Latitude e Longitude    
    gps1.get_position(&latitude, &longitude, &idadeInfo);

    char sLat[11];
    if (latitude != TinyGPS::GPS_INVALID_F_ANGLE) {
      Serial.print("Latitude: ");
      dtostrf((float(latitude) / 100000), 2, 6,sLat);
      Serial.println(float(latitude)/ 100000, 6 );
    }
    char sLng[11];
    if (longitude != TinyGPS::GPS_INVALID_F_ANGLE) {
      Serial.print("Longitude: ");
      dtostrf((float(longitude) / 100000), 2, 6,sLng);
      Serial.println(float(longitude) / 100000, 6);
    }
    
    if (idadeInfo != TinyGPS::GPS_INVALID_AGE) {
      Serial.print("Idade da Informacao (ms): ");
      Serial.println(idadeInfo);
    }

    Serial.println(sLat);
    Serial.println(-16.671671);
    Serial.println(sLng);
    Serial.println(-49.238697);
    
    float distancia_entre = gps1.distance_between(atof(sLat), atof(sLng), latitudeOld, logitudeOld);
    Serial.print("distancia entre: ");
    Serial.println(distancia_entre);   
    count++;    
   
    if (count >= 1) {
      if (distancia_entre>1) { 
        digitalWrite(13,HIGH);
        if (count > 1) {
          count = 0;
          portTwoGprs.listen();
          char path[80];
          memset(path, '\0', 80);
         
          //sprintf(path, "add?id_dispositivo=%d&longitude=%d&latitude=%d%s\0", 1, longitude, latitude, currentdate);
          sprintf(path, "add?id_dispositivos=%d&longitude=%ld&latitude=%ld", 1, longitude, latitude);
          gsm_sendhttp(path, false); // apenas envia dados para o servidor.. nao precisa receber resposta
          sendsms();           
         }
        }else{
         digitalWrite(13,LOW);
       }
     }
     delay(3000);
   }
}


void connect() {
  portTwoGprs.print("AT+CMGF=1\n;AT+CNMI=2,2,0,0,0\n;ATX4\n;AT+COLP=1\n");
  
  portTwoGprs.println("AT+CGATT=0");
  runsl();
  delay(5000);

  portTwoGprs.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
  runsl();
  delay(5000);

  portTwoGprs.println("AT+SAPBR=3,1,\"APN\",\"3g.tim.br\"");
  runsl();
  delay(5000);

  portTwoGprs.println("AT+SAPBR=3,1,\"USER\",\"tim\""); //Comment out, if you need username
  runsl();
  delay(5000);

  portTwoGprs.println("AT+SAPBR=3,1,\"PWD\",\"tim\""); //Comment out, if you need password
  runsl();
  delay(5000);

  portTwoGprs.println("AT+SAPBR=1,1");
  runsl();
  delay(5000);

  portTwoGprs.println("AT+SAPBR=2,1");
  runsl();
  delay(5000);

  portTwoGprs.println("AT+CGATT=1"); //Comment out, if you need password
  runsl();
  delay(5000);
}

bool gsm_sendhttp(const char *path, bool getResponse) {
  portTwoGprs.println("AT+HTTPINIT=?");
  runsl();
  delay(2000);
  portTwoGprs.println("AT+HTTPINIT");
  runsl();
  delay(2000);

  char url[150];
  memset(url, '\0', 150);
  
  sprintf(url, "AT+HTTPPARA=\"URL\",\"http://ghastly-vampire-21887.herokuapp.com/cliente/%s\"\0", path);  
  Serial.println(url);
   
  //portTwoGprs.println("AT+HTTPPARA=\"URL\",\"http://www.mocky.io/v2/5a2b2bd82d0000213491b2d7\""); 
  delay(5000);
  
  portTwoGprs.println(url);
  delay(7000);
  runsl();
  delay(7000);

  portTwoGprs.println("AT+HTTPPARA=\"CID\",1");
  delay(5000);
  runsl();
  delay(5000);

  portTwoGprs.println("AT+HTTPACTION=0");
  delay(5000);
  runsl();  

  portTwoGprs.println("AT+HTTPREAD");  
  delay(10000);
  int tt = 61;

  char slng_aux[15];
  char slat_aux[15];
  memset(slng_aux, '\0', 15);
  memset(slat_aux, '\0', 15);
  int i=0;
  int m=0;
  int l = 0;
  bool got = false;

  if(getResponse) {
    while (portTwoGprs.available()) {
      int c = portTwoGprs.read();
      if(m>34 && m<45){
        slng_aux[i] = (char)c;
        i++;
      }
      if(m>50 && m<61){
        slat_aux[l] = (char)c;
        l++;
        got = true; // Conseguiu ler a resposta do servidor
      }
      m++;
    }
    slng_aux[i] = '\0';
    slat_aux[l] = '\0';

    latitudeOld = atof(slat_aux);
    logitudeOld = atof(slng_aux);
    Serial.println(logitudeOld, 6);
    Serial.println(latitudeOld, 6);
  } else { // Se nao tiver que receber resposta do servidor pode alterar para a porta One, por isso got=true
    got = true;
  }
  portTwoGprs.println("AT+HTTPTERM");
  runsl();
  delay(2000);
  
  Serial.println("\n-------------------------------------------------------------------\n");
  if(got) {
    portOneGps.listen(); // Se deu certo a comunicacao http, ativa a portaOne.
  }
  return got;
}

void runsl() {
  while (portTwoGprs.available()) {
    Serial.write(portTwoGprs.read());    
  }
}

void sendsms(){
  portTwoGprs.listen();
  
  Serial.println("Enviando SMS, um momento...");
   
  portTwoGprs.write("AT+CMGF=1\n");
  delay(2000);
 
  portTwoGprs.write("AT+CMGS=\"985174670\"\n");
  delay(2000);
   
  portTwoGprs.write("Alerta! Ativo fora do perimetro estabelecido");
  delay(2000);
   
  portTwoGprs.write((char)26);
  delay(2000);
     
  Serial.println("Feito");
  portOneGps.listen();
  
}
void leGSM(){
  unsigned char buff[120];
  memset(buff, '\0', 120);
  Serial.println("Aguardando senha");
  int printou = 1;
  unsigned char BuffIndex = 5;
  while(1){// mandei, mas ele ja estava no lendo senha
    if(portTwoGprs.available()>0){
      int x = portTwoGprs.read();
      buff[BuffIndex] = x;
      
      if(printou==1) {
        Serial.println("Lendo senha");
        printou = 0;
      }

      //Serial.write(buff[BuffIndex-1]);
      //Serial.write(buff[BuffIndex]);
      //Serial.println("");
      
      if( buff[BuffIndex-1] == 'C' && buff[BuffIndex] == '4') {               
         Serial.println("Executando comando");
         char path[50];
         memset(path, '\0', 50);
         sprintf(path, "listarId?id_dispositivo=4");

         while(!gsm_sendhttp(path, true)){
           Serial.println("==============");
           delay(1000); 
         }
         
         delay(5000);
         break;
      }
      BuffIndex++;
    }
  }
  Serial.println("aki 6");
}
