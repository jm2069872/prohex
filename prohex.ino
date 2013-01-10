/* 
  ProHex firmware for HexBright FLEX
  v0.1  Jan 09, 2012
  Website: http://justinmrkva.github.com/prohex/
  Please consider donating!
  
  This firmware is provided for free and no warranty is supplied or implied.
  You may modify for your own use without restrictions.
  You may distribute copies, original or modified, ONLY if you include this original header.
  If you distribute any modified versions, please list your modifications below.
  Above all, have fun hacking your HexBright!
  
  ACKNOWLEDGEMENTS
  Some of this code, especially temperature, charging, and setup blocks, are sourced from the 
  HexBright example libraries at http://github.com/hexbright - without them, this project may
  not have been possible. Special thanks to http://www.hexbright.com for producing the light
  in the first place!
*/



// USER PREFERENCES
#define USR_LOW_POWER         0.2
#define USR_MED_POWER         0.6
#define USR_HIGH_POWER        1.0
#define USR_BLINK_POWER       0.6
#define USR_BLINK_TIME        10
#define USR_BLINK_WAVELENGTH  750



// Advanced Settings
#define OVERTEMP                340
#define CRIT_TEMP               350

#define ABSOLUTE_MIN            8
#define FADE_INCREMENT          0.001

#define BTN_CLICK_SENSE         20
#define BTN_HOLD_SENSE          500
#define BTN_LONG_HOLD_SENSE     3000



// Pins
#define DPIN_RLED_SW            2
#define DPIN_GLED               5
#define DPIN_PWR                8
#define DPIN_DRV_MODE           9
#define DPIN_DRV_EN             10
#define APIN_TEMP               0
#define APIN_CHARGE             3
// Modes
#define MODE_OFF                0
#define MODE_LOW                1
#define MODE_MED                2
#define MODE_HIGH               3
#define MODE_BLINKING           4



// Includes
#include <math.h>
#include <Wire.h>



// State Information
byte mode = MODE_OFF;
float brightness = 0.0;

// Button Tracking (internal for button functions)
unsigned long btnDnTime = 0;
unsigned long btnUpTime = 0;
boolean btnDn = false;
boolean btnUp = false;
boolean btnAck = false;
byte btnDownMode = MODE_OFF;

// Button status
boolean btnIsBeingHeld = false;
boolean btnIsBeingLongHeld = false;
boolean btnWasClicked = false;
boolean btnWasHeld = false;
boolean btnWasLongHeld = false;



void setup()
{
  // We just powered on!  That means either we got plugged 
  // into USB, or the user is pressing the power button.
  pinMode(DPIN_PWR, INPUT);
  digitalWrite(DPIN_PWR, LOW);

  // Initialize GPIO
  pinMode(DPIN_RLED_SW, INPUT);
  pinMode(DPIN_GLED, OUTPUT);
  pinMode(DPIN_DRV_MODE, OUTPUT);
  pinMode(DPIN_DRV_EN, OUTPUT);
  digitalWrite(DPIN_DRV_MODE, LOW);
  digitalWrite(DPIN_DRV_EN, LOW);
  
  // Initialize serial busses
  //Serial.begin(9600); // for debugging only
  Wire.begin();
  
  // redo this just in case
  resetBtn();
  mode = MODE_OFF;
}

void loop()
{
  // IMPORTANT
  // Time each operation; during fades, etc. cancel or minimize unnecessary operations
  
  // standard variables used
  unsigned long time = millis(); // rollover time is long, don't have to concern ourselves
  
  // Check the state of the charge controller
  int chargeState = analogRead(APIN_CHARGE);
  if (chargeState < 128) {
    // Low - charging
    digitalWrite(DPIN_GLED, (time&0x0100)?LOW:HIGH);
  } else if (chargeState > 768) {
    // High - charged
    digitalWrite(DPIN_GLED, HIGH);
  } else {
    // Hi-Z - shutdown
    digitalWrite(DPIN_GLED, LOW);
  }
  
  // check temp sensor
  // Test efficiency of both checking every time and of doing every second
  // Temporary values, use true mode changing
  int temperature = analogRead(APIN_TEMP);
  if (temperature > CRIT_TEMP) {
    pinMode(DPIN_PWR, OUTPUT);
    digitalWrite(DPIN_PWR, LOW);
    digitalWrite(DPIN_DRV_MODE, LOW);
    digitalWrite(DPIN_DRV_EN, LOW);
  } else if (temperature > OVERTEMP && (mode != MODE_LOW && mode != MODE_OFF)) {
    // disable MOSFET
    digitalWrite(DPIN_DRV_MODE, LOW);
    // flash to warn, then switch to low
    for (int i = 0; i < 6; i++)
    {
      digitalWrite(DPIN_DRV_EN, LOW);
      delay(100);
      digitalWrite(DPIN_DRV_EN, HIGH);
      delay(100);
    }
    digitalWrite(DPIN_DRV_EN, LOW);
    brightness = 0;
    mode = MODE_LOW;
  }
  
  // periodically pull down the button's pin
  // in certain hardware revisions it can float
  pinMode(DPIN_RLED_SW, OUTPUT);
  pinMode(DPIN_RLED_SW, INPUT);
  
  // check button state
  senseBtn();
  
  // determine transition for button/mode
  byte newMode = mode;
  switch (mode) {
    case MODE_OFF:
      if (btnWasClicked) {
        latchPower();
        newMode = MODE_LOW;
        ackBtn();
      } else if (btnIsBeingHeld) {
        latchPower();
        newMode = MODE_BLINKING;
      } else if (btnWasHeld) {
        latchPower();
        newMode = MODE_BLINKING;
        ackBtn();
      }
      break;
    case MODE_LOW:
      if (btnWasClicked) {
        newMode = MODE_MED;
        ackBtn();
      } else if (btnIsBeingHeld || btnWasHeld) {
        newMode = MODE_OFF;
        ackBtn();
      }
      break;
    case MODE_MED:
      if (btnWasClicked) {
        newMode = MODE_HIGH;
        ackBtn();
      } else if (btnIsBeingHeld || btnWasHeld) {
        newMode = MODE_OFF;
        ackBtn();
      }
      break;
    case MODE_HIGH:
      if (btnWasClicked) {
        newMode = MODE_LOW;
        ackBtn();
      } else if (btnIsBeingHeld || btnWasHeld) {
        newMode = MODE_OFF;
        ackBtn();
      }
      break;
    case MODE_BLINKING:
      if (btnWasClicked) {
        newMode = MODE_OFF;
        ackBtn();
      } else if (btnIsBeingLongHeld || btnWasLongHeld) {
        newMode = MODE_OFF;
        ackBtn();
      } else if (btnDownMode == MODE_BLINKING && btnWasHeld) {
        newMode = MODE_OFF;
        ackBtn();
      }
      break;
  }
  
  // Do the mode
    // Remember to ack button once no more input is expected
  
  switch (newMode) {
    case MODE_OFF:
    fadeTo(0);
      if (brightness == 0) {
        pinMode(DPIN_PWR, OUTPUT);
        digitalWrite(DPIN_PWR, LOW);
      }
      break;
    case MODE_LOW:
      fadeTo(USR_LOW_POWER);
      break;
    case MODE_MED:
      fadeTo(USR_MED_POWER);
      break;
    case MODE_HIGH:
      fadeTo(USR_HIGH_POWER);
      break;
    case MODE_BLINKING:
      blinkTo(((time % USR_BLINK_WAVELENGTH) < USR_BLINK_TIME) ? USR_BLINK_POWER : 0);
      break;
  }
  mode = newMode;
}



// LIBRARY

void latchPower() {
  pinMode(DPIN_PWR, OUTPUT);
  digitalWrite(DPIN_PWR, HIGH);
}

void fadeTo(float val)
{
  // map values here
  float target = brightness;
  if (target > val) {
    target -= FADE_INCREMENT;
    if (target < val) target = val;
  } else if (target < val) {
    target += FADE_INCREMENT;
    if (target > val) target = val;
  }
  if (target != brightness) blinkTo(target);
}
    
void blinkTo(float val)
{
  if (val == 0) {
    digitalWrite(DPIN_DRV_MODE,LOW);
    analogWrite(DPIN_DRV_EN,0);
  } else {
    float strength = pow(val, 2);
    unsigned char setValue = (unsigned char)(strength * (255 - ABSOLUTE_MIN)) + ABSOLUTE_MIN;
    analogWrite(DPIN_DRV_EN, setValue);
    digitalWrite(DPIN_DRV_MODE, HIGH);
  }
  brightness = val;
}

void ackBtn() {
  btnAck = true;
  resetBtn();
}

void resetBtn()
{
  btnDn = false;
  btnUp = false;
  btnIsBeingHeld = false;
  btnIsBeingLongHeld = false;
  btnWasClicked = false;
  btnWasHeld = false;
  btnWasLongHeld = false;
}

// TIME this function's execution time, try to optimize?
boolean senseBtn()
{
  pinMode(DPIN_RLED_SW,INPUT);
  boolean buttonIsDepressed = digitalRead(DPIN_RLED_SW);
  if (btnAck && buttonIsDepressed) return false;
  if (btnAck && !buttonIsDepressed) btnAck = false;
  
  if (buttonIsDepressed && btnUp) resetBtn();
  
  if (!btnDn && buttonIsDepressed) { // mark the down point
    btnDn = true;
    btnDnTime = millis();
    btnDownMode = mode;
  } else if (btnDn && !btnUp && !buttonIsDepressed) { // mark the up point
    btnUp = true;
    btnUpTime = millis();
  }
  btnIsBeingHeld = false;
  btnIsBeingLongHeld = false;
  btnWasClicked = false;
  btnWasHeld = false;
  btnWasLongHeld = false;
  if (btnDn && !btnUp) {
    unsigned long timeDiff = millis() - btnDnTime;
    if (timeDiff >= BTN_HOLD_SENSE && timeDiff < BTN_LONG_HOLD_SENSE) {
      btnIsBeingHeld = true;
    } else if (timeDiff >= BTN_LONG_HOLD_SENSE) {
      btnIsBeingLongHeld = true;
    }
  }
  if (btnDn && btnUp) {
    unsigned long timeDiff = btnUpTime - btnDnTime;
    if (timeDiff >= BTN_CLICK_SENSE && timeDiff < BTN_HOLD_SENSE) {
      btnWasClicked = true;
    } else if (timeDiff >= BTN_HOLD_SENSE && timeDiff < BTN_LONG_HOLD_SENSE) {
      btnWasHeld = true;
    } else if (timeDiff >= BTN_LONG_HOLD_SENSE) {
      btnWasLongHeld = true;
    }
  }
  return buttonIsDepressed;
}
