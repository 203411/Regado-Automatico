#include <LiquidCrystal.h>
#include <DHT.h> //DHT11
#include <WiFi.h>
#include <esp_wifi.h>
#include <HTTPClient.h>

#define DHTPIN 4 //Sensor de humedad
#define DHTTYPE DHT11 
#define humPin 34
#define trigPin 12 //ultrasonico trig
#define echoPin 13  //ultrasonico echo
#define pinLDR 35 //fotoresistencia
#define pinLED 15 //LED para ver el funcionamiento del LDR
#define pinBomba 25


LiquidCrystal lcd(22,23,5,18,19,21);
DHT dht(DHTPIN, DHTTYPE);

//IP, MAC, mascara de red
IPAddress ip(192,168,89,200);
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional DNS
IPAddress secondaryDNS(8, 8, 4, 4); //optional

//Datos de la red
const char* ssid = "homemn2";
const char* passwd = "h0m3M3nd3z/22";
//const char* ssid = "Software";
//const char* passwd = "software22";

//Variables de tiempo
unsigned long time1, time2, t_blink1, t_blink0, t1_blink1, t1_blink0, tlcd;
int intervaloLecturas = 0, intervaloActualizacion = 0, blink1 = 0, blink0 = 0, lcdAct = 0, blink1_1 = 0, blink0_1 = 0;

//Variables
int duracion,distancia,luminosidad,humedad_suelo,estado_bomba,nivel_agua,porcentaje_humedad;
float humedad,temperatura;

byte aguaVacia[8] = {
    0b01010,
    0b11011,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b11111
};
byte customChar[8] = {
    0b01110,
    0b01010,
    0b01110,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000
};
byte porcentChar[] = {
    B01100,
    B01101,
    B00010,
    B00100,
    B01000,
    B10110,
    B00110,
    B00000
};
byte termometro[] = {
    B00100,
    B01010,
    B01010,
    B01010,
    B01110,
    B11111,
    B11111,
    B01110};

byte gota[] = {
    B00100,
    B00100,
    B01010,
    B01010,
    B10001,
    B10001,
    B10001,
    B01110
};
byte humSuelo[] = {
    B00011,
    B00010,
    B00001,
    B01011,
    B01000,
    B11100,
    B11100,
    B01100
};
byte aguaLlena[] = {
    B01010,
    B11011,
    B10001,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111
};

void setup() {
  Serial.begin(115200);
  lcd.begin(16, 2);  
  lcd.createChar(0, customChar);
  lcd.createChar(1, porcentChar);
  lcd.createChar(2, termometro);
  lcd.createChar(3, gota);
  lcd.createChar(4, humSuelo);
  lcd.createChar(5, aguaLlena);
  lcd.clear();
  dht.begin(); 
  pinMode(humPin, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(pinLDR, INPUT);
  pinMode(pinLED, OUTPUT);
  pinMode(pinBomba, OUTPUT);
  estado_bomba = 0;
  conect_to_wifi(); 
}

void loop() {
  temperatura = dht.readTemperature(); //temperatura DHT11
  humedad = dht.readHumidity(); //humedad DHT11
  humedad_suelo = analogRead(humPin); //humedad de suelo
  distancia = obtener_distancia(); //Sensor ultrasonico
  luminosidad = analogRead(pinLDR);
  nivel_agua = ((12.0 - distancia) * 100.0) / 10.0;
  porcentaje_humedad = ((4095 - (humedad_suelo + 500))*100)/2000;
  if(porcentaje_humedad >= 100){
    porcentaje_humedad = 100;
  }
  if(porcentaje_humedad < 0){
    porcentaje_humedad = 0;
  }
  if(nivel_agua > 100 || distancia <= 2){
    nivel_agua = 100;
  }
  if(nivel_agua <= 10){
    nivel_agua = 0;
  }
  Serial.println("Temperatura: " + String(temperatura) + "°C " + "Humedad: " + String(humedad) + "%");
  Serial.println("Humedad de suelo: " + String(humedad_suelo) + " Nivel de agua: " + String(nivel_agua) + "%");
  Serial.println("Luminosidad :" +String(luminosidad) + " Estado bomba: " + String(estado_bomba));
  Serial.println("Humedad de suelo%: "+String(porcentaje_humedad));
  
  if(luminosidad > 1500){
    digitalWrite(pinLED,HIGH);     
  }else{
    digitalWrite(pinLED, LOW);
  }
  mostrarDHT11();
  mostrarHumedadSuelo();
  mostrarNivelAgua();
  regar();
  

  //conect_to_wifi(); //funcion para conectarse a wifi
//    if (time1 - intervaloLecturas >= 1500){
//      intervaloLecturas = time1;
//      enviar_datos();
//    }
//  enviar_datos();
//  WiFi.disconnect(true);

  delay(5000);
  lcd.clear();
}

void conect_to_wifi(){
  WiFi.mode(WIFI_STA);
  if(!WiFi.config(ip, gateway, subnet,primaryDNS,secondaryDNS)){ //Configuracion
    Serial.println("Fallo al configurar el wifi");
  }
  Serial.print("[OLD] ESP32 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  uint8_t newMACAddress[] = {0xD4, 0x4D, 0xA4, 0xD0, 0xA3, 0xC4};
  esp_wifi_set_mac(WIFI_IF_STA, &newMACAddress[0]);
  WiFi.begin(ssid, passwd);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }  
  Serial.print("Conectado con éxito, mi IP es: ");
  Serial.println(WiFi.localIP());
  Serial.print(WiFi.macAddress());
}

void regar(){     
   if (humedad_suelo > 4000 && nivel_agua > 25 && temperatura < 37){ 
    lcd.setCursor(8,1);
    lcd.print("REGANDO");
    digitalWrite(pinBomba,HIGH);
    estado_bomba = 1;
    //enviar_datos();
    delay(500);
    digitalWrite(pinBomba,LOW);
    estado_bomba = 0;
    lcd.clear();
   }else{
      digitalWrite(pinBomba,LOW);
   }
}


int obtener_distancia(){
  digitalWrite(trigPin, HIGH); 
  delayMicroseconds(1000);
  digitalWrite(trigPin, LOW);
  duracion = pulseIn(echoPin, HIGH);
  int dist = (duracion/2)/29.1;//distancia en cm - ultrasonico
  return dist;
}

void mostrarHumedadSuelo(){
  lcd.setCursor(0, 1);
  lcd.write(byte(4));
  lcd.print(porcentaje_humedad);
  lcd.write(byte(1));
}

void mostrarDHT11(){
  // mostrar temperatura ambiental
  lcd.setCursor(0, 0);
  lcd.write(byte(2));
  lcd.print(temperatura);
  lcd.write(byte(0));
  lcd.print("C");
  // mostrar humedad ambiental
  lcd.setCursor(9, 0);
  lcd.write(byte(3));
  lcd.print(humedad);
  lcd.write(byte(1));
}

void mostrarNivelAgua(){
  // mostrar nivel de agua
  lcd.setCursor(8, 1);
  lcd.write(byte(5));
  lcd.print(nivel_agua);
  lcd.write(byte(1));
}

void enviar_datos(){
   if (WiFi.status() == WL_CONNECTED){
    
    HTTPClient http;

    String code = "123456";
    
    datos = String(temperatura) + ","+ String(humedad) + "," + String(humedad_suelo)+ ","+String(nivel_agua) + ","+String(luminosidad) + ","+ String(estado_bomba);
    String json;
    StaticJsonDocument<300> doc;
    doc["data"]=datos;
    doc["code"]=code;
    serializeJson(doc, json);
    Serial.println(json);
    //Indicamos el destino
    http.begin("http://192.168.89.194:8000/api/plant");
    http.addHeader("Content-Type", "application/json");
    
    int status_response = http.POST(json);
    
    if (status_response > 0){
      Serial.println("Código HTTP ► " + String(status_response)); // Print return code

      if (status_response == 200){
        String data_response= http.getString();
        Serial.println("El servidor respondió ▼ ");
        Serial.println(data_response);
      }
    }else{
      String data_response= http.getString();
      Serial.print(" Error enviando POST, código: ");
      Serial.println(status_response);
      Serial.println(data_response);
    }
    http.end(); 
  }
}
