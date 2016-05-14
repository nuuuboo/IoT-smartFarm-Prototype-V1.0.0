#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include <RTClib.h>
#include <Ultrasonic.h>
#include <DHT.h>
//#include <LinkedList.h>
//*********************************************************************
// Class Job and (Schedule)
//*********************************************************************

//class Job{
//public:
//  uint16_t id;
//  String startTime, endTime, detail;
//  bool sun, mon, tue, wen, thu, fri, sat, active;
//
//  Job(uint16_t newId, String detail) : id(newId), detail(detail){
//    int i =0;
//    String wordCheck[15];
//    wordCheck[i] = getValue(detail, ' ', i);
//
//    while(wordCheck[i] != ""){
//      i++;
//      wordCheck[i] = getValue(detail, ' ', i);
//    }
//
//    startTime = wordCheck[2];
//    endTime   = wordCheck[3];
//    sun       = wordCheck[4];
//    mon       = wordCheck[5];
//    tue       = wordCheck[6];
//    wen       = wordCheck[7];
//    thu       = wordCheck[8];
//    fri       = wordCheck[9];
//    sat       = wordCheck[10];
//    active    = wordCheck[11];
//  }
//
//  String getValue(String data, char separator, int index)
//  {
//    int found = 0;
//    int strIndex[] = {0, -1};
//    int maxIndex = data.length()-1;
//
//    for(int i=0; i<=maxIndex && found<=index; i++){
//      if(data.charAt(i)==separator || i==maxIndex){
//          found++;
//          strIndex[0] = strIndex[1]+1;
//          strIndex[1] = (i == maxIndex) ? i+1 : i;
//      }
//    }
//
//    return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
//  }
//
//};
//
//// dictionary
//LinkedList<Job*> _listJobs = LinkedList<Job*>();
//int jobUniqueId = 0;
//***************************End Class Job***********************************

//*********************************************************************
// Function for Scheduler
//*********************************************************************
//void addJob(String allDetail){
//    Job *ajob = new Job(jobUniqueId, allDetail);
//    _listJobs.add(ajob);
//    jobUniqueId++;
//}
//
//int removeJob(int aId){
//    bool found = false;
//    Job *aJob;
//    for(int i=0; i < _listJobs.size() ; i++){
//      aJob = _listJobs.get(i);
//      if(aJob->id == aId){
//        _listJobs.remove(i);
//        found = true;
//        break;
//      }
//    }
//    if(found){return aId;}
//    else{return -1;}
//}

RTC_DS1307 rtc;
// valve
#define valve 7

// data transfer rate
int transferRate = 5;
 
// sensor
#define light 1 //analog
#define temp 3
#define air 4
#define soil 0 // analog
#define water 5
#define cs_pin 10
#define DHTTYPE DHT22

int array[5] = {temp,light,air,soil,water};
DHT dht(temp, DHTTYPE);
Ultrasonic ultrasonic(water);

// option for watering depend on which sensor
int optionScheHum = 1;
// variable to control the system
int soilLow = 0;//970 for test
int soilHigh = 0;//1000 for test
int waterLow = 0;
int waterHigh = 0;

// dictionary
//const byte HASH_SIZE = 5;
//HashType<char*,int> hashRawArray[HASH_SIZE];
//HashMap<char*,int> hashMap = HashMap<char*,int>( hashRawArray , HASH_SIZE );
 

// date & time
int dd = 0;
int mm = 0;
int yy = 0;
int hh = 0;
int mi = 0;
unsigned long timePrev;

// SDCard
bool error = false;
bool error2 = false;
File logFile;
File logFile2;
int countOpen = 0;
int countClose = 0;

//Serial input
char inChar;
bool endFlag = false;
String str;
String wordCheck;


//*****************************************************
// *************** Function *************************
// print Date & Time
void display(int dd, int mm, int yy, int hh, int mi) {
  //Serial.print(dd);
  //Serial.print("/");
  //Serial.print(mm);
  //Serial.print("/");
  //Serial.print(yy);
  //Serial.print(" ");
  //Serial.print(hh);
  //Serial.print(":");  
  //Serial.println(mi);
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
      //Serial.print("Temp: ");
      //Serial.println(dht.readTemperature());
      return dht.readTemperature();
    case air:
      //Serial.print("Humidity: ");
      //Serial.println(dht.readHumidity());
      return dht.readHumidity();
    case light:
      //Serial.print("Light: ");
      //Serial.println(analogRead(light));
      return analogRead(light);
    case soil:
      //Serial.print("Soil Moisture: ");
      //Serial.println(analogRead(soil));
      return analogRead(soil);
    case water:
      ultrasonic.MeasureInCentimeters();
      //Serial.print("Water Level: ");
      //Serial.println(ultrasonic.RangeInCentimeters);
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
// select to use Watering based on schedule or Humidity Level
void setOption(int scheduleHum){
  if (scheduleHum == 0){
    optionScheHum = 0;
    }
  else{
    optionScheHum =  1;
  }
}
// method to set how low humidity should be to open valve and how high to close valve
void setHumidity(int l, int h){
  soilLow = l;
  soilHigh = h;
  //Serial.println("===== Set Value to ======");
  //Serial.print(soilLow);
  //Serial.print(" ");
  //Serial.println(soilHigh);
}

// method to set Schedule(timeStart, timeStop, Day0-6, active)
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

void openValve( ){// may add int valveID
  digitalWrite(valve, HIGH);
  //Serial.println(F("======= Open Valve ========"));
  //Serial.println(F("======= Write Data (Water) ========"));
  //Serial.print("Soil Moisture: ");
  //Serial.println(getData(soil));
  countClose = 0;
  if (countOpen == 0){
    countOpen =1;
    logFile2.print(timeStamp(dd,mm,yy,hh,mi));
    logFile2.print(",");
    logFile2.print(getData(soil));
    logFile2.print(",");
    logFile2.print("open");
    logFile2.println();
    logFile2.flush(); 
    }
  }

void closeValve( ){// may add int valveID
  digitalWrite(valve, LOW);
  countOpen = 0;
  //Serial.println(F("======= Close Valve ========"));
  //Serial.println(F("======= Write Data (Water) ========"));
  //Serial.print("Soil Moisture: ");
  //Serial.println(getData(soil));
  if (countClose = 0){
    countClose = 1;
    logFile2.print(timeStamp(dd,mm,yy,hh,mi));
    logFile2.print(",");
    logFile2.print(getData(soil));
    logFile2.print(",");
    logFile2.print("close");
    logFile2.println();
    logFile2.flush(); 
    }
  }

void deleteSchedule(int scheduleID){
  //doSomething
//  for (int i=1; i<12; i++){
//    
//    }
  }
void setWaterLevel(int low, int high){
  waterLow = low;
  waterHigh = high;
}
// ********** End of Function *****************

void setup() {
  Wire.begin();
  rtc.begin();
  dht.begin();
  Serial.begin(9600);
  pinMode(valve, OUTPUT);
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
      error =true;
      //Serial.println("Can't Open");
      }
    else{ //Serial.println("Open file1 for writing.");
//       logFile.print("Timestamp,Temperature,Light,Air Humidity,Soil Moisture,Water Level");
//       logFile.println();
//       logFile.flush();
      }
    logFile2 = SD.open("logWater.csv", FILE_WRITE);
    if (logFile2 == NULL){
      error2 = true;
      //Serial.println("Can't Open");
      }
    else{ 
      //Serial.println("Open file2 for writing.");
//       logFile2.print("Timestamp,Soil Moisture, State");
//       logFile2.println();
//       logFile2.flush();
      }
    }
    
   else {error = true;
   error2 = true;
   //Serial.println("Can't Open");
   }
}

void loop() {
  // check Time
  DateTime now = rtc.now();
  dd = now.day();
  mm = now.month();
  yy = now.year();
  hh = now.hour();
  mi = now.minute();
  //display(dd, mm, yy, hh, mi);

// get Data from Sensor and write to csv file in SD card
  unsigned long timeNow, timeElapsed;
  timeNow = rtc.now().unixtime();
  timeElapsed = timeNow - timePrev;
  if (timeElapsed > transferRate) {
    //Serial.println("======= Write Data ========");
    logFile.print(timeStamp(dd,mm,yy,hh,mi));
    for (int i=0; i < 5; i++){
      //Serial.print("Round:");
      //Serial.println(i);
      logFile.print(",");
      logFile.print(getData(array[i]));
      }
      logFile.println();
      logFile.flush(); 
    timePrev = timeNow;
//      ledState = 1 - ledState; //toggle state
//      digitalWrite(D13, ledState); //turn LED on after 1 sec
  }

// read From //Serial
  if (Serial.available() > 0)
    {
    inChar = Serial.read();
    if (inChar == '\n' ){
      endFlag = true;
      }
    else{
      str.concat(inChar);
      }
    }
  if (endFlag == true){
    str.toLowerCase();
    //Serial.print("end");
    int i = 0;
    //Serial.println("//////////////////////////////////////////////");
    //Serial.println(str);
    String wordCheck[15];
    wordCheck[i] = getValue(str, ' ', i);
    
    while(wordCheck[i] != ""){
    //  wordCheck[i] = getValue(split, ' ', i);
      //Serial.println(wordCheck[i]);
      i++;
      wordCheck[i] = getValue(str, ' ', i);
      
    }
    
    Serial.println((String)wordCheck[1]);
    if (wordCheck[1] == (String)"sethumidity"){
       //setHumidity (Low, High); // Low: open valve | High: stop valve
      Serial.println("************************************************");
      Serial.println("==== Setting Humidity ====");
      Serial.print(wordCheck[2]);
      Serial.print(" ");
      Serial.println(wordCheck[3]);
      setHumidity(wordCheck[2].toInt(), wordCheck[3].toInt());
      }
    else if (wordCheck[1] == (String)"option"){
      setOption(wordCheck[2].toInt());
      }
    else if (wordCheck[1] == (String)"setschedule"){
      // call Function
//      setSchedule(wordCheck);
      }
    else if (wordCheck[1] == (String)"openvalve"){
      // call Function
      openValve();
      }
    else if (wordCheck[1] == (String)"stopvalve"){
      // call Function
      closeValve();
      }
    else if (wordCheck[1] == (String)"deleteschedule"){
      // call Function
      
      }
    else if (wordCheck[1] == (String)"setwaterlevel"){
      // call Function
      setWaterLevel(wordCheck[2].toInt(), wordCheck[3].toInt());
      }
//    else if (wordCheck[1] == (String)"setdataupdate"){
//      // call Function
//      }
      else {
        //Serial.println(F("Command not Found"));
      }
    endFlag = false;
    i = 0;
    str = "";
    }

// use var: optionScheHum to change the system watering option between Humidity and Schedule
  if (optionScheHum == 1){
    if (getData(soil) < soilLow){
      openValve();

      }
    else if (getData(soil) > soilHigh){
      closeValve();
      }
  }

    
// end loop  
}



