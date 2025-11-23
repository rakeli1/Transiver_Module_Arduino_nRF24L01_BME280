#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <GyverBME280.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "LowPower.h"
#define CE_PIN 9
#define CSN_PIN 10

 const float BATPIN = A0;              // Определение пина А0 для входа от делителя батарейного напряжения 
 const float INREF = 1.1f;           // внутреннее опорное напряжение подключается функцией analogReference(INTERNAL);
 const float R1 = 3;                 // R1 - подключен м-у Vbat и пином A0 
 const float R2 = 1;                 // R2 - покдключен м-у GND
 const float RATIO = (R1 + R2) / R2; // коефицент увеличения напряжения выхода
 const float VMIN = 3.3f;            // значение напряжения батареи при котором считать уровень зарядки "0"
 const float VMAX = 4.2f;            // значение напряжения батареи при котором считать уровень заряда 100%
 const float K = R2 / (R1 + R2);     // коефицент затухания делителя
       float Va0 = 0;                // значение напряжения на пине А0; расчетное по значению от analogread(BATPIN)
       float Vbat = 0;               //  расчетное значение напряжения на батарее (подлежит калибровке)
          

// put function declarations here:
RF24 radio(CE_PIN, CSN_PIN); // "создать" модуль на пинах 9 и 10 Для Уно
//RF24 radio(9,53); // для Меги
GyverBME280 bme;

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"}; //возможные номера труб
    

enum Type
{
  ID,
  TEMPERATURE,
  HUMIDITY,
  PRESSURE,
  PERCENT,
  ADCFL,
  VA0,
  VBAT,
  COUNER,
   
};

float tx_data[8];


void arduinoSleep30min()      // функция усыпляющая ардуинку в данном случае сон на 64 сек. в рабочей версии будет 30 мин.
{
  for(int a = 0; a < 1; a++)
  {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }
}

bool firstStart = true;       // переменная которая в операторе "if(firstStart)" выполняет условие первого запуска после подачи питания без засыпания
                                                                                // Ардуинки
void setup() 
{
  Serial.begin(9600);         // открываем порт для связи с ПК/

  analogReference(INTERNAL); // подключение к внутреннему опорному напряжению 1,1В, на пин А0
                            // подключать напряжение не более 1,1В ВАЖНО!!!!!
  
  bme.begin(0x76);            // активируем датчик BME280 
  radio.begin();              // активировать модуль
  radio.setAutoAck(1);        // режим подтверждения приёма, 1 вкл 0 выкл,тест модуля без приемника - 0
  radio.setRetries(0, 15);    // (время между попыткой достучаться, число попыток)
  radio.enableAckPayload();   // разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);   // размер пакета, в байтах

  radio.openWritingPipe(address[0]);  // мы - труба 0, открываем канал для передачи данных
  radio.setChannel(0x60);             // выбираем канал (в котором нет шумов!)

  radio.setPALevel (RF24_PA_LOW);   // уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_250KBPS); // скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  //должна быть одинакова на приёмнике и передатчике!
  //при самой низкой скорости имеем самую высокую чувствительность и дальность!!

  radio.powerUp();                                       // начать работу радиопередатчика
  radio.stopListening();                                 // не слушаем радиоэфир, мы передатчик 

}

void loop() 
{
  if(!firstStart)                                        // этим кодом мы при первом запуске пропускаем сон чтобы показания датчика на приемнике выводились сразу без ожидания окончания периода сна
  {
    arduinoSleep30min();
  }

  int adc = 0;                           //число формируемое АЦП 
  
  for(int sempl = 0; sempl < 20; ++sempl) // цикл для  усреднения значения АЦП
  {
     adc += analogRead(BATPIN);
      delayMicroseconds(50);
  }
  adc /= 20;
  float adcfl = (float)adc;                                       // уреднение значения АЦП
  Va0 = (adc * (INREF / 1024)) / 1.022;  // 1.022 это калибровочный коефицент разница между реальным и расчетным напряжением                   // вычисление напряжения на пине А0 
  Vbat = (adc * (INREF / 1024)) * RATIO;           // вычисление напряжения на батарее
  float percent = ((Vbat - VMIN) / (VMAX - VMIN)) * 100; // вычисление процента зарядки батареи
  if (percent > 100) percent = 100;
  if (percent < 0) percent = 0;

  
  

  bme.setMode(FORCED_MODE);                              // после измерения датчик сам уходит в сон это режим поведения на уровне датчика
  delay(10);
   
    
    tx_data[ID] = 1.0;                         // идентификатор местоположения радиомодуля
    tx_data[TEMPERATURE] = bme.readTemperature();           //чтение температуры и запись в структуру                               
    tx_data[HUMIDITY] = bme.readHumidity();              //чтение влажности и запись в структуру
    tx_data[PRESSURE] = pressureToMmHg((bme.readPressure())); //чтение давления и запись в структуру
    tx_data[PERCENT] = percent;                         // процент заряда батареи
    tx_data[ADCFL] = adcfl;                  // ОТЛАДКА значение АЦП
    tx_data[VA0] = Va0;                  // ОТЛАДКА напяжение на пине А0
    tx_data[VBAT] = Vbat;                 // ОТЛАДКА напряжение на батарее(расчетное)
  
    
   
    
    radio.powerUp();                                       // выводим радиомодуль из спящего режима
  delay(5);
  bool ok = radio.write(&tx_data, sizeof(tx_data));          // отсылка данных в эфир, Переменная ок - отладочная(узнаем успешность отсылки данных)
  delay(5);
  radio.powerDown();                                     //переводим радиомодуль в спящий режим
  Serial.println("I SENT: ");                            // ОТЛАДКА
  
  Serial.print("Sent: ");                                // ОТЛАДКА                               // ОТЛАДКА
  

   // температура
  Serial.print("Temperature: ");                         // ОТЛАДКА
  Serial.println(tx_data[TEMPERATURE]);                     // ОТЛАДКА
            

  // влажность
  Serial.print("Humidity:    ");                         // ОТЛАДКА
  Serial.println(tx_data[HUMIDITY]);                        // ОТЛАДКА 
  

  // давление
  Serial.print("Pressure:    ");        // ОТЛАДКА 
  Serial.println(tx_data[PRESSURE]);        // ОТЛАДКА

  Serial.println();

  Serial.print("Значение АЦП           ");
  Serial.println(tx_data[ADCFL]);   //подключить конденсатор паралельно нижнему резистору 0.01-0.1мкФ

  // Напряжение на батарее
  Serial.print("Напряжение на батарее  ");
  Serial.println(tx_data[VBAT]);

  // Напряжение на пине А0
  Serial.print("Напряжение на пине A0  ");
  Serial.println(tx_data[VA0]);

  // Процент заряда батареи
  Serial.print("Процент заряда батареи ");
  Serial.println(tx_data[PERCENT]);

  Serial.println();
  

  if (ok)                                  // ОТЛАДКА. 
  {                                        // ОТЛАДКА
    Serial.println("Sent sucsses!");       // ОТЛАДКА
  }                                        // ОТЛАДКА
   else                                    // ОТЛАДКА
  {                                        // ОТЛАДКА
    Serial.println("Error of sending");// проверить метод setAutuAsk(1),тест без приемника-0, с приемником-1; // ОТЛАДКА
                                      //модуль ждет подтверждения приема при 1, при 0 - не ждет подтверждения // ОТЛАДКА
  }                                                                                                           // ОТЛАДКА

  Serial.println();                                                                                           // ОТЛАДКА
  delay(3000);

  firstStart = false;     // эта переменная обеспечивает запуск спящего режима при следующем цикле "loop"
  
}

