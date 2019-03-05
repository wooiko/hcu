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

const char hcuVersion[14] = "HCU v.2.1.5"; 
const int pinPump = 3; //пин насоса
const int pinOverflow = 4; //пин датчика перелива в системе
const int pinLight = 6; //пин датчика освещенности
const int pinHVRelay = 7; // пин реле высокого напряжения
const int pinJCUMoveY = 0; // пин джойстика оси Y, подключен к аналоговому входу 0
const int pinJCUMoveX = 1; //пин джойстика оси X, подключен к аналоговому входу 1
const int pinJCUMoveZ = 13; // пин джойстика кнопки, подключен к цифровому выводу 13

unsigned lghtTresholdOn = 300; //порог счетчика дребезга датчика освещенности, включение, секунд
unsigned lghtTresholdOff = 300; //порог счетчика дребезга датчика освещенности, отключение, секунд
Chrono  lghtChronoOn(Chrono::SECONDS); //хронометр включения освещения
Chrono  lghtChronoOff(Chrono::SECONDS); //хронометр выключения освещения

int wpMinutePeriod = 2; //период срабатывания помпы, минут. определятся кратностью минут в часе. рекомендуется: 2, 5, 10, 15, 20, 30
int wpSecondPeriod = 10; // время работы помпы, секунд. от 1 до 60

//float jcuStepSize = (float)180 / 1024; // вычисляем шаг градусы / на градацию; угол поворота джойстика 180 градусов, АЦП выдает значения от 0 до 1023, всего 1024 градации
int jcuState = 0; // режим работы джойстика: 0-отключен, 1-показ параметров, 2-программирование
Chrono jcuChronoPeriod(Chrono::MILLIS); //хронометр джойстика

int jcuParamWatchPeriod = 5; // время клика джойстика для просмотра заданных параметров, кратно 100 мс
int jcuReturnPeriod = 3; //время возврата в исходный режим после работы джойстиком, с

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
	//time.settime(0, 28, 11, 27, 2, 19, 3); //установка времени таймера
	//time.period(1);

	pinMode(pinPump, OUTPUT); //задание режима работы пина помпы
	pinMode(pinHVRelay, OUTPUT); //задание режима работы пина реле
	pinMode(pinLight, INPUT); //задание режима работы пина освещенности
	pinMode(pinOverflow, INPUT); //задание режима работы пина перелива

	digitalWrite(pinHVRelay, HIGH); //отключить освещение
	digitalWrite(pinPump, LOW); //выключить помпу

	lghtChronoOn.stop(); //остановить таймер задержки включения освещения
	lghtChronoOff.stop(); //остановить таймер задержки отключения освещения

	jcuChronoPeriod.stop(); //остановить хронометр манипулятора
}

void loop()
{
	switch (jcuState) {
	case 0:
		//lcd.clear(); //?
		lcdPrint("working", 0, 0);
		lcdPrint(time.gettime("H:i:s"), 0, 1);
		break;
	case 1:
		//lcd.clear(); //?
		lcdPrint("overview", 0, 0);
		lcdPrint(time.gettime("H:i:s"), 0, 1);
		break;
	case 2:
		//lcd.clear(); //?
		lcdPrint("programming", 0, 0);
		lcdPrint(time.gettime("H:i:s"), 0, 1);
		break;
	}
	//delay(100);
	//управление помпой
	pmctrl();

	//delay(100);

	//управление подсветкой
	ltgctrl();

	//delay(100);
	//проверка нажатия джойстика
	jcuchkclk();
	
	jcuchkmv();
}

void pmctrl() {
	//функцяи управления помпой
	if (digitalRead(pinOverflow) == HIGH) {//если не сработал датчик перелива
		if (time.minutes % wpMinutePeriod == 0 && time.seconds < wpSecondPeriod) { //если выполняется условие запуска помпы по времени
			digitalWrite(pinPump, HIGH); //включить помпу

			lcdPrint("on ", 13, 0);
			/*switch (jcuState) {
			case 0:
				lcdPrint("on", 9, 1);
				break;
			case 1:
				lcdPrint("st1", 9, 1);
				break;
			case 2:
				lcdPrint("st2", 9, 1);
				break;
			}*/
		}
		else { //в противном случае отключить помпу
			digitalWrite(pinPump, LOW); //выключить помпу

			lcdPrint("off", 13, 0);
			/*switch (jcuState) {
			case 0:
				lcdPrint("off", 9, 1);
				break;
			case 1:
				lcdPrint("st1", 9, 1);
				break;
			case 2:
				lcdPrint("st2", 9, 1);
				break;
			}*/
		}
	}
	else {
		{ //в противном случае отключить помпу
			digitalWrite(pinPump, LOW); //выключить помпу

			lcdPrint("ovr", 13, 0);
			/*switch (jcuState) {
			case 0:
				lcdPrint("ovr", 9, 1);
				break;
			case 1:

				break;
			case 2:

				break;

			}*/
		}
	}
}

void ltgctrl() {
	//функцяи управления подсветкой
	if (time.Hours > 3 && digitalRead(pinLight) == HIGH) {
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

void jcuchkclk() {
	//контроль клика джойстика

	if (digitalRead(pinJCUMoveZ) == true) { //если считывается нажатие джойстика
		if (!jcuChronoPeriod.isRunning() || (jcuChronoPeriod.isRunning() && jcuState == 0)) { //если счетчик режима работы не запущени или ичетчик режима запущен и состояние равно "в работе (0)"
			jcuChronoPeriod.restart(); //рестартовать счетчик состояния
			jcuState = 1; //сразу выставить состояние "просмотр параметров (1)"
			lcd.clear(); //
			//Serial.println("Clicked");
		}
		if (jcuChronoPeriod.hasPassed(jcuParamWatchPeriod * 100)) { //по завершению цикла, если кнопка не была отпущена
			jcuState = 2; //выставить состояние "программирование (2)"
			lcd.clear(); //
			jcuChronoPeriod.resume(); //продолжить работу счетчика
			//Serial.println("Passed 3 s");
		}
	}
	else { // если нажатия джойстика не зафиксировано
		if (jcuState != 0) {
			if (jcuChronoPeriod.hasPassed(jcuReturnPeriod * 1000)) { //по прошествию периода
				jcuState = 0; //выставить состояние "в работе (0)"
				lcd.clear(); //
				jcuXDir = 0; //обнулить сдвиг джойстика по оси X
				jcuYDir = 0; //обнулить сдвиг джойстика по оси Y
				//jcuChronoPeriod.stop();
				Serial.println("clear");
			}
		}
	}
}

void jcuchkmv() {
	//контроль наклона джойстика
	int yVal = analogRead(pinJCUMoveY); //считывание аналогового значения джойстика по оси Y - 0, 512, 1023 (0, 90, 180) 
	int xVal = analogRead(pinJCUMoveX); //считывание аналогового значения джойстика по оси X - 0, 512, 1023 (0, 90, 180) 

	if (xVal < 341 || xVal>682 || yVal < 341 || yVal>682) {//если джойстик сдвинут более, чем на 30 градусов в любую сторону от 90 градусов
		jcuChronoPeriod.restart(); //рестартовать счетчик возврата в режим работы
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

void lcdPrint(String t, int x, int y) {
	lcd.setCursor(x, y);
	lcd.print(t);
}
