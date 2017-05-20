//https://sites.google.com/site/marthalprojects/home/arduino/i2c-via-digital-pins
//http://radiolaba.ru/microcotrollers/i2c-interfeys.html
/*
  это пример работы с i2c шиной. для простоты взята i2c eeprom микросхема с адресом 0x50
  первый способ читает с нее данные через аппаратную i2c шину ардуины с помощью Wire.h
  второй способ тоже использует аппаратную шину i2c но читает уже на более ниском уровне через twi.h
  третий способ не использует пины i2c шины а использует пины A0, A1 и "ногодрыг"!
*/

/* extern "C" { 
#include "utility/twi.h"  // from Wire library, so we can do bus scanning
}
#include <Wire.h>
*/
#define DEV_ADDR 0x50

/*
//обычное чтение из eeprom-а с помощью Wire.h
void read_eeporom_v1(void){
  char ddd[20];
  char buf[2];
  Wire.beginTransmission(DEV_ADDR);
  Wire.write(0x0); //адрес для чтения из eeprom
  Wire.endTransmission();
  Wire.requestFrom(DEV_ADDR, 2);
  if(Wire.available()){
    buf[0] = Wire.read() & 0xFF;
    buf[1] = Wire.read() & 0xFF;
    snprintf(ddd, sizeof(ddd), "%02X %02X", buf[0] & 0xFF, buf[1] & 0xFF);
    Serial.println(ddd);
  }else{
     Serial.println("No data from I2C");
  }
}

//чтение из eeprom-а с помощью twi.h
void read_eeporom_v2(void){
  char ddd[20];
  uint8_t buf[10];
  byte rc;
  buf[0] = 0;
  rc = twi_writeTo(DEV_ADDR, buf, 1, 1, 1);
  Serial.print("twi_writeTo return := ");
  Serial.println(rc);
  rc = twi_readFrom(DEV_ADDR, buf, 2, 1);
  Serial.print("twi_readFrom return := ");
  Serial.println(rc);
  if(rc > 0){
    snprintf(ddd, sizeof(ddd), "%02X %02X", buf[0] & 0xFF, buf[1] & 0xFF);
  }else
    snprintf(ddd, sizeof(ddd), "No DaTa!");
  Serial.println();
  if(rc > 0)
    Serial.print("Data: ");  
  Serial.println(ddd);
}
*/


/* чтение из eeprom-а с помощью ногодрыга */
//вспомни bus-pirat-а(https://bitbucket.org/jiajun/rtl8xxx-switch)! он что то подобное и делал своим конфигом!
//это очень примитивная реализация. не для продакшена! тут нет зажержек и clock сигнал соблюдается весьма условно!
//отсутствие задержек(delay) наверное компенсирует тормознутость процессора ардуины
//и к тому же оно чоень медленное.
#define MY_SDA A0
#define MY_SCL A1
int i2c_out;
void putbyte(){
  int i;
  for(i = 7; i >= 0; i--){ //побитовая передача
    if((i2c_out & (1 << i)) == 0)
      digitalWrite(MY_SDA, LOW);
    else
      digitalWrite(MY_SDA, HIGH);
    digitalWrite(MY_SCL, HIGH); //pulse clock
    digitalWrite(MY_SCL, LOW);
  }
}
void getbyte(){
  int i;
  pinMode(MY_SDA, INPUT);
  i2c_out = 0;
  for(i = 7; i >= 0; i--){
    if(digitalRead(MY_SDA)  ==  HIGH){   
      i2c_out = (i2c_out + (1 <<i ));
      delay(10);
    }
    digitalWrite(MY_SCL, HIGH); //pulse clock
    digitalWrite(MY_SCL, LOW);
  }     
  pinMode(MY_SDA, OUTPUT);   
}
byte getack(){
  byte ret;
  digitalWrite(MY_SDA, HIGH);
  pinMode(MY_SDA, INPUT);
  digitalWrite(MY_SCL, HIGH);
  ret =   digitalRead(MY_SDA);
  digitalWrite(MY_SCL, LOW);
  pinMode(MY_SDA, OUTPUT);
  return ret;
}
//тут все как на картиках работы i2c шины.
//например как тут: http://radiolaba.ru/microcotrollers/i2c-interfeys.html
void start(){
  digitalWrite(MY_SCL, HIGH);
  digitalWrite(MY_SDA, HIGH);
  digitalWrite(MY_SDA, LOW);
  digitalWrite(MY_SCL, LOW);
}
void stop(){
  digitalWrite(MY_SDA, LOW);
  digitalWrite(MY_SCL, HIGH); 
  digitalWrite(MY_SDA, HIGH);
}
void giveack() {
  digitalWrite(MY_SDA, LOW);
  digitalWrite(MY_SCL, HIGH);    //pulse clock
  digitalWrite(MY_SCL, LOW);
//  digitalWrite(MY_SDA, LOW);
}   
void givenoack(){
  digitalWrite(MY_SDA, HIGH);
  digitalWrite(MY_SCL, HIGH);    //pulse clock
  digitalWrite(MY_SCL, LOW);
  digitalWrite(MY_SDA, LOW);
}
void read_eeporom_v3(void){  
  char buf[20];
  byte rc;
  int a;
  int address = 0; //все 3 ноги микры eeprom ноги на земле. это наш 0x50 !
  address |= DEV_ADDR << 1; //со здвигом получается 0xA0 !!! привет bus-pirat-у!
  Serial.print("eeprom i2c slave address := ");
  snprintf(buf, sizeof(buf), "0x%X", address);
  Serial.println(buf);
  //common routines ! пошлем запрос на чтение адреса 0x0 из eeprom-а
  start();
  //отправляем i2c адрес нашей slave 24lcXXX eeprom-ки. это 7 байт. 0-й байт == 0 для write!
  i2c_out = address; 
  putbyte(); 
  rc = getack();
  if(rc) //если ack не был получен!
    goto end;
  //задаем адрес для чтения из eeprom
  i2c_out = 0;
  putbyte(); 
  if((rc = getack()))
    goto end;
  //теперь читаем ответ
  start();
  i2c_out = address + 1; //address slave 24lcXXX eeprom-ки. 0-й байт == 1 для read!
  putbyte(); //просим eeprom прислать нам ответ
  if((rc = getack())) //убеждаемся что она получила запрос
    goto end;
  //начинаем прием данных
  Serial.print("Data: ");
  for(a = 0; a < 20; a++){
    if(a)
      giveack();   //еще хотим данных
    getbyte(); //retrieve content of the memory cell  
    snprintf(buf, sizeof(buf), "%02X ", i2c_out & 0xFF);
    Serial.print(buf);
  }
  givenoack(); //все наелись данными. конец приема.
end:
  stop();
  if(rc){
    Serial.print("END rc = ");
    Serial.println(rc);
  }
  delay(10);
}

void setup(){       
  //Wire.begin(); //Start I2C connections
  pinMode(MY_SCL, OUTPUT);
  pinMode(MY_SDA, OUTPUT);
  Serial.begin(115200);  
  read_eeporom_v3();
}

void loop(){
}

