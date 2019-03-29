#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <math.h>

#define MAIN_LED PORTB0
#define CALIBRATION_LED PORTB1

#define CALIBRATION_BUTTON PIND7
#define FACTORY_RESET_BUTTON PIND6

#define DEFAULT_DELAY_MS 500
#define SHORT_DELAY_MS 100

#define MAX_ADC 1023
#define DENOISE_VALUE 10

/******************** CONSTANTS ********************/
const uint16_t DEFAULT_DRY_HUMIDITY = 512;
//difference between lower and upper humidity limit
const uint16_t LIMIT_GAP = 20;
/******************** CONSTANTS ********************/


/******************** VARIABLES ********************/
uint8_t* eeprom_is_calibrated = (uint8_t*)2;
uint16_t* eeprom_calibrated_value = (uint16_t*)0;

//setup() variables:
uint8_t is_flashing, is_calibrating;
volatile uint16_t humidity;
uint16_t lower_humidity_limit,
	 	 upper_humidity_limit;
/******************** VARIABLES ********************/


/********** FUNCTIONS DECLARATIONS **********/
void setup();
void adc_start_conversion();
uint16_t set_upper_limit(uint16_t lower_limit);
void calibrate();
void factory_reset();
/********** FUNCTIONS DECLARATIONS **********/


/******************** INTERRUPTS ********************/
ISR(TIMER1_COMPA_vect) {
	adc_start_conversion();
}

ISR(ADC_vect) {
	if( is_calibrating && (abs(ADC - humidity) > DENOISE_VALUE) ) {
		humidity = ADC;
	} else if( !is_calibrating ) {
		humidity = ADC;
	}
}
/******************** INTERRUPTS ********************/



/**********************************************/
/******************** MAIN ********************/
/**********************************************/
int main(void) {
	setup();
	sei();


    while (1) {
    	if( !(PIND & _BV(CALIBRATION_BUTTON)) ) {
			//calibration button pressed?
    		calibrate();
    	}
    	if( (humidity < lower_humidity_limit) && !is_flashing ) {
			//if wetness goes below lower lewel, turn the LED on
    		PORTB |= _BV(MAIN_LED);
    		is_flashing = 1;
    	}
    	if( (humidity > upper_humidity_limit) && is_flashing ) {
			//if wetness goes above upper level, turn the LED off    	
			PORTB &= ~_BV(MAIN_LED);
    		is_flashing = 0;
    	}
    	if( !(PIND & _BV(FACTORY_RESET_BUTTON)) ) {
			//software reset button pressed?
    		factory_reset();
    	}
    }

    return 0;
}
/**********************************************/
/******************** MAIN ********************/
/**********************************************/



/********** FUNCTIONS DEFINITIONS **********/
void setup() {
	//setting global vars
	is_flashing = is_calibrating = 0;
	humidity = MAX_ADC;

	if( eeprom_read_byte(eeprom_is_calibrated) == 1 ) {
		lower_humidity_limit = eeprom_read_word(eeprom_calibrated_value);
		upper_humidity_limit = set_upper_limit(lower_humidity_limit);
	} else {
		lower_humidity_limit = DEFAULT_DRY_HUMIDITY;
		upper_humidity_limit = DEFAULT_DRY_HUMIDITY + LIMIT_GAP;
	}

	//setting TIMER 1
	TCCR1B |= _BV(WGM12);
	TCCR1B |= _BV(CS12);
	OCR1A = 15624;
	TIMSK1 |= _BV(OCIE1A);

	//setting ADC
	ADMUX |= _BV(REFS0);
	ADMUX |= _BV(MUX0) | _BV(MUX2);
	ADCSRA |= _BV(ADEN) | _BV(ADIE);
	ADCSRA |= _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2);
	//disabling degital output on ADC port
	DIDR0 = _BV(ADC5D);

	//setting ports as inputs and outputs
	DDRB |= _BV(MAIN_LED) | _BV(CALIBRATION_LED);	//outputs
	PORTB = 0x00;	//turned off
	DDRD = 0x00;	//inputs
	PORTD |= _BV(CALIBRATION_BUTTON) | _BV(FACTORY_RESET_BUTTON);	//with pullups

	//blink three times
	for( uint8_t i = 0; i < 3; ++i) {
		_delay_ms(SHORT_DELAY_MS);
		PORTB |= _BV(MAIN_LED);
		_delay_ms(SHORT_DELAY_MS);
		PORTB &= ~_BV(MAIN_LED);
		_delay_ms(SHORT_DELAY_MS);
	}

	//wait a while before first check
	_delay_ms(DEFAULT_DELAY_MS);

	//start first soil check
	adc_start_conversion();
}

uint16_t set_upper_limit(uint16_t lower_limit) {
	if(lower_limit < (MAX_ADC - LIMIT_GAP)) {
		return (lower_limit + LIMIT_GAP);
	} else {
		return MAX_ADC;
	}

}

void adc_start_conversion() {
	ADCSRA |= _BV(ADSC);
}

void calibrate() {
	//turn calibration diode on to inform about calibration begining
	PORTB |= _BV(CALIBRATION_LED);

	//enable denoising in ADC interrupt
	is_calibrating = 1;

	//turn main LED off
	is_flashing = 0;
	PORTB &= ~_BV(MAIN_LED);

	//wait for a stable humidity level
	_delay_ms(SHORT_DELAY_MS);
	uint16_t calibrated_value = humidity + DENOISE_VALUE;

	//save to eeprom that calibrationg was performed and save the value
	eeprom_update_byte(eeprom_is_calibrated, 1);
	eeprom_update_word(eeprom_calibrated_value, calibrated_value);

	//wait until button released
	while( !(PIND & _BV(CALIBRATION_BUTTON)) );

	//set humidity levels
	lower_humidity_limit = calibrated_value;
	upper_humidity_limit = calibrated_value + LIMIT_GAP;

	//wait a moment and turn the lights off
	_delay_ms(DEFAULT_DELAY_MS);
	PORTB &= ~_BV(CALIBRATION_LED);

	//disable ADC denoising
	is_calibrating = 0;
}

void factory_reset() {
	//turn all two diodes on and reset calibration data
	PORTB |= _BV(MAIN_LED) | _BV(CALIBRATION_LED);

	//erase calibration data
	eeprom_update_byte(eeprom_is_calibrated, 0);
	eeprom_update_word(eeprom_calibrated_value, 0);

	//wait for releasing the button
	while( !(PIND & _BV(FACTORY_RESET_BUTTON)) );

	//wait a while and turn informing diodes off
	_delay_ms(DEFAULT_DELAY_MS);
	PORTB &= ~(_BV(MAIN_LED) | _BV(CALIBRATION_LED));

	//reset used registers
	TCCR1B = 0x00;
	TIMSK1 = 0x00;
	ADMUX = 0x00;
	ADCSRA = 0x00;
	DDRB = 0x00;

	setup();
}
/********** FUNCTIONS DEFINITIONS **********/
