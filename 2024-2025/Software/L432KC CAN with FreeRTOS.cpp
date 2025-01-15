
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>


#include <L432_CAN.h>
#include <STM32FreeRTOS.h>


// Queue Handler for CAN messages (In and Out)
QueueHandle_t msgInQ, msgOutQ;


// Global Semaphore handle for CAN TX thread
SemaphoreHandle_t CAN_TX_Semaphore;


// Incoming message variables
uint8_t RX_Message[8];


struct {
 SemaphoreHandle_t mutex; // Protected items are locked by a thread when accessed and released when no longer accessed
 uint8_t var1 = 0;
 uint8_t var2 = 32;
 uint8_t var3 = 42;
 uint8_t var4 = 128; // dummy variables to represent importance pieces of data
} sysState;


const int debugLED = LED_BUILTIN; // May change debug signal to external gpio pin as
                                 // LED_BUILTIN is associated with D13, SPI3 SCK


// Timer Driven:
// Blink the associated debug LED pin at 1 Hz to check for deadlocks
void toggle_DEBUG_led_ISR() {
 digitalWrite(debugLED, !digitalRead(debugLED));
}


// Event Driven:
// Triggered when a new CAN message arrives at the CAN receiver module of the STM32,
// the CAN hardware raises an interrupt signal and executes this:
void CAN_RX_ISR(void) {
uint8_t RX_Message_ISR[8];
uint32_t ID;
CAN_RX(ID, RX_Message_ISR);                          // Receive message from CAN bus
xQueueSendFromISR(msgInQ, RX_Message_ISR, NULL);     // Place data in the queue
}


// Event Driven:
// Triggered when the CAN module is ready to send a new message (i.e. transmit buffer is free),
// or after a message has been successfully transmitted, the CAN hardware raises an interrupt
void CAN_TX_ISR(void) {
 xSemaphoreGiveFromISR(CAN_TX_Semaphore, NULL);
  // Releases the semaphore back to the CAN_TX task
 // Typically signals or notifies the task that it's possible to send another message
}


// CAN TX Thread
void CAN_TX_thread(void* param) {
 uint8_t msgOut[8];


 while(1) {
   xQueueReceive(msgOutQ, msgOut, portMAX_DELAY);
   // Waits for a message to be available in the outgoing message queue (msgOutQ)
   // If the queue is empty, it will block the task until a message is posted to the queue
   // When a message is available, it copies the message back into the msgOut array


   xSemaphoreTake(CAN_TX_Semaphore, portMAX_DELAY);
   CAN_TX(0x123, msgOut); // Send message with ID 0x123 over CAN bus
 }
}


// CAN RX Thread
void CAN_RX_thread(void* param) {
 while(1) {
   xQueueReceive(msgInQ, RX_Message, portMAX_DELAY);
   // Waits for a message to be available in the outgoing message queue (msgInQ)
   // If the queue is empty, it will block the task until a mesage is posted to the queue
   // When a message is available, it copies th emessage into the msgOut array


   xSemaphoreTake(sysState.mutex, portMAX_DELAY);


   // Decode elemnts of RX message and store to sysState variables here


   xSemaphoreGive(sysState.mutex); // Release the mutex after accessing the resources
 }
}




void setup() {
 Serial.begin(115200);


 pinMode(debugLED, OUTPUT);


 TIM_TypeDef *Instance = TIM2;                     // General purpose timer
 HardwareTimer *toggle_Debug_LED = new HardwareTimer (Instance);
 toggle_Debug_LED->setOverflow(2, HERTZ_FORMAT);
 toggle_Debug_LED->attachInterrupt(toggle_DEBUG_led_ISR);
 toggle_Debug_LED->resume();


 CAN_Init();                                       // Enable loopback mode (recieve and acknowledge its own message)
 setCANFilter(0x123, 0x7ff);                       // Set up filter to receive messages with ID 0x123 only


 CAN_TX_Semaphore = xSemaphoreCreateCounting(3,3); // 3 mailbox states, will block transmit thread on 4th attempt
                                                   // until a semaphore is released by the ISR
 CAN_RegisterRX_ISR(CAN_RX_ISR);                   // Register the ISR function for receiving CAN messages
 CAN_RegisterTX_ISR(CAN_TX_ISR);                   // Register the ISR function for transmitting CAN messages


 CAN_Start();


 msgOutQ = xQueueCreate(36,8);                     // Create a queue with capacity for 36 items, 8 bytes each
 msgInQ = xQueueCreate(36, 8);


 sysState.mutex = xSemaphoreCreateMutex();


 xTaskCreate(CAN_RX_thread, "CAN RX thread", 1024, NULL, 3, NULL);
 xTaskCreate(CAN_TX_thread, "CAN TX thread", 1024, NULL, 2, NULL);










 vTaskStartScheduler();
}


void loop() {}





