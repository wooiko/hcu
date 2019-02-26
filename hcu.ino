/*
   Hydrocontrol
   Creation date: 01.02.2019
   Current version: v.2.1.1
   Modification date: 25.02.2019
   Autor: Wooiko
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <iarduino_RTC.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // Устанавливаем дисплей
iarduino_RTC time(RTC_DS1302, 8, 10, 9); //Инициализация часов

char hcuVersion[14] = "HCU v.2.1.1"; //int pinPump = 2;//насос - поврежден транзистор?
int pinPump = 3; //насос
int pinOverflow = 4; //датчик перелива в системе
int pinLight = 6; //датчик освещенности
int pinRelay = 7; // реле освещенности

int minutePeriod = 2; //период срабатывания помпы, минут. определятся кратностью минут в часе. рекомендуется: 5, 10, 15, 20, 30
int secondPeriod = 10; // время работы помпы, секунд. от 1 до 60
int lightChatter = 0; //счетчик дребезга датчика освещенности
int lightCTreshold = 10; //порог счетчика дребезга датчика освещенности
bool runPump = false;//признак запуска насоса


const int pinY = 0; // Потенциометр оси Y подключен к аналоговому входу 0
const int pinX = 1; //Потенциометр оси X подключен к аналоговому входу 1
const int pinZ = 13; // Кнопка подключена к цифровому выводу 13
float hmstepSize = (float)180 / 1024; // Вычисляем шаг. градусы / на градацию; угол поворота джойстика 180 градусов, АЦП выдает значения от 0 до 1023, всего 1024 градации
int hmState = 0; // режим работы манипулятора: 0-отключен, 1-показ параметров, 2-программирование

void setup()
{
	//Serial.begin(9600); // устанавливаем скорость передачи данных с модулей в 9600 бод

	lcd.init();
	lcd.backlight(); //Включаем подсветку дисплея
	lcd.print(hcuVersion);

	time.begin(); //инициализация таймера
	//time.settime(0, 38, 21, 11, 2, 19, 1); //установка времени таймера
	//time.period(1);

	pinMode(pinPump, OUTPUT); //задание режима работы пина помпы
	pinMode(pinRelay, OUTPUT); //задание режима работы пина реле
	pinMode(pinLight, INPUT); //задание режима работы пина освещенности
	pinMode(pinOverflow, INPUT); //задание режима работы пина перелива
}

void loop()
{
	if (hmState == 0) {
		lcd.setCursor(0, 1); //Устанавливаем курсор на вторую строку и нулевой символ.
		lcd.print(time.gettime("H:i:s"));
	}


	delay(100);

	//управление помпой
	pmctrl();

	delay(100);

	//управление подсветкой
	ltgctrl();

	delay(100);
}

void pmctrl() {
	//функцяи управления помпой
	if (digitalRead(pinOverflow) == HIGH) {//если не сработал датчик перелива
		if (time.minutes % minutePeriod == 0 && time.seconds < secondPeriod) { //если выполняется условие запуска помпы по времени
			digitalWrite(pinPump, HIGH); //подать высокий уровень на пин помпы для запуска

			if (hmState == 0) {
				lcd.setCursor(9, 1);
				lcd.print("on ");
			}

		}
		else { //в противном случае отключить помпу
			digitalWrite(pinPump, LOW);//подать низкий уровень на пин помпы для останова

			if (hmState == 0) {
				lcd.setCursor(9, 1);
				lcd.print("off");
			}
		}
	}
	else {
		{ //в противном случае отключить помпу
			digitalWrite(pinPump, LOW);//подать низкий уровень на пин помпы для останова

			if (hmState == 0) {
				lcd.setCursor(9, 1);
				lcd.print("ovf");
			}
		}
	}
}

void ltgctrl() {
	//функцяи управления подсветкой
	if (time.Hours > 3 && digitalRead(pinLight) == HIGH) {
		if (lightChatter >= lightCTreshold) {
			lightChatter = 0; //обнулить счетчик дребезга датчика освещения
			digitalWrite(pinRelay, LOW) //открыть реле для включения освещения
		}
		else {
			lightChatter += 1; //увеличить счетчик дребезга датчика освещения - максимум 60 секунд
		}
	}
	else {
		// lightChatter = 0; //сбросить счетчик дребезга датчика освещения
		digitalWrite(pinRelay, HIGH); //закрыть реле, отключить освещение
	}
}

void hmctrl() {
	//функция управления манипулятором



	int yVal = analogRead(pinY); // Задаем переменную yVal для считывания показаний аналогового значения
	int xVal = analogRead(pinX); //Аналогично xVal

	float yAngle = yVal * hmstepSize; // Переводим выходные данные yVal в угол наклона джойстика (от 0 до 180)
	float xAngle = xVal * hmstepSize; // Аналогично xVal

	boolean isClicked = digitalRead(pinZ); // Считываем не было ли нажатия на джойстик

	Serial.print("Horisontal angle = "); // Выводим текст 
	Serial.println(xAngle);              // Выводим значение угла 
	Serial.print("Vertical angle = ");
	Serial.println(yAngle);
	if (isClicked)
	{
		Serial.println("Clicked");
	}
	delay(1000);
}

void lcdPrint(char t, int x, int y) {
	lcd.setCursor(x, y);
	lcd.print(t);
}