### Joulemeter

A lesser known project for the Eco Marathon. Efficiency is one of the most important metrics in the competition and the aim is to obtain the highest efficiency possible. A joulemeter will be provided in the competition, however back at Imperial College, we need our own. This project prototype an initial joulemeter for energy consumption tracking and datalogging. 

The first design used the wrong sized components - the smallest components available (0201). 

The second design used more visible and easier to solder components - 0603/0805. Additional grounding planes were added. The general concept of the joulemeter was valid, but required improvement. 

- The ESP32's internal ADC is pretty ass. It can't follow a straight line - this was the main failure of the v2.0 design because the current and voltage readings were poo. Instead a discrete I2C ADC should be used - I am leaning towards the ADS1015 or ADS1115 (differences in resolution and sampling frequency). These have been used in my FYP and are very good. Designing a new PCB to accomodate this component is unnecessary; instead a breakout board/module will be sufficient - this can be from adafruit (works best) or piMöröni, tbh any breakout module will work here. Connect the wires to the corresponding wires on the PCB and change the I2C bus address if necessary. This ADC is very accurate so bypass the "TI circuit" entirely and solder wires from the input channels of the ADC to the various test points scattered across the board. There will be adequate space within the enclosure to stuff it.

- Code uses the classical superloop, which bottlenecks the system. Typically tasks take different times to execute and using FreeRTOS makes better use of system resources. One should consider using it to have greater control of the SD card writing frequency. This should be done by those who are/plan to take the EEE3/EIE3 spring module: Embedded systems, but not mandatory. I tried it once but the Expressif Watchdog forcefully resets the ESP32 when writing the to the SD card (conflict with internal SPI flash).

- The ESP32 fails to boot the whole system from a 4V supply from the battery connector. Strange, but probably due to system inrush current. If the current demand exceeds what is delivered, there will be a voltage sag - this was observed. The ESP32s can operate all the way down to 2.7V, aligning with typical lithium cell voltages. If (for some reason) are using WiFi or ESPNow, this voltage requirement increases to 3V. Find a fix and don't cause a fire otherwise Paul Norman and Daniel Harvey (and probably Zia Rahman) will beat the shit out of you for using an unauthorised nuclear warhead without approval (stiff lithium pouch cells are a "lol no" according to Daniel Harvey's battery whitelist).

- RESET button is a botched job, essentially one pole connects to GND and the other to RST. This provides an external button to push to RESET the ESP32 - this is better than using a screwdriver to remove the lid of the enclosure and press the tiny onboard button. 

- The display and SD card functions work so you dont need to worry about that. The code for getting that shit to work is here: https://github.com/TheRealAnthonyNG/IEM23-24-Joulemeter

- DONT LET ANYONE FROM ROBOTICS SOCIETY (Level 5 EEE) NICK ANY ESP32s - they cost £10 from DigiKey.

- Power the ESP32 with 2V7-3V6 using the 3V3 pins, or USB, or 3V0-5V5 through the VCC pin. Failure to do so results in decorative smoke and a burn mark.

- Don't take this to Canary Wharf else you will be suspected as a terrorist in possession of an explosive

- The power slider on the RHS is a very good fidget mechanism until it breaks. You'll have fun attempting the replace that. 
