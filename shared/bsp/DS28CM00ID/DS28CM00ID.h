/*
 * DS28CM00ID.h
 *
 *  Created on: Jun 14, 2021
 *      Author: reedt
 */

#ifndef INC_DS28CM00ID_H_
#define INC_DS28CM00ID_H_

#include "main.h"

class DS28CM00_ID
{
private:
	I2C_HandleTypeDef *i2c;
	uint8_t address = 0x50 << 1; // stm32 HAL shifts the proveded address for some reason? odd

	enum REG : uint8_t
	{
		FAMILY = 0x00,
		ID_0 = 0x01,
		ID_1 = 0x02,
		ID_2 = 0x03,
		ID_3 = 0x04,
		ID_4 = 0x05,
		ID_5 = 0x06,
		_CRC = 0x07,
		_MODE = 0x08
	};

	uint8_t calcCRC(uint8_t *data);
	uint8_t getCRC();

	static const uint8_t crc8_maxim_table[256];

public:
	DS28CM00_ID(I2C_HandleTypeDef *i2c);
	uint8_t getFamily();
	uint64_t getID();

	typedef enum : uint8_t
	{
		I2C = 0x00, SMBUS = 0x01
	} MODE;

	MODE getMode();
	void setMode(MODE mode);

	virtual ~DS28CM00_ID() = default;
	DS28CM00_ID(const DS28CM00_ID &other) = default;
	DS28CM00_ID(DS28CM00_ID &&other) = default;
	DS28CM00_ID& operator=(const DS28CM00_ID &other) = default;
	DS28CM00_ID& operator=(DS28CM00_ID &&other) = default;
};

#endif /* INC_DS28CM00ID_H_ */
