#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include <stdio.h>
#include <string.h>

#include "I2C.h"

inline uint8_t i2c_get_status(void)
{
	uint8_t status;
	status = TWSR & 0xF8; //Mask status (five upper bits are status bits, three lower bits are prescaler bits)
	return status;
}

void i2c_meaningful_status(uint8_t status) 
{
	switch (status) 
	{
	case 0x08: // START transmitted, proceed to load SLA+W/R
		printf_P(PSTR("START\n"));
		break;
	case 0x10: // repeated START transmitted, proceed to load SLA+W/R
		printf_P(PSTR("RESTART\n"));
		break;
	case 0x38: // NAK or DATA ARBITRATION LOST
		printf_P(PSTR("NOARB/NAK\n"));
		break;
		// MASTER TRANSMIT
	case 0x18: // SLA+W transmitted, ACK received
		printf_P(PSTR("MT SLA+W, ACK\n"));
		break;
	case 0x20: // SLA+W transmitted, NAK received
		printf_P(PSTR("MT SLA+W, NAK\n"));
		break;
	case 0x28: // DATA transmitted, ACK received
		printf_P(PSTR("MT DATA+W, ACK\n"));
		break;
	case 0x30: // DATA transmitted, NAK received
		printf_P(PSTR("MT DATA+W, NAK\n"));
		break;
		// MASTER RECEIVE
	case 0x40: // SLA+R transmitted, ACK received
		printf_P(PSTR("MR SLA+R, ACK\n"));
		break;
	case 0x48: // SLA+R transmitted, NAK received
		printf_P(PSTR("MR SLA+R, NAK\n"));
		break;
	case 0x50: // DATA received, ACK sent
		printf_P(PSTR("MR DATA+R, ACK\n"));
		break;
	case 0x58: // DATA received, NAK sent
		printf_P(PSTR("MR DATA+R, NAK\n"));
		break;
	default:
		printf_P(PSTR("N/A %02X\n"), status);
		break;
	}
}

void i2c_init(void) 
{
	TWCR = (1 << TWEN); //Enable TWI.
	TWBR = 72; //Will result in a clock speed of 100kHz.
	TWSR = 0;  //Prescaler 1.
}

inline void i2c_start() 
{
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN); //Send START condition.
	while (!(TWCR & (1 << TWINT))); //Wait for TWINT Flag set. This indicates that the START condition has been transmitted.
}

inline void i2c_stop() 
{
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN); //Transmit STOP condition.
	while ((TWCR & (1 << TWSTO))); //Wait for stop. 
}

inline void i2c_xmit_addr(uint8_t eeprom_address, uint8_t rw) 
{
	TWDR = (eeprom_address & 0xfe) | (rw & 0x01); //Setting control byte with adress and rw
	TWCR = (1 << TWINT) | (1 << TWEN); //Clear TWINT to start transmission of address. 
	while (!(TWCR & (1 << TWINT))); //Wait for TWINT Flag set. This indicates that the SLA + W has been transmitted, and ACK / NACK has been received.
}

inline void i2c_xmit_byte(uint8_t data) 
{
	TWDR = data;
	TWCR = (1 << TWINT) | (1 << TWEN); //Clear TWINT to start transmission of data. 
	while (!(TWCR & (1 << TWINT))); //Wait for TWINT Flag set. This indicates that the DATA has been transmitted, and ACK / NACK has been received.
}

inline uint8_t i2c_read_ACK() 
{
	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA); //???
	while (!(TWCR & (1 << TWINT))); //Wait for TWINT Flag set.
	return TWDR;
}

inline uint8_t i2c_read_NAK() 
{
	TWCR = (1 << TWINT) | (1 << TWEN); //???
	while (!(TWCR & (1 << TWINT))); //Wait for TWINT Flag set.
	return TWDR;
}

inline void eeprom_wait_until_write_complete() 
{
	while (i2c_get_status() != 0x18) // Wait for MASTER TRANSMIT SLA+W transmitted, ACK received.
	{
		i2c_start();
		i2c_xmit_addr(EEPROM_ADDRESS, I2C_W);
	}
}

void eeprom_write_byte(uint8_t data_address, uint8_t data)
{
	//Start
	i2c_start();

	//Transmit eeprom addres + write
	i2c_xmit_addr(EEPROM_ADDRESS, I2C_W);

	//Transmit data address
	i2c_xmit_byte(data_address);

	//Transmit data
	i2c_xmit_byte(data);

	//Stop
	i2c_stop();

	//Wait for write complete. 
	eeprom_wait_until_write_complete();
}

uint8_t eeprom_read_byte(uint8_t data_address) 
{
	uint8_t data;
	//Start
	i2c_start();
	
	//Transmit eeprom addres + write
	i2c_xmit_addr(EEPROM_ADDRESS, I2C_W);
	
	//Transmit data address
	i2c_xmit_byte(data_address);
	
	//Start
	i2c_start();

	//Transmit eeprom adrress + read
	i2c_xmit_addr(EEPROM_ADDRESS, I2C_R);
	
	//Receive data
	data = i2c_read_NAK();
	
	//Stop
	i2c_stop();

	return data;
}

void eeprom_write_page(uint8_t data_address, uint8_t* data)
{
	//Start
	i2c_start();

	//Transmit eeprom address + write
	i2c_xmit_addr(EEPROM_ADDRESS, I2C_W);

	//Make sure data_address is multiple of 8
	while (data_address % 8 != 0)
	{
		data_address++;
	}

	//Transmit data address
	i2c_xmit_byte(data_address);

	//Transmit data one byte at a time
	int i;
	for (i = 0; i < 8; i++)
	{
		i2c_xmit_byte(data[i]);
	}

	//Stop
	i2c_stop();

	//Wait for write complete. 
	eeprom_wait_until_write_complete();
}

void eeprom_sequential_read(uint8_t* buf, uint8_t data_start_address, uint8_t len)
{
	//Start
	i2c_start();

	//Transmit eeprom address + write
	i2c_xmit_addr(EEPROM_ADDRESS, I2C_W);

	//Transmit data start address
	i2c_xmit_byte(data_start_address);

	//Start
	i2c_start();

	//Transmit eeprom adrress + read
	i2c_xmit_addr(EEPROM_ADDRESS, I2C_R);

	//Receive data one byte at a time
	int i;
	for (i = 0; i < len - 1; i++)
	{
		buf[i] = i2c_read_ACK();
	}

	//Receive last data byte 
	buf[len - 1] = i2c_read_NAK();

	//Stop
	i2c_stop();
}
