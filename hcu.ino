/*
   Hydrocontrol
   Creation date: 01.02.2019
   Current version: v.2.1.2
   Modification date: 26.02.2019
   Autor: Wooiko
*/


#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <iarduino_RTC.h>
#include <Chrono.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // Устанавливаем дисплей
iarduino_RTC time(RTC_DS1302, 8, 10, 9); //Инициализация часов

const char hcuVersion[14] = "HCU v.2.1.6";
const int pinPump = 3; //пин насоса
const int pinOverflow = 4; //пин датчика перелива в системе
const int pinLight = 6; //пин датчика освещенности
const int pinHVRelay = 7; // пин реле высокого напряжения
const int pinJCUMoveY = 0; // пин джойстика оси Y, подключен к аналоговому входу 0
const int pinJCUMoveX = 1; //пин джойстика оси X, подключен к аналоговому входу 1
const int pinJCUMoveZ = 13; // пин джойстика кнопки, подключен к цифровому выводу 13

unsigned lghtTresholdOn = 300; //порог счетчика дребезга датчика освещенности, включение, секунд
unsigned lghtTresholdOff = 300; //порог счетчика дребезга датчика освещенности, отключение, секунд
int lghtOffMaxTime = 23; //максимальное значение времени отключения подсветки
int lghtOffMinTime = 5; //минимальное значение времени отключения подсветки
Chrono  lghtChronoOn(Chrono::SECONDS); //хронометр включения освещения
Chrono  lghtChronoOff(Chrono::SECONDS); //хронометр выключения освещения

int wpMinutePeriod = 10; //период полива, минут. определятся количеством минут в часе - от 1 до 59
//int wpSecondPeriod = 15; // время работы помпы, секунд. от 1 до 60
int wpHourPeriod = 8; //время начала полива от 0 до 23. определятся количеством часов в сутках - от 1 до 23

//float jcuStepSize = (float)180 / 1024; // вычисляем шаг градусы / на градацию; угол поворота джойстика 180 градусов, АЦП выдает значения от 0 до 1023, всего 1024 градации
int jcuState = 0; // режим работы джойстика: 0-работа, 1-программирование
Chrono jcuChronoStatePeriod(Chrono::SECONDS); //хронометр джойстика

unsigned jcuTimeToProgMode = 5; // минимальное время нажатия джойстика для перехода в режим программирования, с
unsigned jcuReturnPeriod = 5; //время возврата в исходный режим после работы джойстиком, с

int jcuXDir = 0; //направление и шаг сдвига джойстика по оси X
int jcuYDir = 0; //направление и шаг сдвига джойстика по оси Y
int jcuXMaxDir = 10; //максимальный количество шагов джойстика по оси X; определяет количество выводимых/программируемых параметров

void setup()
{
  Serial.begin(19200); // устанавливаем скорость передачи данных с модулей в 9600 бод

  lcd.init();
  lcd.backlight(); //Включаем подсветку дисплея
  //lcdPrint(hcuVersion, 0, 0);

  time.begin(); //инициализация таймера
  //time.settime(0, 42, 20, 19, 4, 19, 5); //установка времени таймера
  //time.period(1);

  pinMode(pinPump, OUTPUT); //задание режима работы пина помпы
  pinMode(pinHVRelay, OUTPUT); //задание режима работы пина реле
  pinMode(pinLight, INPUT); //задание режима работы пина освещенности
  pinMode(pinOverflow, INPUT); //задание режима работы пина перелива

  digitalWrite(pinHVRelay, HIGH); //отключить освещение
  digitalWrite(pinPump, LOW); //выключить помпу

  lghtChronoOn.stop(); //остановить таймер задержки включения освещения
  lghtChronoOff.stop(); //остановить таймер задержки отключения освещения

  //jcuChronoStatePeriod.stop(); //остановить хронометр манипулятора
}

void loop()
{
  switch (jcuState) {
    case 0:
      lcdPrint("Working", 0, 0);
      lcdPrint(time.gettime("H:i:s"), 0, 1);
      break;
    case 1:
      lcdPrint("Programming", 0, 0);
      lcdPrint(time.gettime("H:i:s"), 0, 1);
      break;
  }
  //управление помпой
  pmctrl();

  //управление подсветкой
  ltgctrl();

  //проверка нажатия джойстика
  jcuchkclk();

  //проверка движения джойстика
  jcuchkmv();
}

void pmctrl() {
  //функцяи управления помпой
  if (digitalRead(pinOverflow) == HIGH) {//если не сработал датчик перелива
    if (time.minutes % wpMinutePeriod == 0 && time.Hours < wpHourPeriod) { //если выполняется условие запуска помпы по времени
      digitalWrite(pinPump, HIGH); //включить помпу

      lcdPrint("on ", 13, 0);
    }
    else { //в противном случае отключить помпу
      digitalWrite(pinPump, LOW); //выключить помпу

      lcdPrint("off", 13, 0);
    }
  }
  else {
    { //в противном случае отключить помпу
      digitalWrite(pinPump, LOW); //выключить помпу

      lcdPrint("ovr", 13, 0);
    }
  }
}

void ltgctrl() {
  //функцяи управления подсветкой
  if (time.Hours < lghtOffMaxTime && time.Hours > lghtOffMinTime && digitalRead(pinLight) == HIGH) {
    if (!lghtChronoOn.isRunning()) {
      lghtChronoOn.restart();
      lghtChronoOff.stop();
    }
    if (lghtChronoOn.hasPassed(lghtTresholdOn)) {
      lghtChronoOn.stop();
      digitalWrite(pinHVRelay, LOW); //открыть реле для включения освещения
    }
  }
  else {
    if (!lghtChronoOff.isRunning()) {
      lghtChronoOff.restart();
      lghtChronoOn.stop();
    }
    if (lghtChronoOff.hasPassed(lghtTresholdOff)) {
      lghtChronoOff.stop();
      digitalWrite(pinHVRelay, HIGH);
    }
  }
}

void lcdPrint(String t, int x, int y) {
  lcd.setCursor(x, y);
  lcd.print(t);
}

void jcuchkclk() {
  //контроль клика джойстика

  if (digitalRead(pinJCUMoveZ) == true) { //если считывается нажатие джойстика
    if (/*!jcuChronoStatePeriod.isRunning() ||*/ (jcuChronoStatePeriod.isRunning() && jcuState == 0)) { //если счетчик режима работы не запущени или счетчик режима запущен и состояние равно "в работе (0)"
      jcuChronoStatePeriod.restart(); //рестартовать счетчик состояния
      jcuState = -1; //сразу выставить состояние "просмотр параметров (1)"
      //lcd.clear(); //
      Serial.println("Clicked");
    }
    //jcuChronoStatePeriod.start();
    if (jcuChronoStatePeriod.hasPassed(jcuTimeToProgMode)) { //по завершению цикла, если кнопка не была отпущена
      jcuState = 1; //выставить состояние "программирование (1)"
      lcd.clear(); //
      jcuChronoStatePeriod.resume(); //продолжить работу счетчика
      Serial.println("Passed 5 s");
      //jcuChronoStatePeriod.stop();
    }
  }
  else { // если нажатия джойстика не зафиксировано
    if (jcuState != 0) {
      if (jcuChronoStatePeriod.hasPassed(jcuReturnPeriod)) { //по прошествию периода
        jcuState = 0; //выставить состояние "в работе (0)"
        lcd.clear(); //
        jcuXDir = 0; //обнулить сдвиг джойстика по оси X
        jcuYDir = 0; //обнулить сдвиг джойстика по оси Y
        jcuChronoStatePeriod.resume();
        Serial.println("Clear");
      }
    }
  }
}

void jcuchkmv() {
  //контроль наклона джойстика
  int yVal = analogRead(pinJCUMoveY); //считывание аналогового значения джойстика по оси Y - 0, 512, 1023 (0, 90, 180)
  int xVal = analogRead(pinJCUMoveX); //считывание аналогового значения джойстика по оси X - 0, 512, 1023 (0, 90, 180)

  if (xVal < 341 || xVal > 682 || yVal < 341 || yVal > 682) { //если джойстик сдвинут более, чем на 30 градусов в любую сторону от 90 градусов
    jcuChronoStatePeriod.restart(); //рестартовать счетчик возврата в режим работы
    if (xVal > 682) {
      if (jcuXDir <= jcuXMaxDir) {
        xVal += 1;
      }
      else {
        jcuXDir = 0;
      }
    }
  }
}
