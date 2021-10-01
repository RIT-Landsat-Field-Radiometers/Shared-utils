/*
 * DS28CM00ID.cpp
 *
 *  Created on: Jun 14, 2021
 *      Author: reedt
 */

#include "DS28CM00ID.h"

DS28CM00_ID::DS28CM00_ID(I2C_HandleTypeDef *i2c) :
		i2c(i2c)
{

}

uint8_t DS28CM00_ID::getFamily()
{
	uint8_t buf[8];
	buf[0] = 0xBB;
	buf[1] = 0xAA;
	auto retval = HAL_I2C_Mem_Read(this->i2c, this->address, REG::FAMILY, 1, buf, 8, -1);
	if (retval != HAL_OK)
	{
		return 0;
	}

	if (retval != HAL_OK)
	{
		return 0;
	}


	if (buf[7] != calcCRC(buf))
	{
		return 0;
	}
	return buf[0];
}

uint64_t DS28CM00_ID::getID()
{
	union id_code
	{
		uint8_t raw[8];
		uint64_t id;
	};
	union id_code buf;
	auto retval = HAL_I2C_Mem_Read(this->i2c, this->address, REG::FAMILY, 1,
			buf.raw, 8, -1);

	if (retval != HAL_OK)
	{
		return 0;
	}
	if (buf.raw[7] != calcCRC(buf.raw))
	{
		return 0;
	}
	return buf.id;
}

uint8_t DS28CM00_ID::getCRC()
{
	uint8_t buf[1];
	auto retval = HAL_I2C_Mem_Read(this->i2c, this->address, REG::_CRC, 1, buf,
			1, -1);
	if (retval != HAL_OK)
	{
		return 0;
	}
	return buf[0];
}

DS28CM00_ID::MODE DS28CM00_ID::getMode()
{
	uint8_t buf[1];
	auto retval = HAL_I2C_Mem_Read(this->i2c, this->address, REG::_MODE, 1, buf, 1, -1);
	if (retval != HAL_OK)
	{
		return MODE::SMBUS; // default
	}
	if ((buf[0] & 0x01) == 1)
	{
		return MODE::SMBUS;
	}
	else
	{
		return MODE::I2C;
	}
}

uint8_t DS28CM00_ID::calcCRC(uint8_t *data)
{
    uint8_t crc = 0x00;
    for (uint8_t idx = 0; idx < 7; idx++)
    {
        crc = crc8_maxim_table[crc ^ data[idx]];
    }
    return crc;
}

void DS28CM00_ID::setMode(MODE mode)
{
	uint8_t buf[1] =
	{ (uint8_t) mode };
	HAL_I2C_Mem_Write(this->i2c, this->address, REG::_MODE, 1, buf, 1, -1);
}

const uint8_t DS28CM00_ID::crc8_maxim_table[256] =
{
		0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65, 157,
		195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220, 35,
		125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98, 190,
		224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255, 70,
		24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
		219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
		101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
		248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
		140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147,
		205, 17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236,
		14, 80, 175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82,
		176, 238, 50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145,
		207, 45, 115, 202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105,
		55, 213, 139, 87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119,
		244, 170, 72, 22, 233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151,
		201, 74, 20, 246, 168, 116, 42, 200, 150, 21, 75, 169, 247, 182, 232,
		10, 84, 215, 137, 107, 53
};
