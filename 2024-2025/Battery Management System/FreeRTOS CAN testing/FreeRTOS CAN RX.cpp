// This is for STM32F303K8

#include <Arduino.h>

#include <SPI.h>
#include <Wire.h>

#include <STM32_CAN.h>
#include <STM32FreeRTOS.h>

STM32_CAN Can(CAN1, DEF, RX_SIZE_64, TX_SIZE_16);

void ISR_toggle_LED() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void ISR_toggle_LED_A1() {
  digitalWrite(A1, !digitalRead(A1));
}

void thread_CAN_RX(void* param) {
  const TickType_t xFreq = 10/portTICK_PERIOD_MS;
  TickType_t xLastWakeTime = xTaskGetTickCount();

  CAN_message_t CAN_RX_msg;

  while(1) {
    if(Can.read(CAN_RX_msg)) {
      digitalWrite(D8, !digitalRead(D8));
    }

    vTaskDelayUntil(&xLastWakeTime, xFreq);
  }

  
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(A1, OUTPUT);

  pinMode(D8, OUTPUT); // diagnostic pin to know when CAN has been received

  Can.begin();
  Can.setBaudRate(500000);

  Serial.begin(115200);

  TIM_TypeDef *Instance = TIM1;

  HardwareTimer *LED_toggle = new HardwareTimer(Instance);
  LED_toggle->setOverflow(2, HERTZ_FORMAT);
  LED_toggle->attachInterrupt(ISR_toggle_LED);
  LED_toggle->resume();

  TIM_TypeDef *Instance1 = TIM15;

  HardwareTimer *LED_toggle_A1 = new HardwareTimer(Instance1);
  LED_toggle_A1->setOverflow(4, HERTZ_FORMAT);
  LED_toggle_A1->attachInterrupt(ISR_toggle_LED_A1);
  LED_toggle_A1->resume();

  xTaskCreate(thread_CAN_RX, "when CAN RX message is recieved", 1024, NULL, 8, NULL);

  vTaskStartScheduler();

}

void loop() {
  
}

