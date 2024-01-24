#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "RTClib.h"
// const byte led_gpio = 32;
const int outputPin = 32;
bool state = true;
RTC_DS3231 rtc;
#include <nvs_flash.h>
#include <Preferences.h>
Preferences preferences;

String daysOfTheWeek[7] = { "Domingo", "Lunes", "Martes", "Miercoles", "Jueves", "Viernes", "Sabado" };
String monthsNames[12] = { "Enero", "Febrero", "Marzo", "Abril", "Mayo",  "Junio", "Julio","Agosto","Septiembre","Octubre","Noviembre","Diciembre" };

#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth no está habilitado! Ejecute `make menuconfig` y actívelo
#endif

BluetoothSerial SerialBT;
/*
 * Ejemplo leer temperatura y humedad
 * Sensor DHT11 y ESP32s
 */
#include <DHT.h>      //https://github.com/adafruit/DHT-sensor-library
#define DHTPIN 26  
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);

#include <Arduino.h>;

const int SensorPin=13;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// controlar relay bomba de agua
#define RELAIS 32
#define	BUF_SIZE	32
char estado_bomba[BUF_SIZE] = "Apagada";
char clase_riego[BUF_SIZE] = "Automatico";

char buffer [BUF_SIZE] = "";

int periodo = 2000;
unsigned long TiempoAhora = 0;
int tipo_riego=2;   /// riego automatico por defecto
int humedad,bomba,sensorValue,t,h,readId,pgAct = 0;
// int H_i, H_f, M_i, M_f = 0;

//////************************************************************//////
void setup() {  
  Serial.begin(115200);
  delay(20);
  if (! rtc.begin())
  {
    Serial.println("DS3231 RTC Modulo no esta conectado");
    while (1);
  }
  if (rtc.lostPower())
  {
    Serial.println("RTC power failure, resetting the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  rtc.clearAlarm(1);
  rtc.disableAlarm(1);
  rtc.clearAlarm(2);
  rtc.disableAlarm(2);
  DateTime now = rtc.now(); // Obtener hora actual

  // Imprimir hora y fecha actual
  char buff[] = "Hora de inicio hh:mm:ss DDD, DD MMM YYYY";
  Serial.println(now.toString(buff));  

  SerialBT.begin("Riego_automatico_ESP32");
  Serial.println("El dispositivo comenzó, ¡ahora puedes emparejarlo con bluetooth!");  
  preferences.begin("my-app", false);
  
  delay(100);
  dht.begin();
  pinMode(SensorPin, INPUT);    ///.......... Pin del senosor humedad  
  pinMode(RELAIS,OUTPUT);       ///.......... Relay que controla la bomba

  if(!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 la asignación falló"));
    for(;;);
  }  
}
////**********************************************************////
void loop() 
{   
 if (millis() > TiempoAhora + periodo) {
  TiempoAhora = millis();
  h = dht.readHumidity();  // Leer humedad ºC Antes de leer t, h. Esperar 2 segundos entre cada lectura
  t = dht.readTemperature();  // Leer temperatura ºC   //float f = dht.readTemperature(true); // si se le pasa a la funcion el parametro true obtenemos la temperatura en ºF
  sensorValue = analogRead(SensorPin);
  humedad = ( 100 - ( (sensorValue/4095.00) * 100 ) );    
  }
  if (Serial.available()) {       /// Leer informacion pueto serie y la envia al bluetooth ///
    SerialBT.write(Serial.read());
  }  
  if (SerialBT.available())     /// Leer informacion enviada desde bluetooth ///
  {
    int h_on, m_on, h_off, m_off = 0;
    int domDay, lunDay, marDay, mierDay, jueDay, vierDay, sabDay=0;
    String datos = SerialBT.readString();     /// Guarda como string lo recibido ///
    // Serial.println(datos);  
    tipo_riego = (datos.toInt());  
    delay(10);  
    if (datos.startsWith("2"))
    {
      preferences.begin("my-app", false);
       tipo_riego=2;
      preferences.putInt("ModoProg", tipo_riego);    /// Guarda lo recibido en EEProm ///
    } 
    if (datos.startsWith("1"))
    {
      preferences.begin("my-app", false);
      tipo_riego=1;
      preferences.putInt("ModoProg", tipo_riego);
    }
    if (datos.startsWith("3"))
    {
      preferences.begin("my-app", false);
      tipo_riego=3;
      preferences.putInt("ModoProg", tipo_riego);
    }    
    if (datos.startsWith("<")){
      datos.remove(0,1);
       h_on = (datos.toInt());
      datos.remove(0,((datos.indexOf(","))+1));
       m_on = (datos.toInt());
      datos.remove(0,((datos.indexOf(","))+1));
       h_off = (datos.toInt());
      datos.remove(0,((datos.indexOf(","))+1));
       m_off = (datos.toInt());
      datos.remove(0,((datos.indexOf(">"))+1));       
      domDay=(datos.toInt());
      datos.remove(0,((datos.indexOf(","))+1));
      lunDay=(datos.toInt());
      datos.remove(0,((datos.indexOf(","))+1));
      marDay=(datos.toInt());
      datos.remove(0,((datos.indexOf(","))+1));
      mierDay=(datos.toInt());
      datos.remove(0,((datos.indexOf(","))+1));
      jueDay=(datos.toInt());
      datos.remove(0,((datos.indexOf(","))+1));
      vierDay=(datos.toInt());
      datos.remove(0,((datos.indexOf(","))+1));
      sabDay=(datos.toInt());
      datos.remove(0,((datos.indexOf("."))+1));
      tipo_riego=4;
      guardar_en_EEPROM(4,h_on,h_off,m_on,m_off,domDay,lunDay,marDay,mierDay,jueDay,vierDay,sabDay); 
      /// Guardar programa o horario de riego en EEprom ///           
    }
    delay(10);    
  }
  switch (tipo_riego) {
    case 1:           // riego manual activado manualmente
        encender_bomba(true);         
      break;
    case 2:           // automatico segun humedad del suelo 
        if (humedad <= 35) {
            encender_bomba(true);   /// Si la humedad del suelo es baja encender bomba /// 
        }
          else {
            encender_bomba(false);  /// Si la humedad del suelo es baja encender bomba /// 
          }
      break;
    case 3: // riego manual activado manualmente
          encender_bomba(false); 
      break; 
    default:

      break;      
  }  
  mostrar_datos_x_oled();
  enviar_datos_x_bluettoth(); 
  Horario_de_riego();  
}
////***********END LOOP*****************////
////***********************************************************************////
void Horario_de_riego()
{
    DateTime now = rtc.now();    
    if (state == false && isScheduledON(now))    // Apagado y debería estar encendido
      {
        encender_bomba(true);
        state = true;
        Serial.print("Riego programado iniciado: ");    
        Serial.print(now.hour());
        Serial.print(":");
        Serial.println(now.minute());
      }
      else if (state == true && !isScheduledON(now))  // Encendido y deberia estar apagado
      {
        encender_bomba(false);
        state = false;
        Serial.print("Riego programado finalizado: "); 
        Serial.print(now.hour());
        Serial.print(":");
        Serial.println(now.minute());
      }
      delay(100); 
}
////***********************************************************************////
void enviar_datos_x_bluettoth()  {
  DateTime ahora = rtc.now(); 
  int pgAct=int(isScheduledON(ahora)); 
  // int pgAct=1;

   int h_i = preferences.getInt("HoraI", 0); 
   int h_f = preferences.getInt("HoraF", 0); 
   int m_i = preferences.getInt("MinutoI", 0); 
   int m_f = preferences.getInt("MinutoF", 0);

  sprintf(buffer, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,",humedad,t,h,tipo_riego,bomba,pgAct,h_i,m_i,h_f,m_f);  // Unir los datos en una cadena para enviara bluettoth     
  SerialBT.println(buffer); 
  delay(5); 
  
}
////*************************************************////
void encender_bomba(bool interruptor)
{
  digitalWrite(RELAIS,!interruptor);    //// Envia true o false al pueto dirgital para encender o apagar la bomba ////
  if (interruptor){
    strcpy(estado_bomba, "Encendida");    
    bomba=1;
  }
  else
  {
    strcpy(estado_bomba, "Apagada");    
    bomba=0;
  }
  delay(10);  
}
////*************************************************////
void mostrar_datos_x_oled()
{
  oled.cp437(true);   //Activar página de código 437
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.setCursor(0, 1);
  oled.println("--RIEGO AUTOMATICO--");  
  oled.print("Humedad suelo:");
  oled.print(humedad);
  oled.println("% ");
  oled.print("Temperatura:");
  oled.print(t);
  oled.write(248);  
  oled.println("C");
  oled.print("Humedad aire:");
  oled.print(h);
  oled.println("%");  
  oled.print("Modo:");
  tipo_riego=leer_de_EEPROM();
  
  if (tipo_riego==2) strcpy(clase_riego, "Automatico"); 
  if (tipo_riego==1) strcpy(clase_riego, "Manual");
  if (tipo_riego==3) strcpy(clase_riego, "Manual");
  if (tipo_riego==4) strcpy(clase_riego, "Programado");

  oled.println(clase_riego);
  oled.print("Bomba:");
  oled.println(estado_bomba);
  DateTime now = rtc.now(); // Obtener hora actual
  char buff[] = "Hora hh:mm:ss";
  oled.println(now.toString(buff));   
  char buff2[] = "Fecha: DD MMM YYYY";
  oled.print(now.toString(buff2));  
  oled.display(); 
  oled.clearDisplay();
  delay(50);
}
/////---------------------------------------------------------------------/////
int leer_de_EEPROM()
{
  int dato = preferences.getInt("ModoProg", 0);     
  delay(10); 
  return dato;
}
///////---------------------------------------------------------------------///////
void guardar_en_EEPROM(int Modo, int HoraInicio, int HoraFin, int MinutoInicio, int MinutoFin, int domDay,int lunDay,int marDay,int mierDay,int jueDay,int vierDay, int sabDay)
{ 
  preferences.begin("my-app", false);
  preferences.putInt("ModoProg", Modo);
  preferences.putInt("HoraI", HoraInicio);
  preferences.putInt("HoraF", HoraFin);
  preferences.putInt("MinutoI", MinutoInicio);
  preferences.putInt("MinutoF", MinutoFin);

  preferences.putInt("domDia", domDay);
  preferences.putInt("lunDia", lunDay);
  preferences.putInt("marDia", marDay);
  preferences.putInt("mierDia", mierDay);
  preferences.putInt("jueDia", jueDay);
  preferences.putInt("vierDia", vierDay);
  preferences.putInt("sabDia", sabDay);
  
  Serial.println("Horario de riego se guardo en la EEPROM");
  delay(10);
}
///////---------------------------------------------------------------------///////
// Comprobar si esta programado el encendido
bool isScheduledON(DateTime date)
{
  int weekDay = date.dayOfTheWeek();
  // float hours = date.hour() + date.minute() / 60.0;
  float hora = date.hour();
  float minuto = date.minute();
  int pgAct = preferences.getInt("ModoProg", 0); 
   int H_i = preferences.getInt("HoraI", 0); 
   int H_f = preferences.getInt("HoraF", 0); 
   int M_i = preferences.getInt("MinutoI", 0); 
   int M_f = preferences.getInt("MinutoF", 0);

  int D1 = preferences.getInt("domDia", 10);
  int D2 = preferences.getInt("lunDia", 20); 
  int D3 = preferences.getInt("marDia", 30); 
  int D4 = preferences.getInt("mierDia", 40);
  int D5 = preferences.getInt("jueDia", 50); 
  int D6 = preferences.getInt("vierDia",60); 
  int D7 = preferences.getInt("sabDia", 70);  
  
  if (D1==11) {D1=0;}
  if (D2==21) {D2=1;}
  if (D3==31) {D3=2;}
  if (D4==41) {D4=3;}
  if (D5==51) {D5=4;}
  if (D6==61) {D6=5;}
  if (D7==71) {D7=6;}

  if (D1==10) {D1=8;}
  if (D2==20) {D2=8;}
  if (D3==30) {D3=8;}
  if (D4==40) {D4=8;}
  if (D5==50) {D5=8;}
  if (D6==60) {D6=8;}
  if (D7==70) {D7=8;}
  
  bool hourCondition = (hora >= H_i && minuto >= M_i && hora <= H_f && minuto < M_f);  
  bool dayCondition = (weekDay == D1 || weekDay == D2 ||  weekDay == D3 || weekDay == D4 || weekDay == D5 || weekDay == D6 || weekDay == D7 ); 
  if (hourCondition && dayCondition)  
  {
    return true;    
  }  
  return false;  
}
///______________________________________________________//// 

