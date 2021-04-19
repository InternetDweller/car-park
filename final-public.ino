#include <Servo.h>
#include <SoftwareSerial.h>

SoftwareSerial WIFI(4, 5);
Servo servo1, servo2;

char ssid[] = "(hidden)"; //имя Wi-Fi сети
char pass[] = "(hidden)"; //пароль
int Max = 8; //всего мест на парковке
int maxDist = 4; //максимальное расстояние, на котором шлагбаумы открываются

char server[] = "(hidden)"; //адрес сайта

int avail = Max; //кол-во свободных мест
int unavail = 0; //кол-во занятых мест
int sens1t = 6; //для выезда (триг)
int sens1e = 7; //для выезда (эхо)
int sens2t = 9; //для въезда (триг)
int sens2e = 10; //для въезда (эхо)

void setup() {
  Serial.begin(9600); //открытие последовательного порта на скорости 9600 бод
  while (!Serial) { } //ожидание открытия порта
  WIFI.begin(9600); //открытие Wi-Fi порта на скорости 9600 бод
  while (!WIFI) { } //ожидание открытия порта
  WIFI.println("AT+RST"); //перезапуск Wi-Fi модуля
  WIFI.println("AT+CWMODE_DEF=3"); //установка режима клиентский + точка доступа
  WIFI.println("AT+CIPMUX=0"); //установка режима одинарного соединения

  WIFI.println("AT+CWJAP=?"); //проверяем, есть ли соединение с Wi-Fi
  if (WIFI.find("ERROR")) //если соединения нет...
  {
    while(!WIFI.find("OK")) //пытаемся подключиться к сети, пока не получится
    {
      WIFI.println("AT+CWJAP=\"ssid\",\"pass\""); //команда для подключения к Wi-Fi по имени сети и паролю
      delay(3000);
    }
  }
  Serial.println("OK, Connected to Wi-Fi.");
  delay(3000);

  SEND(Max); //установка на сайте максимального кол-ва свободных мест
  delay(3000);

  pinMode(sens1t, OUTPUT); //триг 1 - отправляет сигнал
  pinMode(sens1e, INPUT); //эхо 1 - получает сигнал
  pinMode(sens2t, OUTPUT); //триг 2 - отправляет сигнал
  pinMode(sens2e, INPUT); //эхо 2 - получает сигнал

  digitalWrite(sens1t, LOW); //установка низкого сигнала у трига 1
  digitalWrite(sens2t, LOW); //установка низкого сигнала у трига 2
}

void loop() {  
  digitalWrite(sens1t, HIGH); //подача сигнала на триг для выезда
  delayMicroseconds(10);
  digitalWrite(sens1t, LOW); //прекращение подачи сигнала на триг для выезда
  int dist1 = pulseIn(sens1e, HIGH) / 58; //рассчёт расстояния до объекта
  if (dist1 <= maxDist && dist1 != 0) //цикл для выезда
  {
    dist1 = maxDist + 1; //сброс значения расстояния на выезде
    servo1.attach(8); //активация шлагбаума для выезда
    for (int i = 0; i <= 90; i++) //от 0 до 90 "градусов"
    {
      servo1.write(i); //поворот привода
      delay(10);
    }
    delay(3000);
    for (int i = 89; i >= 0; i--) //от 89 до 0 "градусов"
    {
      servo1.write(i); //поворот привода
      delay(10);
    }
    servo1.detach(); //деактивация шлагбаума для выезда
    if (unavail > 0) unavail--; //уменьшение числа занятых мест, если оно не уходит в минус
    if (avail < Max) avail++; //увеличение числа свободных мест, если оно не будет превышать максимальное
    SEND(avail); //отправка кол-ва свободных мест на сайт
    SEND(avail);
    Serial.print("Всего: ");
    Serial.print(Max);
    Serial.print(" / Занято: ");
    Serial.print(unavail);
    Serial.print(" / Свободно: ");
    Serial.println(Max - unavail);
  }

  digitalWrite(sens2t, HIGH); //подача сигнала на триг для въезда
  delayMicroseconds(10);
  digitalWrite(sens2t, LOW); //прекращение подачи сигнала на триг для въезда
  int dist2 = pulseIn(sens2e, HIGH) / 58; //рассчёт расстояния до объекта
  if (dist2 <= maxDist && unavail != Max && dist2 != 0) //цикл для въезда (срабатывает только если есть свободные места)
  {
    dist2 = maxDist + 1; //сброс значения расстояния на въезде
    servo2.attach(11); //активация шлагбаума для въезда
    for (int i = 0; i <= 90; i++) //от 0 до 90 "градусов"
    {
      servo2.write(i); //поворот привода
      delay(10);
    }
    delay(3000);
    for (int i = 89; i >= 0; i--) //от 89 до 0 "градусов"
    {
      servo2.write(i); //поворот привода
      delay(10);
    }
    servo2.detach(); //деактивация шлагбаума для въезда
    unavail++; //увеличение числа занятых мест
    if (avail > 0) avail--; //уменьшение числа свободных мест, если оно не уходит в минус
    SEND(avail); //отправка кол-ва свободных мест на сайт
    SEND(avail);
    Serial.print("Всего: ");
    Serial.print(Max);
    Serial.print(" / Занято: ");
    Serial.print(unavail);
    Serial.print(" / Свободно: ");
    Serial.println(Max - unavail);
  }
}

void SEND(int avail)
{
  int cipCount = 0;
  while (!WIFI.find("OK")) //пытаемся открыть TCP соединение с сайтом на порте 80
  {
    WIFI.println("AT+CIPSTART=\"TCP\",\"(hidden)\",80");
    Serial.print(".");
    if (cipCount == 10) //если соединение не удалось открыть за 10 попыток...
    {
      Serial.println("Error! Request has not been sent."); //пишем сообщение об ошибке отправки
      return; //выходим из функции отправки данных
    }
    cipCount++; //подсчёт кол-ва попыток открытия соединения
  }
  Serial.println("TCP opened");
  String cmd = "GET /functions/start.php?CARS=" + String(avail) + " HTTP/1.0\r\nHost: (hidden)\r\n\r\n"; //формирование строки для вычисления её размера
  WIFI.println("AT+CIPSEND=" + String(cmd.length())); //сообщаем, сколько байт мы отправляем
  if (WIFI.find(">")) //если Wi-Fi модуль готов принимать данные...
  {
    WIFI.println("GET /functions/start.php?CARS=" + String(avail) + " HTTP/1.0"); //формируем GET запрос с передачей кол-ва мест и версии протокола HTTP
    WIFI.println("Host: (hidden)"); //передаём адрес сайта
    WIFI.println(); //обязательная пустая строка
  }
  return;
}
