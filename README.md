# AVR soil humidity sensor

***
**NOTE**: This is my university project that I have finished one year ago (2018.02), I am sure that today I would do it little differently (more optimal and easier to read). 
***

This is a simple project of controller that checks soil humidity by analog meter. I used ATmega328P microprocessor which program was written in AVR-C.

#### How it works - user's point of view  
To communicate with user, board have two diodes and three buttons:  

- LED for indicating about dry soil (**MAIN_LED**)   
- LED for indicating about calibration (**CALIBRATION_LED**)    
- button for reguesting calibration (**CALIBRATION_BUTTON**)   
- button for requesting software reset (**FACTORY\_RESET\_BUTTON**)    
- button for requesting hardware reset (plugged directly to AVR's RESET pin)    
    
After plugging a battery to board, it setups itsef and then it blinks MAIN\_LED three times to tell the user that it started to work. During that work, processor constantly checks soil humidity and it turns the MAIN\_LED on and keeps it on until soil humidity changes.  
Some plants need other humidity level (cactus for example needs more dry soil than bonsai), so there is ability to calibrate the level of wetness that is interpreted as dry. To do that you need to push CALIBRATION\_BUTTON when the soil is finally too dry. In that moment, CALIBRATION\_LED turns on for a while, concurrently saving current humidity level to EEPROM, which is immune for hardware reset, so there is no need to calibrate after pushing hardware reset button.  
If there is need to remove calibrated dry soil level and restoring default level, pushing FACTORY\_RESET\_BUTTON is needed. After that, all two LEDs turns on for a while, which informs about removing calibrated values and resettign used internal registers.

#### Something about a code

Controller need to be immune for inaccuracies and noises from internall analog to digital converter (ADC), so there are two variables describing dry soil level - lower\_humidity\_limit and upper\_humidity\_limit. It works like voltages levels in Schmitt trigger - MAIN\_LED turns on only when humidity level goes below lower limit, and turns off only when humidity goes above upper limit.  
There is a ability to manipulate gap value between upper and lower limit by using LIMIT\_GAP constant.   
  
During program's starting up, it checks if there is stored value with calibrated dry level - in that case, it loads that value as threshold, in other case it uses default value.

#### Some things that could be done better
I know there is some things that could be done better (and they will someday). These are examples that I see today: 

1. Using sleep mechanism to lower battery usage  
2. Using simpler microprecessor (ATmega328P isn't that necessary)
3. Not using abs() function in ISR(ADC\_vect), there could be used bit shift of ADC instead
