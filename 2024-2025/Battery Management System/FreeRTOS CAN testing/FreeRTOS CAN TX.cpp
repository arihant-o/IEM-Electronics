// This is for STM32L432KC only

#include <Arduino.h>

#include <SPI.h>
#include <Wire.h>

#include <STM32_CAN.h>
#include <STM32FreeRTOS.h>

STM32_CAN Can(CAN1, DEF, RX_SIZE_64, TX_SIZE_16);

void thread_CAN_TX(void* param) {
  const TickType_t xFreq = 500/portTICK_PERIOD_MS; // repeat thread every 500 ms
  TickType_t xLastWakeTime = xTaskGetTickCount();

  CAN_message_t CAN_TX_msg;
  CAN_TX_msg.id = (0x4D7);
  CAN_TX_msg.len = 8;
  CAN_TX_msg.buf[0] = 0x3C; 
  CAN_TX_msg.buf[1] = 0x1A; 
  CAN_TX_msg.buf[2] = 0x08; 
  CAN_TX_msg.buf[3] = 0x5F; 
  CAN_TX_msg.buf[4] = 0x4D; 
  CAN_TX_msg.buf[5] = 0x23; 
  CAN_TX_msg.buf[6] = 0xA9; 
  CAN_TX_msg.buf[7] = 0xEE; 

  while(1) {
    Can.write(CAN_TX_msg);

    vTaskDelayUntil(&xLastWakeTime, xFreq);
  }
}

void ISR_toggleLED() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void setup() {
  pinMode(A4, INPUT);

  if(digitalRead(A4) == HIGH) { // pulled up to 3V3
    Can.begin(true); // Automatic retransmission if no ACK received
  }
  else {
    Can.begin();
  }

  Can.setBaudRate(500000);

  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);

  TIM_TypeDef *Instance = TIM1;
  HardwareTimer *LED_toggle = new HardwareTimer(Instance);
  LED_toggle->setOverflow(2, HERTZ_FORMAT);
  LED_toggle->attachInterrupt(ISR_toggleLED);
  LED_toggle->resume();

  xTaskCreate(thread_CAN_TX, "publishes CAN TX message at regular intervals", 1024, NULL, 8, NULL);

  vTaskStartScheduler();
}

void loop() {
  
}

