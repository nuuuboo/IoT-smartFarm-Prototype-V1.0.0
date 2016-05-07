#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include <RTClib.h>
#include <Ultrasonic.h>
#include <DHT.h>
#include <HashMap.h>

RTC_DS1307 rtc;
// sensor
#define light 3
#define temp 1
#define air 2
#define soil 4
#define water 5
#define cs_pin 10
#define DHTTYPE DHT22
int array[5] = {temp,light,air,soil,water};
DHT dht(temp, DHTTYPE);
Ultrasonic ultrasonic(water);

int airLow = 0;
int airHigh = 0;

// dictionary
const byte HASH_SIZE = 5;
HashType<char*,int> hashRawArray[HASH_SIZE];
HashMap<char*,int> hashMap = HashMap<char*,int>( hashRawArray , HASH_SIZE );
 

// date & time
int dd = 0;
int mm = 0;
int yy = 0;
int hh = 0;
int mi = 0;
unsigned long timePrev;

// SDCard
bool error = false;
File logFile;

char inChar;
bool endFlag = false;
String str;
String wordCheck;

void setup() {
  Wire.begin();
  rtc.begin();
  dht.begin();
  Serial.begin(9600);
  if (!rtc.isrunning()){
    DateTime compileTime = DateTime(__DATE__,__TIME__);
    rtc.adjust(compileTime);
    dd = compileTime.day();
    mm = compileTime.month();
    yy = compileTime.year();
    hh = compileTime.hour();
    mi = compileTime.minute();
    display(dd, mm, yy, hh, mi);
   }
  if (SD.begin(cs_pin)){
    logFile = SD.open("log.csv", FILE_WRITE);
    if (logFile == NULL){
      error =true;}
    else {error = true;}
    }
   
}

// print Date & Time
void display(int dd, int mm, int yy, int hh, int mi) {
  Serial.print(dd);
  Serial.print("/");
  Serial.print(mm);
  Serial.print("/");
  Serial.print(yy);
  Serial.print(" ");
  Serial.print(hh);
  Serial.print(":");  
  Serial.println(mi);
}

// append timeStamp in File
String timeStamp(int dd, int mm, int yy, int hh, int mi) {
  String timeStamp;
  timeStamp.concat(dd);
  timeStamp.concat("/");
  timeStamp.concat(mm);
  timeStamp.concat("/");
  timeStamp.concat(yy);
  timeStamp.concat(" ");
  timeStamp.concat(hh);
  timeStamp.concat(":");  
  timeStamp.concat(mi);
  return timeStamp;
}

// get Data from Sensor
int getData(int i){
  switch(i){
    case temp:
      return dht.readTemperature();
    case air:
      return dht.readHumidity();
    case light:
      return analogRead(light);
    case soil:
      return analogRead(soil);
    case water:
      ultrasonic.MeasureInCentimeters();
      return ultrasonic.RangeInCentimeters;
    default:
      break;
    }
  }

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void setHumidity(int l, int h){
  airLow = l;
  airHigh = h;
  }

//void setSchedule(String[] schedule){
//      hashMap[2]("timeStart",schedule[2].toInt() );
////      schedule[3].toInt()
////      schedule[4].toInt()
////      schedule[5].toInt()
////      schedule[6].toInt()
////      schedule[7].toInt()
////      schedule[8].toInt()
////      schedule[9].toInt()
////      schedule[10].toInt()
////      schedule[11].toInt()
////      schedule[12].toInt()
//  }


void loop() {
  // check Time
  DateTime now = rtc.now();
  dd = now.day();
  mm = now.month();
  yy = now.year();
  hh = now.hour();
  mi = now.minute();
  //display(dd, mm, yy, hh, mi);

// get Data from Sensor
  unsigned long timeNow, timeElapsed;
  timeNow = rtc.now().unixtime();
  timeElapsed = timeNow - timePrev;
  if (timeElapsed > 290) {
    Serial.println("Write Data");
    logFile.print(timeStamp(dd,mm,yy,hh,mi));
    for (int i=0; i < sizeof(array); i++){
      logFile.print(",");
      logFile.print(getData(array[i]));
      }
      logFile.println();
      logFile.flush(); 
    timePrev = timeNow;
//      ledState = 1 - ledState; //toggle state
//      digitalWrite(D13, ledState); //turn LED on after 1 sec
  }

// read From Serial
  if (Serial.available() > 0){
    inChar = Serial.read();
    if (inChar == '\n' ){
      endFlag = true;
      }
    else{
      str.concat(inChar);
      }
    }
  if (endFlag == true){
    Serial.print("end");
    int i =0;
    String wordCheck[15];
    wordCheck[i] = getValue(str, ' ', i);
    
    while(wordCheck[i] != ""){
    //  wordCheck[i] = getValue(split, ' ', i);
      Serial.println(wordCheck[i]);
      i++;
      wordCheck[i] = getValue(str, ' ', i);
    }
    
    if (wordCheck[1] == (String)"sethumidity"){
      // set Humidity (Low, High) | Low: open valve | High: stop valve
      setHumidity(wordCheck[2].toInt(), wordCheck[3].toInt());
      }
    else if (wordCheck[1] == (String)"setschedule"){
      // call Function
//      setSchedule(wordCheck);
      }
    else if (wordCheck[1] == (String)"openvalve"){
      // call Function
      }
    else if (wordCheck[1] == (String)"stopvalve"){
      // call Function
      }
    else if (wordCheck[1] == (String)"deleteschedule"){
      // call Function
      }
    else if (wordCheck[1] == (String)"setwaterlevel"){
      // call Function
      }
//    else if (wordCheck[1] == (String)"setdataupdate"){
//      // call Function
//      }
      else {
        Serial.println(F("Command not Found"));
      }
    endFlag = false;
    }
  
}



