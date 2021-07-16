#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
#include "EmonLib.h"  // Библиотека для EmonLib
#define REQ_BUF_SZ 60
#define ON 0 // Глобальная константа OFF содержит ноль значения LOW
#define OFF 1 // Глобальная константа ON содержит единицу значения HIGH
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
RTC_DS3231 rtc;
EnergyMonitor emon1;             // Create an instance
int i = 0; 
float tenvals = 0.0;
float minval = 1000;
float maxval = 0.0;
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 0, 20);      // IP address, may need to change depending on network
EthernetServer server(80);          // create a server at port 80
File webFile;                       // the web page file on the SD card
char HTTP_req[REQ_BUF_SZ] = { 0 };  // buffered HTTP request stored as null terminated string
char req_index = 0;                 // index into HTTP_req buffer
boolean LED_state[13] = { 0 };       // stores the states of the LED

void setup()
{  
    // disable Ethernet chip
    pinMode(10, OUTPUT);
    digitalWrite(11, HIGH);   
    // Пины статусов
    pinMode(39, INPUT_PULLUP);
    pinMode(41, INPUT_PULLUP);
    pinMode(43, INPUT_PULLUP);
    pinMode(45, INPUT_PULLUP);
    pinMode(47, INPUT_PULLUP);
    pinMode(49, INPUT_PULLUP);
    pinMode(51, INPUT_PULLUP);
    pinMode(53, INPUT_PULLUP);

    pinMode(15, INPUT_PULLUP);
    pinMode(14, INPUT_PULLUP);
    pinMode(13, INPUT_PULLUP);
    pinMode(12, INPUT_PULLUP);
    pinMode(11, INPUT_PULLUP);
    pinMode(10, INPUT_PULLUP);
    pinMode(9, INPUT_PULLUP);
    // Пины Реле
    pinMode(23, OUTPUT);//Включения грязи
    pinMode(25, OUTPUT);//Включения стабилизатора
    pinMode(27, OUTPUT);//Включения нагрузка 1
    pinMode(29, OUTPUT);//Включения нагрузка 2
    pinMode(31, OUTPUT);//Включения нагрузка 3
    pinMode(33, OUTPUT);//Включения вентилятора 1
    pinMode(35, OUTPUT);//Включения вентилятора 2
    pinMode(37, OUTPUT);//Включения вентилятора 3
    pinMode(22, OUTPUT);//Включения вентилятора 4
    pinMode(24, OUTPUT);//Включения вентилятора 5
    pinMode(26, OUTPUT);//Включения спец нагрузки
    pinMode(28, OUTPUT);//РЕЗЕРВ
    pinMode(30, OUTPUT);//РЕЗЕРВ
    pinMode(32, OUTPUT);//РЕЗЕРВ
    pinMode(34, OUTPUT);//РЕЗЕРВ
    pinMode(36, OUTPUT);//РЕЗЕРВ
    emon1.voltage(0, 418, 1.7);  // Voltage: input pin, calibration, phase_shift
     // switches on pins 2, 3 and 5
    pinMode(2, INPUT);
    pinMode(3, INPUT);
    pinMode(5, INPUT);

    if (! rtc.begin()) {
        Serial.println("Couldn't find RTC"); // проверяет есть ли сиглан с реальных часов
        while (1);
    }
    if (rtc.lostPower()) {
        Serial.println("RTC lost power, lets set the time!");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    Serial.begin(9600);  // for debugging
    // initialize SD card
    Serial.println("Initializing SD card...");
    if (!SD.begin(4)) {
        Serial.println("ERROR - SD card initialization failed!");
        return;  // init failed
    }
    Serial.println("SUCCESS - SD card initialized.");
    // check for index.htm file
    if (!SD.exists("index.htm")) {
        Serial.println("ERROR - Can't find index.htm file!");
        return;  // can't find index file
    }
    Serial.println("SUCCESS - Found index.htm file.");
    Ethernet.begin(mac, ip);  // initialize Ethernet device
    server.begin();           // start to listen for clients
} //END SETUP

// Начальное положение пинов реле
void setKeyState1() {
    digitalWrite(23, OFF);
    digitalWrite(25, OFF);
    digitalWrite(27, OFF);
    digitalWrite(29, OFF);
    digitalWrite(31, OFF);
    digitalWrite(33, OFF);
    digitalWrite(35, OFF);
    digitalWrite(37, OFF);
    digitalWrite(22, OFF);
    digitalWrite(24, OFF);
    digitalWrite(26, OFF);
    digitalWrite(28, OFF);
    digitalWrite(30, OFF);
    digitalWrite(32, OFF);
    digitalWrite(34, OFF);
    digitalWrite(36, OFF);
    }
void writeSdLog(){ //Функция записи логов при нажатии на кнопку
  // открываем файл. Примечание: только один файл может быть открыт в один момент времени 
  // поэтому вам необходимо закрыть этот файл перед тем как открывать следующий
  File dataFile = SD.open("LoggerCD.csv", FILE_WRITE);
  int count;
  int status_btn[] = {23, 25, 27, 29, 31, 33, 35, 37, 22, 24, 26, 28, 30, 32, 34, 36};
  // если файл доступен, записываем в него
  if (dataFile) {
    DateTime now = rtc.now();
      dataFile.print(now.year(), DEC);
      dataFile.print('/');
      dataFile.print(now.month(), DEC);
      dataFile.print('/');
      dataFile.print(now.day(), DEC);
      dataFile.print(" (");
      dataFile.print(daysOfTheWeek[now.dayOfTheWeek()]);
      dataFile.print(") ");
      dataFile.print(now.hour(), DEC);
      dataFile.print(':');
      dataFile.print(now.minute(), DEC);
      dataFile.print(',');
      dataFile.print("Events");
        for (count = 0; count < 15; count++){
          dataFile.print(" PIN-");
          dataFile.print(status_btn[count]);
          dataFile.print(",");
          dataFile.print(digitalRead(status_btn[count]));//в логах 0-Включиено 1-Выключено
          dataFile.print(',');
      }
      dataFile.println();
    dataFile.close();
  }
 }
void writeSdLogRelay(){//Функция записи логов при изменение статусов
  // открываем файл. Примечание: только один файл может быть открыт в один момент времени 
  // поэтому вам необходимо закрыть этот файл перед тем как открывать следующий
  File dataFile = SD.open("LoggerCD.csv", FILE_WRITE);
  int count;
  int status_relay[] = {39, 41, 43, 45, 47, 49, 51, 53, 15, 14, 13, 12, 11, 10, 9};
  // если файл доступен, записываем в него
  if (dataFile) {
    DateTime now = rtc.now();
      dataFile.print(now.year(), DEC);
      dataFile.print('/');
      dataFile.print(now.month(), DEC);
      dataFile.print('/');
      dataFile.print(now.day(), DEC);
      dataFile.print(" (");
      dataFile.print(daysOfTheWeek[now.dayOfTheWeek()]);
      dataFile.print(") ");
      dataFile.print(now.hour(), DEC);
      dataFile.print(':');
      dataFile.print(now.minute(), DEC);
      dataFile.print(',');
      dataFile.print("Events");
        for (count = 0; count < 15; count++){
          dataFile.print(" PIN-");
          dataFile.print(status_relay[count]);
          dataFile.print(",");
          dataFile.print(digitalRead(status_relay[count]));//в логах 0-Включиено 1-Выключено
          dataFile.print(',');
      }
      dataFile.println();
    //dataFile.println();
    dataFile.close();
    }
  }


// sets every element of str to 0 (clears array)
void StrClear(char *str, char length) {
  for (int i = 0; i < length; i++) {
    str[i] = 0;
  }
}
// searches for the string sfind in the string str
// returns 1 if string found
// returns 0 if string not found
char StrContains(char *str, char *sfind) {
  char found = 0;
  char index = 0;
  char len;
  len = strlen(str);
  if (strlen(sfind) > len) {
    return 0;
  }
  while (index < len) {
    if (str[index] == sfind[found]) {
      found++;
      if (strlen(sfind) == found) {
        return 1;
      }
    } else {
      found = 0;
    }
    index++;
  }
  return 0;
}


// checks if received HTTP request is switching on/off LEDs
// also saves the state of the LEDs
void SetLEDs(void) {
    // RELAY 1 (pin 23)
    if (StrContains(HTTP_req, "LED3=1")) {
        LED_state[2] = 1;  // save LED state
        digitalWrite(23, ON);
        writeSdLog();
    } else if (StrContains(HTTP_req, "LED3=0")) {
        LED_state[2] = 0;  // save LED state
        digitalWrite(23, OFF);
        writeSdLog();
    }
    //END------>
    // RELAY 2 (pin 25)
    if (StrContains(HTTP_req, "LED4=1")) {
        LED_state[3] = 1;  // save LED state
        digitalWrite(25, ON);
        writeSdLog();
    } else if (StrContains(HTTP_req, "LED4=0")) {
        LED_state[3] = 0;  // save LED state
        digitalWrite(25, OFF);
        writeSdLog();
    }
    //END------->
    // RELAY 3 (pin 27)
    if (StrContains(HTTP_req, "LED5=1")) {
        LED_state[4] = 1;  // save LED state
        digitalWrite(27, ON);
        writeSdLog();
    } else if (StrContains(HTTP_req, "LED5=0")) {
        LED_state[4] = 0;  // save LED state
        digitalWrite(27, OFF);
        writeSdLog();
    }
    //END------>
    // RELAY 4 (pin 29)
    if (StrContains(HTTP_req, "LED6=1")) {
        LED_state[5] = 1;  // save LED state
        digitalWrite(29, ON);
        writeSdLog();
    } else if (StrContains(HTTP_req, "LED6=0")) {
        LED_state[5] = 0;  // save LED state
        digitalWrite(29, OFF);
        writeSdLog();
    } 
    //END------>
    // RELAY 5 (pin 31)
    if (StrContains(HTTP_req, "LED7=1")){
        LED_state[6] = 1;  // save LED state
        digitalWrite(31, ON);
        writeSdLog();
    } else if (StrContains(HTTP_req, "LED7=0")){
        LED_state[6] = 0;  // save LED state
        digitalWrite(31, OFF);
        writeSdLog();
    }
    //END------>
    // RELAY 6 (pin 33)
    if (StrContains(HTTP_req, "LED8=1")){
      LED_state[7] = 1;  // save LED state
      digitalWrite(33, ON);
      writeSdLog();
    }else if (StrContains(HTTP_req, "LED8=0")){
      LED_state[7] = 0;  // save LED state
      digitalWrite(33, OFF);
      writeSdLog();
    }
    //END------>
    // RELAY 7 (pin 35)
    if (StrContains(HTTP_req, "LED9=1")){
      LED_state[8] = 1;  // save LED state
      digitalWrite(35, ON);
      writeSdLog();
    }else if (StrContains(HTTP_req, "LED9=0")){
      LED_state[8] = 0;  // save LED state
      digitalWrite(35, OFF);
      writeSdLog();
    }
    //END------>
    // RELAY 8 (pin 37)
    if (StrContains(HTTP_req, "LED10=1")){
      LED_state[9] = 1;  // save LED state
      digitalWrite(37, ON);
      writeSdLog();
    }else if (StrContains(HTTP_req, "LED10=0")){
      LED_state[9] = 0;  // save LED state
      digitalWrite(37, OFF);
      writeSdLog();
    }
    //END------>
    // RELAY 9 (pin 22)
    if (StrContains(HTTP_req, "LED11=1")){
      LED_state[10] = 1;  // save LED state
      digitalWrite(22, ON);
      writeSdLog();
    }else if (StrContains(HTTP_req, "LED11=0")){
      LED_state[10] = 0;  // save LED state
      digitalWrite(22, OFF);
      writeSdLog();
    }
    //END------>
    // RELAY 10 (pin 24)
    if (StrContains(HTTP_req, "LED12=1")){
      LED_state[11] = 1;  // save LED state
      digitalWrite(24, ON);
      writeSdLog();
    }else if (StrContains(HTTP_req, "LED12=0")){
      LED_state[11] = 0;  // save LED state
      digitalWrite(24, OFF);
      writeSdLog();
    }
    //END------>
    // RELAY 11 (pin 26)
    if (StrContains(HTTP_req, "LED13=1")){
      LED_state[12] = 1;  // save LED state
      digitalWrite(26, ON);
      writeSdLog();
    }else if (StrContains(HTTP_req, "LED13=0")){
      LED_state[12] = 0;  // save LED state
      digitalWrite(26, OFF);
      writeSdLog();
    }
    //END------>
    // Кондиционер 

  //----Конец---//
}

// send the XML file with analog values, switch status
//  and LED status
void XML_response(EthernetClient cl) {
  int analog_val;              // stores value read from analog inputs
  int count;                   // used by 'for' loops
  int sw_arr[] = { 39, 41, 43, 45, 47, 49, 51, 53, 15, 14, 13, 12, 11, 10, 9 };  // pins interfaced to switches
  emon1.calcVI(20,2000);         // Calculate all. No.of half wavelengths (crossings), time-out
  float Vrms = (emon1.Vrms - 3);
    
  cl.print("<?xml version = \"1.0\" ?>");
  cl.print("<inputs>");
    // read analog inputs
    for (count = 2; count <= 5; count++) {  // A2 to A5
        analog_val = analogRead(count);
        cl.print("<analog>");
        cl.print(analog_val);
        cl.println("</analog>");
    }
        // read analog voltage
        cl.print("<voltage>");
        if (Vrms < 0) { Vrms = 0.0; }
        tenvals += Vrms;
        if (minval > Vrms) { minval = Vrms; }
        if (maxval < Vrms) { maxval = Vrms; }
        i++;
        if (i == 10)
        {
            Serial.print("Avg: ");
            Serial.print(tenvals/10);
            i = 0;
            tenvals = 0.0;
            minval = 1000.0;
            maxval = 0.0;
        }
            //cl.print(tenvals/10);
            cl.print(Vrms);
            cl.print("Min: ");
            cl.print(minval);
            cl.print("Max: ");
            cl.println(maxval);
            i = 0;
            tenvals = 0.0;
            minval = 1000.0;
            maxval = 0.0;
        cl.println("</voltage>");
    // read switches
    for (count = 0; count < 15; count++) {
        cl.print("<switch>");
        if (digitalRead(sw_arr[count])) {
        writeSdLogRelay();
        cl.print("Выключено");
        } else {
        writeSdLogRelay();
        cl.print("Включино");
        }
        cl.println("</switch>");
    }
    // checkbox LED states
    // LED1
    cl.print("<LED>");
    if (LED_state[0]) {
        cl.print("checked");
    } else {
        cl.print("unchecked");
    }
    cl.println("</LED>");
    // LED2
    cl.print("<LED>");
    if (LED_state[1]) {
        cl.print("checked");
    } else {
        cl.print("unchecked");
    }
    cl.println("</LED>");
    // button LED states
    
    // RELAY 1 (pin 23)
    cl.print("<LED>");
    if (LED_state[2]) {
        cl.print("on");
    } else {
        cl.print("off");
    }
    cl.println("</LED>");
    // RELAY 2 (pin 25)
    cl.print("<LED>");
    if (LED_state[3]) {
        cl.print("on");
    } else {
        cl.print("off");
    }
    cl.println("</LED>");

    // RELAY 3 (pin 27)
    cl.print("<LED>");
    if (LED_state[4]) {
        cl.print("on");
    } else {
        cl.print("off");
    }
    cl.println("</LED>");

    // RELAY 4 (pin 29)
    cl.print("<LED>");
    if (LED_state[5]) {
        cl.print("on");
    } else {
        cl.print("off");
    }
    cl.println("</LED>");

    // RELAY 5 (pin 31)
    cl.print("<LED>");
    if (LED_state[6]) {
        cl.print("on");
    } else {
        cl.print("off");
    }
    cl.println("</LED>");

    // RELAY 6 (pin 33)
    cl.print("<LED>");
    if (LED_state[7]) {
        cl.print("on");
    }else {
        cl.print("off");
    }
    cl.println("</LED>");

    // RELAY 7 (pin 35)
    cl.print("<LED>");
    if (LED_state[8]){
        cl.print("on");
    }else {
        cl.print("off");
    }
    cl.println("</LED>");

    // RELAY 8 (pin 37)
    cl.print("<LED>");
    if (LED_state[9]) {
        cl.print("on");
    }else{
        cl.print("off");
    }
    cl.println("</LED>");

    // RELAY 9 (pin 22)
    cl.print("<LED>");
    if (LED_state[10]) {
        cl.print("on");
    }else{
        cl.print("off");
    }
    cl.println("</LED>");

    // RELAY 10 (pin 24)
    cl.print("<LED>");
    if (LED_state[11]) {
        cl.print("on");
    }else{
        cl.print("off");
    }
    cl.println("</LED>");

    // RELAY 11 (pin 26)
    cl.print("<LED>");
    if (LED_state[12]) {
        cl.print("on");
    }else{
        cl.print("off");
    }
    cl.println("</LED>");
  cl.print("</inputs>");
}

void loop()
{
EthernetClient client = server.available();  // try to get client
  
  if (client) {  // got client?
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {  // client data available to read
        char c = client.read();  // read 1 byte (character) from client
        // limit the size of the stored received HTTP request
        // buffer first part of HTTP request in HTTP_req array (string)
        // leave last element in array as 0 to null terminate string (REQ_BUF_SZ - 1)
        if (req_index < (REQ_BUF_SZ - 1)) {
          HTTP_req[req_index] = c;  // save HTTP request character
          req_index++;
        }
        // last line of client request is blank and ends with \n
        // respond to client only after last line received
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          // remainder of header follows below, depending on if
          // web page or XML page is requested
          // Ajax request - send XML file
          if (StrContains(HTTP_req, "ajax_inputs")) {
            // send rest of HTTP header
            client.println("Content-Type: text/xml");
            client.println("Connection: keep-alive");
            client.println();
            SetLEDs();
            // send XML file containing input states
            XML_response(client);
          } else {  // web page request
            // send rest of HTTP header
            client.println("Content-Type: text/html");
            client.println("Connection: keep-alive");
            client.println();
            // send web page
            webFile = SD.open("index.htm");  // open web page file
            if (webFile) {
              while (webFile.available()) {
                client.write(webFile.read());  // send web page to client
              }
              webFile.close();
            }
          }
          // display received HTTP request on serial port
          Serial.print(HTTP_req);
          // reset buffer index and all buffer elements to 0
          req_index = 0;
          StrClear(HTTP_req, REQ_BUF_SZ);
          break;
        }
        // every line of text received from the client ends with \r\n
        if (c == '\n') {
          // last character on line of received text
          // starting new line with next character read
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // a text character was received from client
          currentLineIsBlank = false;
        }
      }             // end if (client.available())
    }               // end while (client.connected())
     delay(10);       // give the web browser time to receive the data
    client.stop();  // close the connection
  }                 // end if (client)
  
}

