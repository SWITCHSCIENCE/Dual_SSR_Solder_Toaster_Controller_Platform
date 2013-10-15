
#define allOFF 0
#define upON   1
#define downON 2
#define allON  3
#define ControlDataLen 3
int temperature_control_data[ControlDataLen + 1][3] = {
  {downON, 130, 15},  // 2ON   , temperature130, keep15sec
  {allON , 230,  0},  // 1&2ON , temperature230, keep0sec
  {downON, 225,100},  // 2ON   , temperature225, keep100sec
  {allOFF,   0,  0}   // 1&2OFF, temperature0  , keep0sec
};

#include <LiquidCrystal.h>
#include <SPI.h>

// LCD(D2-D7)
#define LCDrsPin 2
#define LCDenablePin 3
#define LCDd4Pin 4
#define LCDd5Pin 5
#define LCDd6Pin 6
#define LCDd7Pin 7
// button(D8)
#define StartButton 8
// Beep(D9)
#define TonePin 9
// Tempratier(D10,D12-D13)
#define TemperatureSlavePin 10
#define TemperatureMisoPin 12
#define TemperatureSckPin 13
// PowerControl(A0,A1)
#define Heat1Pin 14
#define Heat2Pin 15

LiquidCrystal lcd(LCDrsPin,LCDenablePin,LCDd4Pin,LCDd5Pin,LCDd6Pin,LCDd7Pin);

#define delayWait 100
#define oneSec (1000 / delayWait)
byte state;          // main program mode
byte heatMode;       // UpDown heater mode
byte heatState;      // UpDown heater status
byte tableCounter;   // data table counter
int temperatureWait;  // temprature keep time(SEC)
float temperature;    // Temperature
float temperatureMax; // target Temprature
int blinkTimer;      // blink timer
boolean blinkFlag;   // blink ON/OFF flag

void setup() {
  // degug Initialize(SerialMonitor)
  Serial.begin(9600);
  // LCD initialize
  lcd.begin(20, 4);
  // button initialize
  pinMode(StartButton, INPUT_PULLUP);
  // PowerControl initialize
  pinMode(Heat1Pin, OUTPUT);
  pinMode(Heat2Pin, OUTPUT);
  // Temprature initialize
  pinMode(TemperatureSlavePin, OUTPUT);
  digitalWrite(TemperatureSlavePin, HIGH);
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV4);
  SPI.setDataMode(SPI_MODE0);
  // memory initialize
  state = 0;
}

void loop() {
  tempratureRead();
  switch (state) {
    case 0: // initialize
      lcd.clear();
      heatMode = 0;
      temperatureMax = 0;
      tableCounter = 0;
      state++;
      break;
    case 1: // start switch wait
      if (digitalRead(StartButton) == LOW) {
        tone(TonePin,600,800);  // StartSound
        lcd.clear();
        setTempratureData();
        state++;
      }
      break;
    case 2: // target Temperature
      if (temperatureMax <= temperature) {
        state++;
      }
      break;
    case 3: // keep time
      if (--temperatureWait <= 0) {
        state++;
      }
      break;
    case 4: // Loop or Finish?
      tableCounter++;
      setTempratureData();
      if (tableCounter < ControlDataLen) {
        state = 2;
      } else {
        tone(TonePin,600,1500);  // FinishSound
        state++;
      }
      break;
    case 5: // finish switch wait
      if (digitalRead(StartButton) == LOW) {
        state = 0;
      }
      break;
  }
  heatControl();
  lcdDisplay();
  delay(delayWait);
}

void setTempratureData() {
  heatMode = temperature_control_data[tableCounter][0];
  temperatureMax = temperature_control_data[tableCounter][1];
  temperatureWait = temperature_control_data[tableCounter][2] * oneSec;
  heatState = heatMode;
}

void tempratureRead() {
  unsigned int thermocouple;
  unsigned int internal;
  float disp;
  // read tem
  digitalWrite(TemperatureSlavePin, LOW);
  thermocouple = (unsigned int)SPI.transfer(0x00) << 8;
  thermocouple |= (unsigned int)SPI.transfer(0x00);
  internal = (unsigned int)SPI.transfer(0x00) << 8;
  internal |= (unsigned int)SPI.transfer(0x00);
  digitalWrite(TemperatureSlavePin, HIGH);
  if ((thermocouple & 0x0001) != 0) {
    Serial.print("ERROR: ");
    if ((internal & 0x0004) !=0) {
      Serial.print("Short to Vcc, ");
    }
    if ((internal & 0x0002) !=0) {
      Serial.print("Short to GND, ");
    }
    if ((internal & 0x0001) !=0) {
      Serial.print("Open Circuit, ");
    }    
    Serial.println();
  } else {
    if ((thermocouple & 0x8000) == 0) {
      temperature = (thermocouple >> 2) * 0.25;
    } else {
      temperature = (0x3fff - (thermocouple >> 2) + 1)  * -0.25;
    }
  }
}

void heatControl() {
  if (temperature > temperatureMax) {
    heatState = 0;
  } else if (temperature < (temperatureMax - 0.5)) {
    heatState = heatMode;
  }
  if ((heatState & 1) == 0) {
    digitalWrite(Heat1Pin, LOW);
  } else {
    digitalWrite(Heat1Pin, HIGH);
  }
  if ((heatState & 2) == 0) {
    digitalWrite(Heat2Pin, LOW);
  } else {
    digitalWrite(Heat2Pin, HIGH);
  }
}

void lcdDisplay() {
  lcd.setCursor(3, 0);
  lcd.print("STATUS:");
  switch (state) {
    case 0: // initialize
    case 1: // start switch wait
      lcd.print("-------");
      lcd.setCursor(1, 1);
      if (blinkFlag == true) {
        lcd.print("press START button");
      } else {
        lcd.print("                  ");
      }
      lcd.setCursor(3, 3);
      lcd.print("SWITCH SCIENCE");
      break;
    case 2: // target Temperature
    case 3: // keep time
    case 4: // Loop or Finish?
    case 5: // finish switch wait
      if (state != 5) {
        if (blinkFlag == true) {
          lcd.print("RUNNING");
        } else {
          lcd.print("       ");
        }
      } else {
        lcd.print("FINISH!");
      }
      lcd.setCursor(0, 1);
      if ((heatState & 1) == 0) {
        lcd.print("HEAT1:OFF  ");
      } else {
        lcd.print("HEAT1:ON   ");
      }
      if ((heatState & 2) == 0) {
        lcd.print("HEAT2:OFF");
      } else {
        lcd.print("HEAT2:ON ");
      }
      lcd.setCursor(5, 3);
      lcd.print("WAIT:");
      if (state == 3) {
        lcd.print(temperatureWait / oneSec);
        lcd.print(".");
        lcd.print(temperatureWait % oneSec * 10 / oneSec);
        lcd.print("sec");
      } else {
        lcd.print("---.-  ");
      }
      lcd.print("  ");
      break;
  }
  lcd.setCursor(2, 2);
  if (temperature < 100.0) lcd.print(" ");
  if (temperature < 10.0) lcd.print(" ");
  lcd.print(temperature);
  lcd.print(" / ");
  lcd.print(temperatureMax);
  lcd.print("  ");
  // blink control
  if (++blinkTimer >= oneSec) {
    blinkTimer = 0;
    if (blinkFlag == false) {
      blinkFlag = true;
    } else {
      blinkFlag = false;
    }
  }
}

