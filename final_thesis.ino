#include <RH_ASK.h>
#include <SPI.h>
#include <Keypad.h>
#include <SoftwareSerial.h>
#include<stdio.h>
SoftwareSerial sim(3, 2); //for the simcom Tx-Rx
RH_ASK driver;
int systemArm = 0; 
int a = 0;
String msgrcv = "";
String msg = "";
const unsigned long breaktime = 10000;
const unsigned long leavetime = 5000;
unsigned long currentTime = 0;
unsigned long intruder = 0;
unsigned long clockforhelp = 0;
const  char prog1[] = "opd";
const  char prog2[] = "cld";
const  char prog3[] = "lbt";
const int ROW_NUM = 4; //four rows
const int COLUMN_NUM = 4;
int sensormagn = 13;
int statemagn;
int state;
int systemrst = 0;
int alarmflagmagn = 1;
int alarmflagsens = 1;
int falsepass = 0;
int alarmsystem = 0;
int rst = 0;
int _timeout;
String _buffer;
String number = "+306983656719"; //-> noymero

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte pin_rows[ROW_NUM] = {12, 10, 9, 8}; //connect to the row pinouts of the keypad
byte pin_column[COLUMN_NUM] = {7, 6, 5 , 4}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

const String password = "1"; // change your password here
String input_password;
String new_input_password;

volatile int calm = 1;
volatile int angry = 0;
volatile int cm = 0;
volatile int cr = 0;
volatile int timeflag = 0;
volatile int p = 0; //is given for when password is correct or not
volatile int cmflag = 0;
volatile int crflag = 0;
int alarmtimeflag = 0;

void setup() {
  Serial.begin(9600);
  pinMode(sensormagn, INPUT_PULLUP);
  delay(7000);
  _buffer.reserve(50);
  sim.begin(9600);
  delay(1000);
  
  input_password.reserve(32); // maximum input characters is 33, change if needed
  Serial.println("give the password");
  if (!driver.init())
    Serial.println("init failed");

}

void loop() {
  currentTime = millis();
  switch (calm) {

    case 1:
      keypadpass();
      calm = 2;
      break;

    case 2:
      if (p == 1)
        timetoexit();
      calm = 3;
      break;

    case 3:
      checkmagnsens();
      delay(20);
      if (cm == 1)
        checkmagnsens();
      if (cm == 0)
        calm = 4;
      //else
      //digitalWrite (cmLed, HIGH);
      if ((systemArm == 1) && ( cm == 1))
      {
        angry = 1;
        calm = 0;
        p = 0;
        input_password = "";
        intruder = millis();

      }

      break;

    case 4:

      checkrf();

      calm = 1;
      break;

    case 5:

      break;

    default:
      break;
  }
  switch (angry) {

    case 1:

      keypadpass();
      if (timeflag == 0)
        timetoentry();
      else if (cmflag == 0)
      {
        msg = "timeflag triggered";
        cmflag = 1;

      }
      if (p == 1) {
        angry = 4;
        break;
      }

      angry = 2;
      break;

    case 2:

      checkrf();
      angry = 3;
      break;

    case 3:
      if (cm == 0)
        checkmagnsens();
      angry = 1;
      break;

    case 4:
      disarm();
      break;

    case 5:

      break;

    default:

      break;
  }
  if (msg != "") {
    Serial.println(msg);
    SendMessage();
    msg = "";
  }
  if (sim.available() > 0)
    Serial.write(sim.read());
  
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//void functions
/////////////////////////////////////////////////////////////////////////////////////////
void SendMessage()
{
  //Serial.println ("Sending Message");
  sim.println("AT+CMGF=1");    //Sets the GSM Module in Text Mode
  delay(1000);
  //Serial.println ("Set SMS Number");
  sim.println("AT+CMGS=\"" + number + "\"\r"); //Mobile phone number to send message
 

  delay(1000);
  String SMS = "";
  sim.println(msg);// allaksa to "SMS" me "msg"
  delay(100);
  sim.println((char)26);// ASCII code of CTRL+Z
  delay(1000);
  _buffer = _readSerial();
}

String _readSerial() {
  _timeout = 0;
  while  (!sim.available() && _timeout < 12000  )
  {
    delay(13);
    _timeout++;
  }
  if (sim.available()) {
    return sim.readString();
  }
}


void keypadpass () {
  char key = keypad.getKey();

  if (key) {
    Serial.println(key);

    if (key == '*') {
      input_password = ""; // clear input password
    } else if (key == '#') {
      if (password == input_password) {
        Serial.println("password is correct");
        p = 1;
        if (systemArm == 0) {
          Serial.println("system is calibrating");
          clockforhelp = millis();
        }
        else {
          angry = 4;

        }
      } else {
        Serial.println("password is incorrect, try again");
      }

      input_password = ""; // clear input password
    } else {
      input_password += key; // append new character to input password string
    }
  }
}

void timetoexit() {

  if ((currentTime - clockforhelp >= leavetime) && (systemArm == 0)) {
    systemArm = 1;
    p = 0;

    input_password = "";
    alarmsystem = 0;
    rst = 1;
    cmflag = 0;
    crflag = 0;
    //digitalWrite (cmLed,LOW);
    //digitalWrite (crLed,LOW);

    Serial.println("system is on");

  }


}

void timetoentry () {


  if ((millis() - intruder) >= breaktime)   {
    timeflag = 1;

  }
}




void checkmagnsens () {
  cm = 0;
  statemagn = digitalRead (sensormagn);
  if (statemagn == 1) {
    intruder = millis();
    cm = 1;

  }
  else {

    delay(20);
  }
}


void checkrf () {

  uint8_t buf[5];
  uint8_t buflen = sizeof(buf);
  if (driver.recv(buf, &buflen)) // Non-blocking
  { if (buf[0] == prog1[0] && buf[1] == prog1[1] && buf[2] == prog1[2] ) {
      msg = "door is open but no alarm";
      if (systemArm == 1)
        msg = "door is open but yes alarm";//steile minima sto kinito
    }
    if (buf[0] == prog2[0] && buf[1] == prog2[1] && buf[2] == prog2[2] )
      msg = "closedoor";//kleise led
    if (buf[0] == prog3[0] && buf[1] == prog3[1] && buf[2] == prog3[2] )
      msg = "battery";

  }
}

void disarm() {

  calm = 1;
  angry = 0;
  cm = 0;
  cmflag = 0;
  cr = 0;
  crflag = 0;

  timeflag = 0;
  p = 0;
  systemArm = 0;
  statemagn = 0;
  a = 0;

  Serial.println("system has been disarmed");
}
