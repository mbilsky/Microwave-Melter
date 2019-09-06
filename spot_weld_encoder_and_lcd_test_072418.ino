#define WITH_LCD 0

#include <ClickEncoder.h>
#include <TimerOne.h>

#ifdef WITH_LCD
//YWROBOT
//Compatible with the Arduino IDE 1.0
//Library version:1.1
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display
#endif

ClickEncoder *encoder;
int16_t last, value;

void timerIsr() {
  encoder->service();
}

#ifdef WITH_LCD
void displayAccelerationStatus() {
  lcd.setCursor(0, 1);
  lcd.print("Acceleration ");
  //lcd.print(encoder->getAccelerationEnabled() ? "on " : "off");
}
#endif

//variables for the rest of the program
//use a pwm pin for the solid state relay in case we want to enable that feature
int relay = 3;

//trigger switch
int trigger = 4;

long triggerTime = 0;

//button light
int light = 5;

//variable for the state change
int state = 0;

//variables for the pulse properties
int pulseLength = 2500;

int pulseDuty = 100;//percent

//store if the system is armed
int armed = 0;

//buffers for lcd
char lenBuf[6];
char dutyBuf[4];

//variable for storing the blink rate
int blinkRate = 300;

//store the last time of the blink
long lastBlink = 0;

//variable for the blink state
int blinkState = 0;

void setup() {
 // Serial.begin(9600);
  encoder = new ClickEncoder(A1, A0, A2);

  // initialize the lcd
  lcd.init();
  // Print a message to the LCD.
  lcd.backlight();
  lcd.clear();

  //setup the duration printing
  lcd.setCursor(0, 0);
  lcd.print("Duration:");
  //use a char array buffer to hold the spaces

  sprintf(lenBuf, "%05d", pulseLength);
  lcd.print(lenBuf);
  lcd.setCursor(14, 0);
  lcd.print("mS");


  //setup the duty cycle printing
  lcd.setCursor(0, 1);
  lcd.print("Duty:");
  sprintf(dutyBuf, "%03d", pulseDuty);
  lcd.print(dutyBuf);

  //setup the armed
  lcd.print("% Arm?");

  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr);

  last = -1;



  //setup the pins for the inputs/outputs
  pinMode(relay, OUTPUT);
  pinMode(trigger, INPUT);
  pinMode(light, OUTPUT);


  //set the value to the duration (initialize encoder stuff)
  value = 0;
  last = value;
}

void loop() {

  //these are the things to always do regardless of the state

  //handle the blinking of the text and lights
  if (millis() - lastBlink > blinkRate) {
    lastBlink = millis();
    blinkState = !blinkState;
  }

  //read the encoder
  value += encoder->getValue();


  switch (state) {

    case 0:
      //turn off the big red button light
      digitalWrite(light, 0);



      //deal with changing the value.
      if (value != last) {
        last = value;
        pulseLength += value * 5;
        value = 0;
        if (pulseLength < 0) pulseLength = 0;
        else if (pulseLength > 99999) pulseLength = 99999;
      }



      //flash the duration value
      if (blinkState) {
        lcd.setCursor(9, 0);
        sprintf(lenBuf, "%05d", pulseLength);
        lcd.print(lenBuf);
        lcd.setCursor(14, 0);
        lcd.print("mS");
      }
      else {
        lcd.setCursor(9, 0);
        lcd.print("       ");

      }

      break;

    //set the duty cycle
    case 1:

      // deal with changing the value.
      if (value != last) {
        last = value;
        pulseDuty += value;
        value = 0;
        if (pulseDuty < 0) pulseDuty = 0;
        else if (pulseDuty > 100) pulseDuty = 100;
      }



      //flash the duration value
      if (blinkState) {
        lcd.setCursor(5, 1);
        sprintf(dutyBuf, "%03d", pulseDuty);
        lcd.print(dutyBuf);
        lcd.print("%");
      }
      else {
        lcd.setCursor(5, 1);
        lcd.print("     ");

      }


      break;

    //enable the arming
    case 2:


      //flash the duration value
      if (blinkState) {
        lcd.setCursor(10, 1);

        lcd.print("Arm?  ");
      }
      else {
        lcd.setCursor(10, 1);
        lcd.print("      ");

      }

      break;


    //armed and ready
    case 3:

      lcd.setCursor(10, 1);
      lcd.print("ARMED!");



      //blink the button until it is pressed
      if (blinkState) digitalWrite(light, 1);
      else digitalWrite(light, 0);



      //wait for the button to be pressed and held for 3 second
      if (digitalRead(trigger)) {
        triggerTime = millis();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Hold Button For");
        int localFlag = 1;
        while (digitalRead(trigger)) {
          if (millis() - triggerTime < 3000) {
            lcd.setCursor(0, 1);
            lcd.print("    ");
            lcd.setCursor(0, 1);
            lcd.print(3000 - (millis() - triggerTime));
            lcd.setCursor(6, 1);
            lcd.print("more mS");
          
          }
          else {

            //only do this once
            if (localFlag) {
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Release To Fire!");
              localFlag = 0;
            }
          }



        }
        if (millis() - triggerTime > 3000) {

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Firing now!");

          delay(500);




          //we held the button
          int pwmVal = map(pulseDuty, 0, 100, 0, 255);

          //fire the relay
          analogWrite(relay, pwmVal);
          //and turn on the button light
          digitalWrite(light, 1);

          //delay the specified time
          delay(pulseLength);

          //then turn it off twice to be safe
          digitalWrite(relay, 0);
          delay(5);
          digitalWrite(relay, 0);
          digitalWrite(light, 0);

          //then return to state 0;
          state = 0;

          //reset the lcd
          lcd.clear();

          //setup the duration printing
          lcd.setCursor(0, 0);
          lcd.print("Duration:");
          //use a char array buffer to hold the spaces

          sprintf(lenBuf, "%05d", pulseLength);
          lcd.print(lenBuf);
          lcd.setCursor(14, 0);
          lcd.print("mS");


          //setup the duty cycle printing
          lcd.setCursor(0, 1);
          lcd.print("Duty:");
          sprintf(dutyBuf, "%03d", pulseDuty);
          lcd.print(dutyBuf);

          //setup the armed
          lcd.print("% Arm?  ");

        }
        else {

          //reset the lcd
          lcd.clear();

          //setup the duration printing
          lcd.setCursor(0, 0);
          lcd.print("Duration:");
          //use a char array buffer to hold the spaces

          sprintf(lenBuf, "%05d", pulseLength);
          lcd.print(lenBuf);
          lcd.setCursor(14, 0);
          lcd.print("mS");


          //setup the duty cycle printing
          lcd.setCursor(0, 1);
          lcd.print("Duty:");
          sprintf(dutyBuf, "%03d", pulseDuty);
          lcd.print(dutyBuf);

          //setup the armed
          lcd.print("% ARMED!");
        }
      }



      break;





  }


  //help with this code: https://arduino.stackexchange.com/questions/21601/how-do-i-statements-for-the-various-conditions-of-this-function
  ClickEncoder::Button b = encoder->getButton();
  if (b != ClickEncoder::Open) {
   // Serial.print("Button: ");
//#define VERBOSECASE(label) case label: Serial.println(#label); break;
    switch (b) {
      //  VERBOSECASE(ClickEncoder::Pressed);
      //  VERBOSECASE(ClickEncoder::Held)
      //  VERBOSECASE(ClickEncoder::Released)

      case ClickEncoder::Clicked:
        state++;
        if (state > 3) state = 0;
        //also reset the value
        value = 0;






        //make sure that the lcd has the most current information
        //pulse Length
        lcd.setCursor(9, 0);
        sprintf(lenBuf, "%05d", pulseLength);
        lcd.print(lenBuf);
        lcd.setCursor(14, 0);
        lcd.print("mS");

        //duty
        lcd.setCursor(5, 1);
        sprintf(dutyBuf, "%03d", pulseDuty);
        lcd.print(dutyBuf);
        lcd.print("%");

        break;


//      case ClickEncoder::DoubleClicked:
//        Serial.println("ClickEncoder::DoubleClicked");
//        encoder->setAccelerationEnabled(!encoder->getAccelerationEnabled());
//        Serial.print("  Acceleration is ");
//        Serial.println((encoder->getAccelerationEnabled()) ? "enabled" : "disabled");
//#ifdef WITH_LCD
//        displayAccelerationStatus();
//#endif
//        break;
    }
  }
}

