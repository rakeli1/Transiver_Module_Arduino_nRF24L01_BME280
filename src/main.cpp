#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <GyverBME280.h>
#include <nRF24L01.h>
#include <RF24.h>
#define CE_PIN 9
#define CSN_PIN 10

// put function declarations here:
RF24 radio(CE_PIN, CSN_PIN); // "создать" модуль на пинах 9 и 10 Для Уно
//RF24 radio(9,53); // для Меги
GyverBME280 bme;

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"}; //возможные номера труб

byte counter;

float dataSensor[3];

enum Type
{
   HOME = 1, // Идентификатор датчика находящегося в доме,раскоментировать в зависимости от назначения модуля
   OUTDOR = 2, // Идентификатор датчика находящегося на улице,раскоментировать в зависимости от назначения модуля
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

  radio.powerUp();        // начать работу
  radio.stopListening();  // не слушаем радиоэфир, мы передатчик 

}

void loop() 
{
  while(paket.temperature == 0)        // Этот цикл решает проблему отсылки первого  пустого пакета.Пока в структуру не будут записаны данные
  {                                                                                    // передатчик не будет отсылать пакет
  paket.temperature = bme.readTemperature();                                 
  paket.hymidity    = bme.readHumidity();
  paket.pressure    = bme.readPressure();
  }

  bool ok = radio.write(&paket, sizeof(paket));

  Serial.println("I SENT: ");

  Serial.print("Sent: ");
  Serial.println(counter);
  radio.write(&counter, sizeof(counter));
  counter++;

   // температура
  Serial.print("Temperature: ");
  Serial.println(paket.temperature);
  //Serial.println(bme.readTemperature());

  // влажность
  Serial.print("Humidity:    ");
  Serial.println(paket.hymidity);
  //Serial.println(bme.readHumidity());

  // давление
  Serial.print("Pressure:    ");
  Serial.println(pressureToMmHg(paket.pressure));
  //Serial.println(pressureToMmHg(bme.readPressure()));
  

  if (ok)
  {
    Serial.println("Sent sucsses!");
  }
   else
  {
    Serial.println("Error of sending");// проверить метод setAutuAsk(1),тест без приемника-0, с приемником-1;
                                      //модуль ждет подтверждения приема при 1, при 0 - не ждет подтверждения
  }

  Serial.println();
  delay(5000);
}

