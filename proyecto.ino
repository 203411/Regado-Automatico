#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <HTTPClient.h>
#include <DHT.h> //DHT11

#define DHTPIN 5 //Sensor de humedad
#define DHTTYPE DHT11 
#define trigPin 32 //ultrasonico trig
#define echoPin 13  //ultrasonico echo

WiFiServer server(80);
uint8_t newMACAddress[] = {0xc8, 0x3d, 0xd4, 0x7a, 0x57, 0x85};

//Variables de tiempo
unsigned long time1, time2, t_blink1, t_blink0, t1_blink1, t1_blink0, tlcd;
int intervaloLecturas = 0, intervaloActualizacion = 0, blink1 = 0, blink0 = 0, lcdAct = 0, blink1_1 = 0, blink0_1 = 0;

const int pinLDR = 35; //luminosidad
const int pinBomba = 25; //mini bomba de agua

//IP, MAC, mascara de red
IPAddress ip(192,168,0,200);
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional DNS
IPAddress secondaryDNS(8, 8, 4, 4); //optional

//Datos de la red
const char* ssid = "homemn2";
const char* passwd = "h0m3M3nd3z/22";

const int lcdColumns = 16;
const int lcdRows = 2;
//Pines de LCD D21 y D22

byte aguaVacia[8] = {
    0b01010,
    0b11011,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b10001,
    0b11111};

byte customChar[8] = {
    0b01110,
    0b01010,
    0b01110,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000};

byte porcentChar[] = {
    B01100,
    B01101,
    B00010,
    B00100,
    B01000,
    B10110,
    B00110,
    B00000};

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
    B01110};

byte humSuelo[] = {
    B00011,
    B00010,
    B00001,
    B01011,
    B01000,
    B11100,
    B11100,
    B01100};

byte aguaLlena[] = {
    B01010,
    B11011,
    B10001,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111};

byte aguaMedioLlena[] = {
    B01010,
    B11011,
    B10001,
    B10001,
    B11111,
    B11111,
    B11111,
    B11111};

byte aguaMitad[] = {
    B01010,
    B11011,
    B10001,
    B10001,
    B10001,
    B11111,
    B11111,
    B11111};

byte aguaBaja[] = {
    B01010,
    B11011,
    B10001,
    B10001,
    B10001,
    B10001,
    B11111,
    B11111};

int duracion,distancia,luminosidad,humedad_suelo,estado_bomba,nivel_agua;
float humedad,temperatura;

LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  
  conect_to_wifi(); //funcion para conectarse a wifi
  
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(pinBomba, OUTPUT);
  pinMode(pinLDR, INPUT);
  pinMode(34, INPUT); //entrada de sensor de humedad de suelo

  lcd.createChar(0, customChar);
  lcd.createChar(1, porcentChar);
  lcd.createChar(2, termometro);
  lcd.createChar(3, gota);
  lcd.createChar(4, humSuelo);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  
  dht.begin();  

  digitalWrite(pinBomba, LOW);
  estado_bomba = 0;
  
}

void loop() {
  //Tiempos
  time1 = millis(); // cada 15 segundos actualizar datos
  time2 = millis();
  t_blink1 = millis();
  t_blink0 = millis();
  t1_blink1 = millis();
  t1_blink0 = millis();
  tlcd = millis();

  if(time2 - intervaloActualizacion >= 2000){
    distancia = obtener_distancia(); //Sensor ultrasonico
    luminosidad = analogRead(pinLDR); //Fotoresistencia
    temperatura = dht.readTemperature(); //temperatura DHT11
    humedad = dht.readHumidity(); //humedad DHT11
    float hic = dht.computeHeatIndex(temperatura, humedad, false);  // Calcular el índice de calor en grados centígrados
    humedad_suelo = analogRead(34); //humedad de suelo
    
    nivel_agua = ((distancia-11)*100)/11;
    //int porcentaje_humedad = ((4095 - humedad_suelo)*100)/2300;

    Serial.println("Temperatura: " + String(temperatura) + "°C " + "Humedad: " + String(humedad) + "%");
    Serial.println("Humedad de suelo: " + String(humedad_suelo) + "% Nivel de agua: " + String(nivel_agua) + "%");
    Serial.println("Distancia:" + String(distancia) + "cm Estado bomba: " + String(estado_bomba) + "\n");

    if(estado_bomba == 0){
      mostrarDHT11();
      mostrarHumedadSuelo();
      mostrarNivelAgua();
      if (humedad_suelo <= 500 && nivel_agua >= 15){
        estado_bomba = 1;
        lcd.clear();
      }
    }else{
      regar();  
    }
    if (time1 - intervaloLecturas >= 15000){
    // Serial.println("Tiempo:" + String(time1));
    // Serial.println("duracionInvervalo:" + String(time1 - intervaloLecturas));
    intervaloLecturas = time1;
  }

  }
}



void imprimir_lcd(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(temperatura);
  lcd.setCursor(8,0);
  lcd.print(humedad);
  lcd.setCursor(0,1);
  lcd.print(nivel_agua);
  lcd.setCursor(5,1);
  lcd.print(luminosidad);
}

void regar(){
  lcd.setCursor(8,1);
  Serial.println("Desactivado");
  digitalWrite(pinBomba, HIGH);
  lcd.print("R e g a n d o");
  mostrarHumedadSuelo();
  mostrarNivelAgua();
  if (humedad_suelo >= 50 || nivel_agua <= 0)  {
    digitalWrite(pinBomba, LOW);
    Serial.println("Desactivado");
    estado_bomba = 0;
  }
}

void mostrarDHT11()
{
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
void mostrarHumedadSuelo()
{
  // mostrar humedad de suelo
  if (humedad_suelo <= 0.0)
  {
    if (t1_blink1 - blink1_1 >= 750)
    {
      blink1_1 = t1_blink1;
      lcd.setCursor(0, 1);
      lcd.write(byte(4));
      lcd.print(humedad_suelo);
      lcd.write(byte(1));
      lcd.setCursor(6, 1);
      lcd.print("   ");
    }
    /* if (t1_blink0 - blink0_1 >= 1500)
    {
      blink0_1 = t1_blink0;
      lcd.setCursor(0, 1);
      lcd.print("       ");
    } */
  }
  else
  {
    lcd.setCursor(0, 1);
    lcd.write(byte(4));
    lcd.print(humedad_suelo);
    lcd.write(byte(1));
    if (humedad_suelo < 10.0)
    {
      lcd.setCursor(6, 1);
      lcd.print(" ");
      lcd.setCursor(7, 1);
      lcd.print(" ");
    }
    lcd.setCursor(7, 1);
    lcd.print(" ");
    if (humedad_suelo == 100.0)
    {
      lcd.setCursor(7, 1);
      lcd.write(byte(1));
    }
  }
}
void mostrarNivelAgua()
{
  // mostrar nivel de agua
  if (nivel_agua <= 0.0)
  {
    if (t_blink1 - blink1 >= 750)
    {
      blink1 = t_blink1;
      lcd.createChar(5, aguaVacia);
      lcd.setCursor(9, 1);
      lcd.write(byte(5));
      lcd.print(nivel_agua);
      lcd.write(byte(1));
      lcd.setCursor(15, 1);
      lcd.print(" ");
    }
    /* if (t_blink0 - blink0 >= 1500)
    {
      lcd.createChar(5, aguaVacia);
      blink0 = t_blink0;
      lcd.setCursor(9, 1);
      lcd.print("       ");
    } */
  }
  else
  {
    lcd.createChar(5, aguaBaja);
    lcd.setCursor(9, 1);
    lcd.write(byte(5));
    if (nivel_agua > 24)
    {
      lcd.createChar(5, aguaMitad);
      lcd.setCursor(9, 1);
      lcd.write(byte(5));
      if (nivel_agua > 50)
      {
        lcd.createChar(5, aguaMedioLlena);
        lcd.setCursor(9, 1);
        lcd.write(byte(5));
        if (nivel_agua > 74)
        {
          lcd.createChar(5, aguaLlena);
          lcd.setCursor(9, 1);
          lcd.write(byte(5));
        }
      }
    }
    lcd.setCursor(10, 1);
    lcd.print(nivel_agua);
    lcd.write(byte(1));
    if (nivel_agua < 10)
    {
      lcd.setCursor(15, 1);
      lcd.print(" ");
    }
    if (nivel_agua == 100.0)
    {
      lcd.setCursor(15, 1);
      lcd.write(byte(1));
    }
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

void conect_to_wifi(){
  WiFi.mode(WIFI_STA);
  if(!WiFi.config(ip, gateway, subnet)){ //Configuracion
    Serial.println("Fallo al configurar el wifi");
  }
  Serial.print("[OLD] ESP32 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  uint8_t newMACAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x66};
  esp_wifi_set_mac(WIFI_IF_STA, &newMACAddress[0]);
  WiFi.begin(ssid, passwd);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected to WiFi network");
  
  Serial.print("Conectado con éxito, mi IP es: ");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.macAddress());
}

void enviar_datos(){
   if (WiFi.status() == WL_CONNECTED){
    
    HTTPClient http;
    String datos = "temperatura=" + String(temperatura) + "&humedad=" + String(humedad)+ "&humedad_suelo="+String(humedad_suelo) + "&estado_bomba="+String(estado_bomba)+"&nivel_agua="+String(nivel_agua)+"&luminosidad="+String(luminosidad)+"&METHOD="+String("POST");;

    // http.begin("http://192.168.0.113/waterlettuce/index.php");        //Indicamos el destino
    http.begin("http://192.168.0.10/waterlettuce/index.php");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded"); // Preparamos el header text/plain si solo vamos a enviar texto plano sin un paradigma llave:valor.

    int status_response = http.POST(datos); // Enviamos el post pasándole, los datos que queremos enviar. (esta función nos devuelve un código que guardamos en un int)

    if (status_response > 0){
      Serial.println("Código HTTP ► " + String(status_response)); // Print return code

      if (status_response == 200){
        String data_response= http.getString();
        Serial.println("El servidor respondió ▼ ");
        Serial.println(data_response);
      }
    }else{

      Serial.print(" Error enviando POST, código: ");
      Serial.println(status_response);
    }
    http.end(); 
  }
}
void enviar_datos_riego(){
   if (WiFi.status() == WL_CONNECTED){
    
    HTTPClient http;
    String datos = "nivel_agua="+String(nivel_agua)+"&METHOD="+String("RIEGO");;

    // http.begin("http://192.168.0.113/waterlettuce/index.php");        //Indicamos el destino
    http.begin("http://192.168.0.10/waterlettuce/index.php");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded"); // Preparamos el header text/plain si solo vamos a enviar texto plano sin un paradigma llave:valor.

    int status_response = http.POST(datos); // Enviamos el post pasándole, los datos que queremos enviar. (esta función nos devuelve un código que guardamos en un int)

    if (status_response > 0){
      Serial.println("Código HTTP ► " + String(status_response)); // Print return code

      if (status_response == 200){
        String data_response= http.getString();
        Serial.println("El servidor respondió ▼ ");
        Serial.println(data_response);
      }
    }else{

      Serial.print(" Error enviando POST, código: ");
      Serial.println(status_response);
    }
    http.end(); 
  }
}
