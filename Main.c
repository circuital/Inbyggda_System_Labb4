#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>

#include "ADC.h"
#include "GPIO.h"
#include "I2C.h"
#include "Serial.h"
#include "Timer.h"

char input[] = "Elvira";
char output[6];
char buffer[6];

int main(void) 
{
	i2c_init();
	uart_init();

	sei();

	while (1)
	{
		//Deluppgift 1-2
		//int i;
		//for (i = 0; i < sizeof(input); i++)
		//{
		//	eeprom_write_byte(DATA_ADDRESS + i, input[i]);
		//}

		//int j;
		//for (j = 0; j < sizeof(output); j++)
		//{
		//	output[j] = eeprom_read_byte(DATA_ADDRESS + j);
		//}

		//printf_P(PSTR("DATA FROM EEPROM: %s\n"), output);

		//Deluppgift 3
		eeprom_write_page(DATA_ADDRESS, input);
		eeprom_sequential_read(buffer, DATA_ADDRESS, sizeof(input));
		printf_P(PSTR("DATA FROM EEPROM: %s\n"), buffer);

		//int i;
		//for (i = 0; i < sizeof(buffer); i++)
		//{
		//	printf_P(PSTR("DATA FROM EEPROM: %x\n"), buffer[i]);
		//}
	}
}

