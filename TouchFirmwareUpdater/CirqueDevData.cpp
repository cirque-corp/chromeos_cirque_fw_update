// Copyright (c) 2019 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "CirqueDevData.h"
#include <algorithm>

CirqueDevData::CirqueDevData(CirqueBootloaderCollection *cirque_bl)
{
	this->bl = cirque_bl;

	// Get X/Y dimensions of the touchpad
	vector<uint8_t> dimensions = this->bl->ExtendedRead(0x2001080C, 2);
	this->X_COUNT = dimensions[0];
	this->Y_COUNT = dimensions[1];

	// Get logical scaling values
	uint8_t logical_scalar_flags = this->bl->ExtendedRead(0x20080018, 1)[0];
	this->INVERT_X = ( (logical_scalar_flags & 0x01) == 0 ) ? 0 : 1;
	this->INVERT_Y = ( (logical_scalar_flags & 0x02) == 0 ) ? 0 : 1;
}

CirqueDevData::~CirqueDevData()
{

}

vector<int16_t> CirqueDevData::ConvertStreamToInt16Array(vector<uint8_t> &data_bytes)
{
	vector<int16_t> data;

	if(this->bl->IS_BIG_ENDIAN == 0)
	{
		// Process data as little-endian
		for(int i = 0; i < data_bytes.size(); i += 2)
		{
			int16_t val = (int16_t)((data_bytes[i] & 0xFF) | ((data_bytes[i+1] << 8) & 0xFF00));
			data.push_back(val);
		}
	}
	else
	{
		// Process data as big-endian
		for(int i = 0; i < data_bytes.size(); i += 2)
		{
			int16_t val = (int16_t)( ((data_bytes[i] << 8) & 0xFF00) | (data_bytes[i+1] & 0xFF) );
			data.push_back(val);
		}

	}

	return data;
}

void CirqueDevData::CorrectAxisInversion(vector<vector<int16_t>> &array_2d)
{
	if( this->INVERT_X != 0)
	{
		for(int row = 0; row < array_2d.size(); ++row)
		{
			std::reverse(array_2d[row].begin(), array_2d[row].end());
		}
	}

	if(this->INVERT_Y != 0)
	{
		std::reverse(array_2d.begin(), array_2d.end());
	}
}

vector<vector<int16_t>> CirqueDevData::GetImage(uint32_t image_index)
{
	uint32_t base_addr = 0x30000000 + image_index;

	// Request image
	vector<uint8_t> request_data = {0x01, 0x00};
	this->bl->ExtendedWrite(base_addr, request_data);

	// Wait for image to be ready
	uint16_t length = 0;
	while(length == 0)
	{
		vector<uint8_t> length_bytes = this->bl->ExtendedRead(base_addr, 2);
		length = length_bytes[0] + (length_bytes[1] << 8);
	}

	// Read image bytes
	vector<uint8_t> image_buffer;
	uint16_t offset = 0;
	while(length != 0)
	{
		uint16_t transaction_length =
			( length > this->MAX_IMAGE_TRANSFER_LENGTH ) ?
			this->MAX_IMAGE_TRANSFER_LENGTH :
			length;

		vector<uint8_t> data_bytes =
			this->bl->ExtendedRead(
				base_addr + 2 + offset,
				transaction_length);
		
		image_buffer.insert(image_buffer.end(), data_bytes.begin(), data_bytes.end());

		length -= transaction_length;
		offset += transaction_length;
	}

	// Release image
	request_data[0] = 0;
	request_data[1] = 1;
	this->bl->ExtendedWrite(base_addr, request_data);

	vector<int16_t> array_i16_1d = this->ConvertStreamToInt16Array(image_buffer);

	// Convert to 2D array, to better match touchpad
	vector<vector<int16_t>> array_i16_2d;
	vector<int16_t> row;
	for (int y = 0; y < this->Y_COUNT; ++y)
	{
		row.clear();
		for (int x = 0; x < this->X_COUNT; ++x)
		{
			row.push_back(array_i16_1d[(y * this->X_COUNT) + x]);
		}
		array_i16_2d.push_back(row);
	}

	this->CorrectAxisInversion(array_i16_2d);

	return array_i16_2d;
}

vector<vector<int16_t>> CirqueDevData::GetCompensationImage()
{
	return this->GetImage(this->DEV_DATA_COMP);
}

vector<vector<int16_t>> CirqueDevData::GetRawMeasurementImage()
{
	return this->GetImage(this->DEV_DATA_PRE_DEMUX);
}

vector<vector<int16_t>> CirqueDevData::GetUncompensatedImage()
{
	return this->GetImage(this->DEV_DATA_PRE_COMP);
}

vector<vector<int16_t>> CirqueDevData::GetCompensatedImage()
{
	return this->GetImage(this->DEV_DATA_POST_COMP);
}

string CirqueDevData::PrintImageArray(string title, vector<vector<int16_t>> &image)
{
	string formatted_str = title;
	formatted_str += ":\n";

	for(int row = 0; row < image.size(); ++row)
	{
		for(int i = 0; i < image[row].size(); ++i)
		{
			char buffer[20];
			int chars_written;

			chars_written = snprintf(buffer, sizeof(buffer), "%6d,", image[row][i]);
			formatted_str += string(buffer);
		}
		formatted_str += "\n";
	}

	return formatted_str;
}

void CirqueDevData::GetVersionInfo(uint32_t &fw_revision, int &is_dirty, int &is_branch)
{
	vector<uint8_t> rev_bytes = this->bl->ExtendedRead(0x20000810, 4);

	if(this->bl->IS_BIG_ENDIAN == 0)
	{
		fw_revision = rev_bytes[0]
			| ((rev_bytes[1] << 8) & 0xFF00)
			| ((rev_bytes[2] << 16) & 0xFF0000)
			| ((rev_bytes[3] << 24) & 0xFF000000);
	}
	else
	{
		fw_revision = rev_bytes[3]
			| ((rev_bytes[2] << 8) & 0xFF00)
			| ((rev_bytes[1] << 16) & 0xFF0000)
			| ((rev_bytes[0] << 24) & 0xFF000000);
	}

	is_dirty = ( (fw_revision & 0x80000000) != 0 ) ? 1 : 0;
	is_branch = ( (fw_revision & 0x40000000) != 0 ) ? 1 : 0;
	fw_revision &= 0x3FFFFFFF;
}
