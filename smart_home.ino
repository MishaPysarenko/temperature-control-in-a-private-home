#include <string.h>

#include <avr/eeprom.h>

#include <DS_raw.h>
#include <microDS18B20.h>
#include <microOneWire.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <GyverEncoder.h>
#define CLK A1
#define DT A2
#define SW A3

#include  <iarduino_RTC.h>

#define floor_pump 2
#define electric_boiler 3
#define electric_valve 4
#define battery_pump 5

uint8_t addr1[] ={0x28, 0xFF, 0x64, 0xE, 0x75, 0xF,  0x64, 0x95};
uint8_t addr2[] ={0x28, 0xFF, 0x64, 0xE, 0x69, 0xDB, 0x32, 0x1 };
uint8_t addr3[] ={0x28, 0xFF, 0x64, 0xE, 0x6F, 0x6B, 0xAE, 0x60}; // 0 0 0
uint8_t addr4[] ={0x28, 0xFF, 0x64, 0xE, 0x75, 0x33, 0x72, 0xB5}; // 1 0 0
uint8_t addr5[] ={0x28, 0xFF, 0x64, 0xE, 0x6F, 0x66, 0xA1, 0xA8}; // 0 1 0
uint8_t addr6[] ={0x28, 0xFF, 0x64, 0xE, 0x6B, 0x94, 0x9A, 0xA0}; // 0 0 1

MicroDS18B20 <A0, addr1> D1;
MicroDS18B20 <A0, addr2> D2;
MicroDS18B20 <A0, addr3> D3;
MicroDS18B20 <A0, addr4> D4;
MicroDS18B20 <A0, addr5> D5;
MicroDS18B20 <A0, addr6> D6;

struct DATA
{
  int user_temp = 10;
  int minHour = 0;
  int maxHour = 24;
};

LiquidCrystal_I2C lcd(0x27,16,2);  // Устанавливаем дисплей   A4 - SDA : A5 - SCL

Encoder enc(CLK, DT, SW);

iarduino_RTC clock(RTC_DS1307); 

short int temp1 = -1,temp2 = -1, temp3 = -1, temp4 = -1, temp5 = -1, temp6 = -1;
short int select_menu = 1;

DATA data;

void showUserTime(){
  lcd.setCursor(0, 1);
  lcd.print(data.minHour);
  lcd.setCursor(2, 1);
  lcd.print(":");
  lcd.print("00");
  lcd.setCursor(5, 1);
  lcd.print(" - ");
  lcd.print(data.maxHour);
  lcd.setCursor(10, 1);
  lcd.print(":");
  lcd.print("00");
}

void allRequestTemp(){
  D1.requestTemp();
  D2.requestTemp();
  D3.requestTemp();
  D4.requestTemp();
  D5.requestTemp();
  D6.requestTemp();
}

void allGetTemp(){
  if(D1.readTemp())temp1 = D1.getTemp();
  if(D2.readTemp())temp2 = D2.getTemp();
  if(D3.readTemp())temp3 = D3.getTemp();
  if(D4.readTemp())temp4 = D4.getTemp();
  if(D5.readTemp())temp5 = D5.getTemp();
  if(D6.readTemp())temp6 = D6.getTemp();
}
//D1 - датчик темп в кухне
//D2 - датчик темп в спальне 
//D3 - датчик темп пола в кухне
//D4 - датчик темп пола в спальне
//D5 - датчик темп дров котла
//D6 - датчик темп эл котла
//если на D3 & D4 температура больше 27, остановить насос при любых обстоятельствах
//нагрев и работа насоса пока не будет температуры соотвествии с установленой на D1 //& D2
//если на D5 болше 35 и D6 больше 35 отключить нагрев эл котла, работает только насос
//если на датчике 6 больше 55 включить насос на батареи 
void mainLogic(){
  if(millis() % 900000){
    lcd.noDisplay();
    lcd.display();
  }
  allRequestTemp();
  allGetTemp();
  String H = clock.gettime("H"), i = clock.gettime("i");
  short int timeH = H.toInt(), timeM = i.toInt();
  //работа эл котла по времени 
  if((data.minHour <= timeH) && (data.maxHour >= timeH) && (temp6  < 50))
  {
    //нагрев и работа насоса пока не будет температуры соотвествии с установленой на D1 //& D2
    if((temp1 < data.user_temp))//включить насос и эл котел
    {
      digitalWrite(electric_boiler, HIGH);
      digitalWrite(electric_valve, LOW);
    }
    else
    {
      digitalWrite(electric_boiler, LOW);
      digitalWrite(electric_valve, HIGH);
    }

    //если на D5 болше 35 и D6 больше 35 отключить нагрев эл котла, работает только насос
    if(temp5 > 35)
    {
      digitalWrite(electric_boiler, LOW);
      digitalWrite(electric_valve, HIGH);
    }
    else
    {
      digitalWrite(electric_boiler, HIGH);
      digitalWrite(electric_valve, LOW);
    }
    
  }
  else
  {
    digitalWrite(electric_boiler, LOW);
    digitalWrite(electric_valve, HIGH);
  }

  if(temp1 < data.user_temp)
  {
    digitalWrite(floor_pump, HIGH); 
  }
  else
  {
    digitalWrite(floor_pump, LOW);
  }

  //если на D3 & D4 температура больше 27, остановить насос при любых обстоятельствах
  if((temp3 >= 27) || (temp4 >= 27) || (temp6 >= 45))//выключить насос
  {
    digitalWrite(floor_pump, LOW); 
  }

  //если на датчике 5 больше 55 включить насос на батареи 
  digitalWrite(battery_pump, LOW);
  if(temp5 > 55)
  {
    digitalWrite(battery_pump, HIGH); 
  }  
  else
  {
    digitalWrite(battery_pump, LOW);
  }

}

void drawMenu(){
  if(select_menu == 1) // основное меню
  {
    if((millis() % 5000) < 100)lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("K ");
    lcd.print(temp1);
    lcd.print("C ");
    lcd.setCursor(6, 0);
    lcd.print(temp3);
    lcd.print("C ");
    lcd.setCursor(10, 0);
    lcd.print("D ");
    lcd.print(temp5);
    lcd.print("C ");    

    lcd.setCursor(0, 1);
    lcd.print("R ");
    lcd.print(temp2);
    lcd.print("C ");
    lcd.setCursor(6, 1);
    lcd.print(temp4);
    lcd.print("C ");

    lcd.setCursor(10, 1);
    lcd.print(clock.gettime("H:i"));
  }
  else if(select_menu == 2) //термостат
  {
    lcd.clear();
    do{
      enc.tick();
      if((millis() % 5000) < 100)mainLogic();
      lcd.setCursor(0, 0);
      lcd.print("termo stat ");
      lcd.setCursor(0, 1);
      lcd.print(data.user_temp);
      if(enc.isRight())
      {
        data.user_temp++;
        if(data.user_temp > 25)data.user_temp = 25;
        eeprom_update_block((void*)&data,0,sizeof(data));
        lcd.clear();
      }
      if(enc.isLeft())
      {
        data.user_temp--;
        if(data.user_temp < 10)data.user_temp = 10;
        eeprom_update_block((void*)&data,0,sizeof(data));
        lcd.clear();
      }
    }while(!(enc.isClick()));
    select_menu++;
  }
  else if(select_menu == 3) // 
  {
    lcd.clear();
    showUserTime();
    do{
      if((millis() % 5000) < 100)mainLogic();
      lcd.setCursor(0, 0);
      lcd.print("working time 1/2");
      lcd.setCursor(0, 1);
      enc.tick();
      if(enc.isTurn())
      {
        if(enc.isRight() || enc.isRightH() || enc.isFastR()){ 
          data.minHour++;
          if(data.minHour > 24)data.minHour = 0;
          eeprom_update_block((void*)&data,0,sizeof(data));  
          lcd.clear();
          showUserTime();
        }
        else if(enc.isLeft() || enc.isLeftH() || enc.isFastL() ){
          data.minHour--;
          if(data.minHour < 0)data.minHour = 24;
          eeprom_update_block((void*)&data,0,sizeof(data)); 
          lcd.clear();
          showUserTime();
        }
      }
    }while(!(enc.isClick()));
    select_menu++;
  }
  else if(select_menu == 4) // 
  {
    lcd.clear();
    showUserTime();
    do{
      if((millis() % 5000) < 100)mainLogic();
      lcd.setCursor(0, 0);
      lcd.print("working time 2/2");
      lcd.setCursor(0, 1);
      enc.tick();
      if(enc.isTurn())
      {
        if(enc.isRight() || enc.isRightH() || enc.isFastR()){ 
          data.maxHour++;
          if(data.maxHour > 24)data.maxHour = 0;  
          eeprom_update_block((void*)&data,0,sizeof(data));
          lcd.clear();
          showUserTime();
        }
        else if(enc.isLeft() || enc.isLeftH() || enc.isFastL() ){
          data.maxHour--;
          if(data.maxHour < 0)data.maxHour = 24;
          eeprom_update_block((void*)&data,0,sizeof(data));
          lcd.clear();
          showUserTime();
        }
      }
    }while(!(enc.isClick()));
    select_menu++;
  }
  else if(select_menu == 5) // 
  {
    if((millis() % 5000) < 100)lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("time ");
    lcd.print(clock.gettime("d-m-Y"));
    lcd.setCursor(15, 0);
    lcd.print("  ");
    lcd.setCursor(0, 1);
    lcd.print(clock.gettime("H:i:s, D"));
  }
  else{
    select_menu = 1;
  }
}

void setup()
{
  lcd.init();
  lcd.backlight();
  enc.setType(TYPE1);
  clock.begin();
  pinMode(floor_pump, OUTPUT);
  pinMode(battery_pump, OUTPUT);
  pinMode(electric_boiler, OUTPUT);
  pinMode(electric_valve, OUTPUT);
  eeprom_read_block((void*)&data,0,sizeof(data));
  mainLogic();
}

void loop()
{
  if((millis() % 5000) < 100)mainLogic();
  enc.tick();
  if(enc.isClick())select_menu++;
  drawMenu();
}
