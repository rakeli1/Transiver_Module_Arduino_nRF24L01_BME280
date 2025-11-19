#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <GyverBME280.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "LowPower.h"
#define CE_PIN 9
#define CSN_PIN 10

// put function declarations here:
RF24 radio(CE_PIN, CSN_PIN); // "создать" модуль на пинах 9 и 10 Для Уно
//RF24 radio(9,53); // для Меги
GyverBME280 bme;

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"}; //возможные номера труб

byte counter;     // ОТЛАДКА

enum Type
{
   HOME = 1, // Идентификатор радиопередатчика находящегося в домет 
   OUTDOR = 2, // Идентификатор радиопередатчика находящегося на улице
};

struct TX_Data
{
   Type type;
  float temperature;
  float hymidity;
  float pressure;
  float batttery;
};


TX_Data paket;

void arduinoSleep30min()      // функция усыпляющая ардуинку в данном случае сон на 64 сек. в рабочей версии будет 30 мин.
{
  for(int a = 0; a < 8; a++)
  {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }
}

bool firstStart = true;       // переменная которая в операторе "if(firstStart)" выполняет условие первого запуска после подачи питания без засыпания
                                                                                // Ардуинки
void setup() 
{
 Serial.begin(9600);          // открываем порт для связи с ПК
  
  bme.begin(0x76);            // активируем датчик BME280 
  radio.begin();              // активировать модуль
  radio.setAutoAck(0);        // режим подтверждения приёма, 1 вкл 0 выкл,тест модуля без приемника - 0
  radio.setRetries(0, 15);    // (время между попыткой достучаться, число попыток)
  radio.enableAckPayload();   // разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);   // размер пакета, в байтах

  radio.openWritingPipe(address[0]);  // мы - труба 0, открываем канал для передачи данных
  radio.setChannel(0x60);             // выбираем канал (в котором нет шумов!)

  radio.setPALevel (RF24_PA_MAX);   // уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
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

  bme.setMode(FORCED_MODE);                              // после измерения датчик сам уходит в сон это режим поведения на уровне датчика
  delay(10);
   
    paket.temperature = bme.readTemperature();           //чтение температуры и запись в структуру                               
    paket.hymidity    = bme.readHumidity();              //чтение влажности и запись в структуру
    paket.pressure    = bme.readPressure();              //чтение давления и запись в структуру
   
  
  radio.powerUp();                                       // выводим радиомодуль из спящего режима
  delay(5);
  bool ok = radio.write(&paket, sizeof(paket));          // отсылка данных в эфир, Переменная ок - отладочная(узнаем успешность отсылки данных)
  delay(5);
  radio.powerDown();                                     //переводим радиомодуль в спящий режим

  Serial.println("I SENT: ");                            // ОТЛАДКА

  Serial.print("Sent: ");                                // ОТЛАДКА
  Serial.println(counter);
  //radio.write(&counter, sizeof(counter));              //ОБРАТИТЬ ПРИСТАЛЬНОЕ ВНИМАНИЕ ПРИ РАЗРАБОТКЕ ПРИЕМНИКА  // ОТЛАДКА
  counter++;                                             // ОТЛАДКА

   // температура
  Serial.print("Temperature: ");                         // ОТЛАДКА
  Serial.println(paket.temperature);                     // ОТЛАДКА
  //Serial.println(bme.readTemperature());               // ОТЛАДКА

  // влажность
  Serial.print("Humidity:    ");                         // ОТЛАДКА
  Serial.println(paket.hymidity);                        // ОТЛАДКА 
  //Serial.println(bme.readHumidity());                  // ОТЛАДКА

  // давление
  Serial.print("Pressure:    ");        // ОТЛАДКА 
  Serial.println(pressureToMmHg(paket.pressure));        // ОТЛАДКА
  //Serial.println(pressureToMmHg(bme.readPressure()));  // ОТЛАДКА
  

  if (ok)                                  // ОТЛАДКА
  {                                        // ОТЛАДКА
    Serial.println("Sent sucsses!");       // ОТЛАДКА
  }                                        // ОТЛАДКА
   else                                    // ОТЛАДКА
  {                                        // ОТЛАДКА
    Serial.println("Error of sending");// проверить метод setAutuAsk(1),тест без приемника-0, с приемником-1; // ОТЛАДКА
                                      //модуль ждет подтверждения приема при 1, при 0 - не ждет подтверждения // ОТЛАДКА
  }                                                                                                           // ОТЛАДКА

  Serial.println();                                                                                           // ОТЛАДКА
  delay(1000);

  firstStart = false;     // эта переменная обеспечивает запуск спящего режима при следующем цикле "loop"
  
}

