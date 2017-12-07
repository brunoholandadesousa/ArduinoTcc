#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <Wire.h>
#include <math.h>


SoftwareSerial portOneGps(4, 3); // RX, TX
SoftwareSerial portTwoGprs(9, 8);  // RX, TX Pins
TinyGPS gps1;

String apn = "3g.tim.br";                       //APN
String apn_u = "tim";                     //APN-Username
String apn_p = "tim";                     //APN-Password
String url = "http://url.com/index.php";  //URL for HTTP-POST-REQUEST
int count = 0;
const float latitudeOld = -16.669605;//casa -16.828821 // Huapa -16.816534 // distancia casa / huapa = 1,41km
const float logitudeOld = -49.240081;//casa -49.281757 // Huapa -49.278120
long latitude, longitude;
float rad, a1, a2, b1, b2, dlon, dlat, a, c, R, d;
unsigned long idadeInfo;
unsigned char Buff[250];
unsigned char BuffIndex;


void setup() {

  Serial.begin(9600);
  portOneGps.begin(9600);
  portTwoGprs.begin(9600);
  delay(5000);
  Serial.println("Conectando...");
//  connect();
  //leGSM();
  portOneGps.listen();

}

void loop() {
 
  bool recebido = false;
  long xxx = 0;
  while (portOneGps.available()) {
    char cIn = portOneGps.read();
    xxx = xxx + 1;
    Serial.print(cIn);
    recebido = gps1.encode(cIn);
  }

  if (recebido) {
    Serial.print("----------------------------------------");
    Serial.print(xxx);
    Serial.println("");
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


    //Dia e Hora
    int ano;
    byte mes, dia, hora, minuto, segundo, centesimo;
    gps1.crack_datetime(&ano, &mes, &dia, &hora, &minuto, &segundo, &centesimo, &idadeInfo);

    Serial.print("Data (GMT): ");
    Serial.print(dia);
    Serial.print("/");
    Serial.print(mes);
    Serial.print("/");
    Serial.println(ano);

    Serial.print("Horario (GMT): ");
    Serial.print(hora-3);
    Serial.print(":");
    Serial.print(minuto);
    Serial.print(":");
    Serial.print(segundo);
    Serial.print(":");
    Serial.println(centesimo);   

    //satelites e precisão
    unsigned short satelites;
    unsigned long precisao;
    satelites = gps1.satellites();
    precisao =  gps1.hdop();

    if (satelites != TinyGPS::GPS_INVALID_SATELLITES) {
      Serial.print("Satelites: ");
      Serial.println(satelites);
    }

    if (precisao != TinyGPS::GPS_INVALID_HDOP) {
      Serial.print("Precisao (centesimos de segundo): ");
      Serial.println(precisao);
    } 

    Serial.println(sLat);
    Serial.println(-16.671671);
    Serial.println(sLng);
    Serial.println(-49.238697);
    
    float latitude1 = latitude;
    float longitude1 =  longitude;

    rad = PI/180;
    a1 = latitude1 * rad;
    a2 = longitude1 * rad;
    b1 = latitudeOld * rad;
    b2 = logitudeOld * rad;
    dlon = b2 - a2;
    dlat = b1 - a1;       
    a = ((sin(dlat / 2))*(sin(dlat / 2)))+ cos(a1) * cos(b1) * ((sin(dlon / 2)) * (sin(dlon / 2)));
    c = 2 * atan2(sqrt(a), sqrt(1 - a));
    R = 6378.145;
    d = R * c;
    float distancia_entre = gps1.distance_between(atof(sLat), atof(sLng), latitudeOld, logitudeOld);
    Serial.print("distancia entre: ");
    Serial.println(d);   
    Serial.print("distancia entre: ");
    Serial.println(distancia_entre);   
    count++;    
    if (count >= 5) {
      if (d>0.5) {      
        if (count > 6) {
          count = 0;
          portTwoGprs.listen();
          gsm_sendhttp(latitude, longitude);
          //char str[90];
          //sprintf(str, "cliente/add?lat=%f&long=%f&moment=%d-%d-%d--%d-%d%d", latitude, longitude, ano, mes, dia, hora, minuto, segundo );           
         // gsm_sendhttp(String(str));
          sendsms();           
        }
      }
    }

    delay(3000);
  }
}


void connect() {
  portTwoGprs.print("AT+CMGF=1\n;AT+CNMI=2,2,0,0,0\n;ATX4\n;AT+COLP=1\n");
  memset(Buff, '\0', 250);    // Initialize the string
    BuffIndex=5;
  portTwoGprs.println("AT+CGATT=0");
  runsl();
  delay(5000);

  portTwoGprs.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
  runsl();
  delay(5000);

  portTwoGprs.println("AT+SAPBR=3,1,\"APN\",\"" + apn + "\"");
  runsl();
  delay(5000);

  portTwoGprs.println("AT+SAPBR=3,1,\"USER\",\"" + apn_u + "\""); //Comment out, if you need username
  runsl();
  delay(5000);

  portTwoGprs.println("AT+SAPBR=3,1,\"PWD\",\"" + apn_p + "\""); //Comment out, if you need password
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
void gsm_sendhttp(long lat, long lng) {
//String path
//  Serial.println(path);
  portTwoGprs.println("AT+HTTPINIT=?");
  runsl();
  delay(2000);
  portTwoGprs.println("AT+HTTPINIT");
  runsl();
  delay(2000);
  
  portTwoGprs.println("AT+HTTPPARA=\"URL\",\"http://www.mocky.io/v2/59bb244e0f00003d07622a7e\"");
  delay(2000);
  runsl();
  delay(2000);


  portTwoGprs.println("AT+HTTPPARA=\"CID\",1");
  delay(2000);
  runsl();
  delay(2000);

  portTwoGprs.println("AT+HTTPACTION=0");
  delay(2000);
  runsl();


  portTwoGprs.println("AT+HTTPREAD");
  delay(10000);
  runsl();
  delay(10000);

  portTwoGprs.println("AT+HTTPTERM");
  runsl();
  delay(2000);

  Serial.println("\n-------------------------------------------------------------------\n");
  portOneGps.listen();
}

//Print GSM Status
void runsl() {
  while (portTwoGprs.available()) {
    Serial.write(portTwoGprs.read());
  }

}

void sendsms(){
  portTwoGprs.listen();
  
  Serial.println("Enviando SMS, um momento...");
   
  portTwoGprs.write("AT+CMGF=1\r\n");
  delay(1000);
 
  portTwoGprs.write("AT+CMGS=\"985174670\"\r\n");
  delay(1000);
   
  portTwoGprs.write("Alerta! Ativo fora do perimetro estabelecido");
  delay(1000);
   
  portTwoGprs.write((char)26);
  delay(1000);
     
  Serial.println("Feito");
  portOneGps.listen();
  
}
void leGSM(){
  Serial.println("aki 1");
  while(1){
    Serial.println("Aguardando senha");
    if(portTwoGprs.available()>0){
      Buff[BuffIndex] = portTwoGprs.read();    
      Serial.println("Lendo senha");
      Serial.println(Buff[BuffIndex-2]);
      Serial.println(Buff[BuffIndex-1]);
      Serial.println(Buff[BuffIndex]);
      
      if( (Buff[BuffIndex-1] == 'C') && (Buff[BuffIndex] == '4')){               
         Serial.println("Executando comando");
         //char str[90];
         //sprintf(str, "cliente/listarId?id_dispositivo=3" );           
         //gsm_sendhttp(String(str));
         delay(10000);
         break;
      }         
      Serial.println("aki 4");
      BuffIndex++;
      if(BuffIndex>250){
        BuffIndex=5;
        Serial.println("aki 5");
      }
    }
  }
  Serial.println("aki 6");
}
