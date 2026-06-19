/*
 * Код для двух датчиков SHT75 с выводом на LCD Keypad Shield (16x2)
 * 
 * Распиновка LCD Keypad Shield (D1 Robot):
 * - LCD RS    -> pin 8
 * - LCD EN    -> pin 9
 * - LCD D4    -> pin 4
 * - LCD D5    -> pin 5
 * - LCD D6    -> pin 6
 * - LCD D7    -> pin 7
 * 
 * Кнопки (используют аналоговый вход A0):
 * - RIGHT -> 0-100
 * - UP    -> 100-200
 * - DOWN  -> 200-400
 * - LEFT  -> 400-600
 * - SELECT -> 600-800
 * 
 * Свободные пины для датчиков (не пересекаются с LCD и кнопками):
 * - Пин 2, 3, 10, 11, 12, 13, A1-A5
 */

#include <LiquidCrystal.h>
#include <Sensirion.h>

// ===== НАСТРОЙКА LCD ДИСПЛЕЯ =====
const int rs = 8, en = 9, d4 = 4, d5 = 5, d6 = 6, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// ===== НАЗНАЧЕНИЕ ПИНОВ ДЛЯ ДАТЧИКОВ =====
// Используем пины, которые не заняты LCD (2,3,10,11,12,13)
// и аналоговые пины A1-A5 (можно использовать как цифровые)
const uint8_t SHT75_DATA_PIN_1 = 2;    // Первый датчик - линия данных
const uint8_t SHT75_SCLK_PIN_1 = 3;    // Первый датчик - линия тактов

const uint8_t SHT75_DATA_PIN_2 = A2;   // Второй датчик - линия данных
const uint8_t SHT75_SCLK_PIN_2 = A3;   // Второй датчик - линия тактов

// ===== ПИНЫ КНОПОК =====
const int buttonPin = A0;  // Аналоговый вход для кнопок

// ===== СОЗДАНИЕ ОБЪЕКТОВ ДЛЯ ДАТЧИКОВ =====
Sensirion sensor1(SHT75_DATA_PIN_1, SHT75_SCLK_PIN_1);
Sensirion sensor2(SHT75_DATA_PIN_2, SHT75_SCLK_PIN_2);

// ===== ПЕРЕМЕННЫЕ ДЛЯ ХРАНЕНИЯ ДАННЫХ =====
float temperature1, humidity1, dewpoint1;
float temperature2, humidity2, dewpoint2;

// ===== ПЕРЕМЕННЫЕ ДЛЯ УПРАВЛЕНИЯ РЕЖИМАМИ =====
int displayMode = 0;  // 0 - оба датчика, 1 - датчик 1, 2 - датчик 2
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 300;  // Защита от дребезга кнопок

void setup() {
  // Инициализация последовательного порта для отладки
  Serial.begin(9600);
  
  // Инициализация LCD дисплея (16 символов x 2 строки)
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("SHT75 Sensors");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  
  // Ожидание готовности датчиков
  delay(15);
  
  // Проверка подключения датчиков
  checkSensors();
  
  // Очистка дисплея
  lcd.clear();
  
  Serial.println("--- SHT75 Dual Sensor System Started ---");
  Serial.println("Use buttons to change display mode:");
  Serial.println("RIGHT - Show both sensors");
  Serial.println("LEFT  - Show Sensor 1 only");
  Serial.println("SELECT- Show Sensor 2 only");
}

void loop() {
  // Обработка нажатий кнопок
  handleButtons();
  
  // Измерение данных с датчиков
  measureSensors();
  
  // Обновление дисплея
  updateDisplay();
  
  // Вывод в Serial для отладки
  printToSerial();
  
  // Задержка между измерениями
  delay(2000);
}

/**
 * Функция проверки подключения датчиков
 */
void checkSensors() {
  bool sensor1Ok = false;
  bool sensor2Ok = false;
  
  // Пытаемся прочитать данные с первого датчика
  if (sensor1.measure(&temperature1, &humidity1, &dewpoint1) == 0) {
    sensor1Ok = true;
    Serial.println("Sensor 1: OK");
  } else {
    Serial.println("Sensor 1: ERROR - Check connection!");
  }
  
  // Пытаемся прочитать данные со второго датчика
  if (sensor2.measure(&temperature2, &humidity2, &dewpoint2) == 0) {
    sensor2Ok = true;
    Serial.println("Sensor 2: OK");
  } else {
    Serial.println("Sensor 2: ERROR - Check connection!");
  }
  
  // Вывод статуса на дисплей
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("S1:");
  lcd.print(sensor1Ok ? "OK " : "ERR");
  lcd.setCursor(8, 0);
  lcd.print("S2:");
  lcd.print(sensor2Ok ? "OK" : "ERR");
  lcd.setCursor(0, 1);
  lcd.print("Press any button");
  
  delay(2000);
}

/**
 * Функция измерения данных с датчиков
 */
void measureSensors() {
  // Измерение с первого датчика
  byte error1 = sensor1.measure(&temperature1, &humidity1, &dewpoint1);
  if (error1 != 0) {
    Serial.print("Sensor 1 Error: ");
    logError(error1);
    temperature1 = -999.0;  // Индикатор ошибки
    humidity1 = -999.0;
  }
  
  // Измерение со второго датчика
  byte error2 = sensor2.measure(&temperature2, &humidity2, &dewpoint2);
  if (error2 != 0) {
    Serial.print("Sensor 2 Error: ");
    logError(error2);
    temperature2 = -999.0;
    humidity2 = -999.0;
  }
}

/**
 * Функция обновления дисплея в зависимости от режима
 */
void updateDisplay() {
  lcd.clear();
  
  switch (displayMode) {
    case 0:  // Режим: оба датчика
      // Первая строка: Sensor 1
      lcd.setCursor(0, 0);
      lcd.print("S1:");
      if (temperature1 > -100) {
        lcd.print(temperature1, 1);
        lcd.print((char)223);  // Символ градуса
        lcd.print("C");
      } else {
        lcd.print("ERR");
      }
      
      // Вторая строка: Sensor 2
      lcd.setCursor(0, 1);
      lcd.print("S2:");
      if (temperature2 > -100) {
        lcd.print(temperature2, 1);
        lcd.print((char)223);
        lcd.print("C");
      } else {
        lcd.print("ERR");
      }
      break;
      
    case 1:  // Режим: только датчик 1
      lcd.setCursor(0, 0);
      lcd.print("Sensor 1");
      
      lcd.setCursor(0, 1);
      if (temperature1 > -100) {
        lcd.print("T:");
        lcd.print(temperature1, 1);
        lcd.print((char)223);
        lcd.print("C H:");
        lcd.print(humidity1, 1);
        lcd.print("%");
      } else {
        lcd.print("Sensor Error!");
      }
      break;
      
    case 2:  // Режим: только датчик 2
      lcd.setCursor(0, 0);
      lcd.print("Sensor 2");
      
      lcd.setCursor(0, 1);
      if (temperature2 > -100) {
        lcd.print("T:");
        lcd.print(temperature2, 1);
        lcd.print((char)223);
        lcd.print("C H:");
        lcd.print(humidity2, 1);
        lcd.print("%");
      } else {
        lcd.print("Sensor Error!");
      }
      break;
  }
}

/**
 * Функция обработки нажатий кнопок на LCD Shield
 */
void handleButtons() {
  int buttonValue = analogRead(buttonPin);
  
  // Защита от дребезга
  if (millis() - lastButtonPress < debounceDelay) {
    return;
  }
  
  // Определение нажатой кнопки
  if (buttonValue < 100) {  // RIGHT
    displayMode = 0;  // Показать оба датчика
    lastButtonPress = millis();
    Serial.println("Mode: Both sensors");
  } 
  else if (buttonValue < 200) {  // UP
    // Можно добавить функцию, например, калибровку
    lastButtonPress = millis();
  } 
  else if (buttonValue < 400) {  // DOWN
    // Можно добавить функцию, например, сброс
    lastButtonPress = millis();
  } 
  else if (buttonValue < 600) {  // LEFT
    displayMode = 1;  // Показать датчик 1
    lastButtonPress = millis();
    Serial.println("Mode: Sensor 1 only");
  } 
  else if (buttonValue < 800) {  // SELECT
    displayMode = 2;  // Показать датчик 2
    lastButtonPress = millis();
    Serial.println("Mode: Sensor 2 only");
  }
}

/**
 * Функция вывода данных в Serial для отладки
 */
void printToSerial() {
  Serial.println("=== SHT75 Sensors Data ===");
  Serial.print("Sensor 1 - T: ");
  Serial.print(temperature1);
  Serial.print(" C, H: ");
  Serial.print(humidity1);
  Serial.print(" %, DP: ");
  Serial.print(dewpoint1);
  Serial.println(" C");
  
  Serial.print("Sensor 2 - T: ");
  Serial.print(temperature2);
  Serial.print(" C, H: ");
  Serial.print(humidity2);
  Serial.print(" %, DP: ");
  Serial.print(dewpoint2);
  Serial.println(" C");
  
  Serial.print("Display Mode: ");
  switch (displayMode) {
    case 0: Serial.println("Both"); break;
    case 1: Serial.println("Sensor 1"); break;
    case 2: Serial.println("Sensor 2"); break;
  }
  Serial.println("---------------------------");
}

/**
 * Функция вывода ошибок
 */
void logError(byte error) {
  switch (error) {
    case S_Err_NoACK:
      Serial.println("No response from sensor!");
      break;
    case S_Err_CRC:
      Serial.println("CRC mismatch!");
      break;
    case S_Err_TO:
      Serial.println("Measurement timeout!");
      break;
    default:
      Serial.println("Unknown error!");
      break;
  }
}
