#include <Arduino.h>
#include <Adafruit_SSD1306.h> // Additional documentation: https://randomnerdtutorials.com/guide-for-oled-display-with-arduino/

#include <STM32FreeRTOS.h> // Implementing concurrency

#include <Interrupts.hpp> // Separate header file for interrupts - declutters the main cpp file

// Wired communication
#include <Wire.h> // I2C library
#include <SPI.h> // SPI library
#include <STM32_CAN.h> // CAN bus library for STM32

//#define device1

int counter = 0;

struct {
  int analogVoltage;
  SemaphoreHandle_t analogVoltage_sema;
} systemData;

Adafruit_SSD1306 display(128, 64, &Wire, -1); // Display Driver object

template <typename T> // use a template function to handle different data types
void displayMessage(T message) {
  display.clearDisplay();
  display.setCursor(0,10);
  display.println(message);
  display.display();
}

void readADC_voltage(void* parameter) {
  const TickType_t xFreq = 1/portTICK_PERIOD_MS;
  TickType_t xLastWake = xTaskGetTickCount();

  while(1) {
    if(xSemaphoreTake(systemData.analogVoltage_sema, portMAX_DELAY) == pdTRUE) {
      systemData.analogVoltage = analogRead(A0);
      xSemaphoreGive(systemData.analogVoltage_sema);
      vTaskDelayUntil(&xLastWake, xFreq);
    }
  }
}

void serialAnalogVoltage(void* parameter) {
  const TickType_t xFreq = 1000/portTICK_PERIOD_MS;
  TickType_t xLastWake = xTaskGetTickCount();
  float localVoltage;

  while(1) {
    if(xSemaphoreTake(systemData.analogVoltage_sema, portMAX_DELAY) == pdTRUE) {
      localVoltage = systemData.analogVoltage;
      xSemaphoreGive(systemData.analogVoltage_sema);
      localVoltage = (localVoltage*3.3)/4095;
      Serial.println(localVoltage);
      vTaskDelayUntil(&xLastWake, xFreq); 
    }
  }
}

void updateDisplay(void* parameter) {
  const TickType_t xFreq = 17/portTICK_PERIOD_MS;
  TickType_t xLastWake = xTaskGetTickCount();

  while(1) {
    displayMessage(counter);

    vTaskDelayUntil(&xLastWake, xFreq);
  }
}

void counterIncrement() {
  counter++;
}

void setup() {
  Serial.begin(115200);

  Wire.begin(); // By default D4/D5 are connected to A4/A5 through SB16 and SB18

  // display setup
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // I2C address of this display is 0x3C
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.display();
  delay(1000);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);

  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(A0, INPUT);
  analogReadResolution(12);

  pinMode(D12, OUTPUT);
  digitalWrite(D12, LOW);

  pinMode(D11, OUTPUT);
  digitalWrite(D11, HIGH);

  // hardware timer

  TIM_TypeDef *Instance = TIM1;
  HardwareTimer *LEDTimer = new HardwareTimer(Instance);

  // Period only works with whole numbers, so 1 is the lowest frequency
  LEDTimer->setOverflow(3, HERTZ_FORMAT); // Set timer frequency (first number is frequency)
  LEDTimer->attachInterrupt(discreteledControlISR); // Attach ISR/Interrupt
  LEDTimer->resume(); // Start Timer

  TIM_TypeDef *Instance1 = TIM6;
  HardwareTimer *BuiltinLEDTimer = new HardwareTimer(Instance1);
  BuiltinLEDTimer->setOverflow(1, HERTZ_FORMAT);
  BuiltinLEDTimer->attachInterrupt(builtinledControlISR);
  BuiltinLEDTimer->resume();

  #ifdef device1
    TIM_TypeDef *Instance2 = TIM7;
    HardwareTimer *counterTimer = new HardwareTimer(Instance2);

    counterTimer->setOverflow(1, HERTZ_FORMAT);
    counterTimer->attachInterrupt(counterIncrement);
    counterTimer->resume();
  

    TaskHandle_t displayDriver = NULL;
    xTaskCreate(updateDisplay, "Updates the text on the display", 256, NULL, 4, &displayDriver);
  #endif

  #ifndef device1
    systemData.analogVoltage_sema = xSemaphoreCreateMutex();

    TaskHandle_t readAnalog = NULL;
    xTaskCreate(readADC_voltage, "Reads the analog voltage from ADC", 128, NULL, 10, &readAnalog);

    TaskHandle_t serialPrintAnalog = NULL;
    xTaskCreate(serialAnalogVoltage, "Outputs analog voltage to serial", 128, NULL, 15, &serialPrintAnalog);
  #endif

  vTaskStartScheduler();

}

void loop() {  
}

