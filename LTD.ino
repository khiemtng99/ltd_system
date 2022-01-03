#include <DHT.h>
#include <Ticker.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define DHT_PIN 14
#define BUTTON1 4
#define BUTTON2 5
#define HEATER 1
#define HEAT_PUMP 3
#define FAN1 15
#define FAN2 13
#define FAN3 12

LiquidCrystal_I2C lcd(0x27, 16, 2);
Ticker timer;
DHT dht(DHT_PIN, DHT11);

byte degree[8] = {
  0B01110,
  0B01010,
  0B01110,
  0B00000,
  0B00000,
  0B00000,
  0B00000,
  0B00000
};

float humidity = 0;
float temperature = 0;

float maxHumidity = 0;
float maxTemperature = 0;

int firstReadButton1 = 0;
int secondReadButton1 = 0;
int firstReadButton2 = 0;
int secondReadButton2 = 0;

int Mode = 0;
int fButton2 = 0;
int fButton2Pressed1s = 0;
int count1 = 0;
int count2 = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(2, 0);

  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);

  pinMode(HEATER, OUTPUT);
  pinMode(HEAT_PUMP, OUTPUT);
  pinMode(FAN1, OUTPUT);
  pinMode(FAN2, OUTPUT);
  pinMode(FAN3, OUTPUT);

  lcd.begin(16, 2);
  lcd.init();
  lcd.backlight();
  lcd.createChar(1, degree);
  
  dht.begin();

  timer.attach_ms(10, ISR);
}

void loop() {
  readDHT();
  button2State();
  LCDMode();
  LTD();
}

void ISR() {
  readButton1();
  readButton2();
}

void readDHT() {
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
}

void readButton1() {
  firstReadButton1 = secondReadButton1;
  secondReadButton1 = digitalRead(BUTTON1);
  if(secondReadButton1 != firstReadButton1) {
    if(firstReadButton1) {
      Mode++;
      if(Mode > 2)
        Mode = 0;
    }
  }
}

void readButton2() {
  firstReadButton2 = secondReadButton2;
  secondReadButton2 = digitalRead(BUTTON2);
  if(secondReadButton2 == firstReadButton2) {
    if(firstReadButton2) {
      fButton2 = 1;
      count1++;
      if(count1 > 50) {
        fButton2Pressed1s = 1;
        count2++;
        if(count2 > 10)
          count2 = 0;
      }
    }
    else {
      fButton2 = 0;
      fButton2Pressed1s = 0;
      count1 = 0;
      count2 = 0;
    }
  }
}

enum {
  STATE0,
  STATE1,
  STATE2
} eState;

void button2State() {
  switch(eState) {
    case STATE0:
      if(fButton2) {
        increaseValue();
        eState = STATE1;
      }
    break;
    case STATE1:
      if(!fButton2)
        eState = STATE0;
      else if(fButton2Pressed1s)
        eState = STATE2;
    break;
    case STATE2:
      if(count2 == 10)
        increaseValue();
      if(!fButton2)
        eState = STATE0;
    break;
  }
}

void increaseValue() {
  if(Mode == 1) {
    maxTemperature++;
    if(maxTemperature > 100)
      maxTemperature = 0;
    lcd.setCursor(10, 0);
    if((int)maxTemperature < 10)
      lcd.print("  ");
    else if((int)maxTemperature < 100)
      lcd.print(" ");
    lcd.print((int)maxTemperature);
  }
  if(Mode == 2) {
    maxHumidity++;
    if(maxHumidity > 100)
      maxHumidity = 0;
    lcd.setCursor(9, 0);
    if((int)maxHumidity < 10)
      lcd.print("  ");
    else if((int)maxHumidity < 100)
      lcd.print(" ");
    lcd.print((int)maxHumidity);
  }
}

void displayMode() {
  switch(Mode) {
    case 0:
      if (isnan(humidity) || isnan(temperature))
        Serial.println("Failed to read from DHT sensor!");
      else {
        lcd.setCursor(0, 0);
        lcd.print("T: ");
        if(temperature < 10)
          lcd.print("  ");
        else if(temperature < 100)
          lcd.print(" ");
        lcd.print(temperature);
        lcd.write(1);
        lcd.print("C");

        lcd.setCursor(0, 1);
        lcd.print("H: ");
        if(humidity < 10)
          lcd.print("  ");
        else if(humidity < 100)
          lcd.print(" ");
        lcd.print(humidity);
        lcd.print("%"); 
      }
    break;
    case 1:
      lcd.setCursor(0, 0);
      lcd.print("MaxTemp = ");
      if((int)maxTemperature < 10)
        lcd.print("  ");
      else if((int)maxTemperature < 100)
        lcd.print(" ");
      lcd.print((int)maxTemperature);
      lcd.write(1);
      lcd.print("C");
    break;
    case 2:
      lcd.setCursor(0, 0);
      lcd.print("MaxHum = ");
      if((int)maxHumidity < 10)
        lcd.print("  ");
      else if((int)maxHumidity < 100)
        lcd.print(" ");
      lcd.print((int)maxHumidity);
      lcd.print("%");
    break;
  }
}

enum {
  MODE0,
  MODE1,
  MODE2
} eMode;

void LCDMode() {
  switch(eMode) {
    case MODE0:
      displayMode();
      if(Mode == 1) {
        lcd.clear();
        eMode = MODE1;
      }
    break;
    case MODE1:
      displayMode();
      if(Mode == 2) {
        lcd.clear();
        eMode = MODE2;
      }
    break;
    case MODE2:
      displayMode();
      if(Mode == 0) {
        lcd.clear();
        eMode = MODE0;
      }
    break;
  }
}

void LTD() {
  if(temperature < maxTemperature) {
    digitalWrite(HEATER, HIGH);
    digitalWrite(FAN2, HIGH);
    digitalWrite(HEAT_PUMP, LOW);
    digitalWrite(FAN3, LOW);
  }
  else {
    digitalWrite(HEATER, LOW);
    digitalWrite(FAN2, LOW);
    digitalWrite(HEAT_PUMP, HIGH);
    digitalWrite(FAN3, HIGH);
  }

  if(humidity > maxHumidity) {
    if(maxHumidity >= 90) {
      analogWrite(FAN1, 255);
    }
    else if(humidity >= 80) {
      analogWrite(FAN1, 204);
    }
    else if(humidity >= 70) {
      analogWrite(FAN1, 178);
    }
    else if(humidity >= 60) {
      analogWrite(FAN1, 153);
    }
    else if(humidity >= 50) {
      analogWrite(FAN1, 127);
    }
    else if(humidity >= 40) {
      analogWrite(FAN1, 102);
    }
    else if(humidity >= 30) {
      analogWrite(FAN1, 76);
    }
    else if(humidity >= 20) {
      analogWrite(FAN1, 51);
    }
    else if(humidity >= 10) {
      analogWrite(FAN1, 25);
    }
    else analogWrite(FAN1, 0);
  }
  else analogWrite(FAN1, 0);
}
