/*
Copyright 2019 Cirque Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "CirqueBootloaderCollection.h"
#include <stdexcept>

#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

CirqueBootloaderCollection::CirqueBootloaderCollection(string& device_path, int report_id)
{
	this->hid_report_id = report_id;

	this->fd = open(device_path.c_str(), O_RDWR);

	if(this->fd == -1)
	{
		string errorMsg = "Could not open path: " + device_path + "\n";
		throw std::runtime_error(errorMsg);
	}
}

CirqueBootloaderCollection::~CirqueBootloaderCollection()
{
	close(this->fd);
}

bool CirqueBootloaderCollection::SanityCheck()
{
	// Check if the device is in bootloader mode.
	CirqueBootloaderStatus status;
	if( GetStatus( status ) != BL_SUCCESS || status.Sentinel != 0x5AC3 && status.Sentinel != 0x6D49 && status.Sentinel != 0x426C ) return false;

	// Sanity check
	vector<uint8_t> base_addr;
	this->ExtendedRead(0x20000800, 4, base_addr);
	if(
		(base_addr.size() < 1) ||
		(base_addr[0] != 0x00) ||
		(base_addr[1] != 0x08) ||
		(base_addr[2] != 0x00) ||
		(base_addr[3] != 0x20)
	)
	{
		return false;
	}

	// Record the endian-ness of the part
	vector<uint8_t> is_big_endian;
	this->ExtendedRead(0x20000824, 1, is_big_endian);
	if( is_big_endian.size() < 1 ) return false;

	this->IS_BIG_ENDIAN = ((is_big_endian[0] & 0x01) == 0) ? 0 : 1;
	return true;
}

int CirqueBootloaderCollection::GenFeatureIOCTL(int length, int get_not_set)
{
	int x = (1 + 2) << 30;
	x += (length << 16);
	x += (0x48) << 8;

	if(get_not_set != 0)
	{
		x += 0x07;
	}
	else
	{
		x += 0x06;
	}

	return x;
}

int CirqueBootloaderCollection::GenGETFeatureIOCTL(int length)
{
	return this->GenFeatureIOCTL(length, 1);
}

int CirqueBootloaderCollection::GenSETFeatureIOCTL(int length)
{
	return this->GenFeatureIOCTL(length, 0);
}

void CirqueBootloaderCollection::AppendU32toBuffer(uint32_t value, vector<uint8_t> &data)
{
	data.push_back((value >> 0) & 0xFF);
	data.push_back((value >> 8) & 0xFF);
	data.push_back((value >> 16) & 0xFF);
	data.push_back((value >> 24) & 0xFF);
}

void CirqueBootloaderCollection::AppendU16toBuffer(uint16_t value, vector<uint8_t> &data)
{
	data.push_back((value >> 0) & 0xFF);
	data.push_back((value >> 8) & 0xFF);
}

uint32_t CirqueBootloaderCollection::GetU32FromBuffer(uint8_t * buffer)
{
	uint32_t val = buffer[0] + (buffer[1] << 8) + (buffer[2] << 16) + (buffer[3] << 24);
	return val;
}

uint16_t CirqueBootloaderCollection::GetU16FromBuffer(uint8_t * buffer)
{
	uint16_t val = buffer[0] + (buffer[1] << 8);
	return val;
}

void CirqueBootloaderCollection::PadBuffer(vector<uint8_t> &data)
{
	size_t needed_length = this->report_length - data.size();

	for(size_t i = 0; i < needed_length; ++i)
	{
		data.push_back(0);
	}
}

void CirqueBootloaderCollection::ParseReadDataFromStatus(vector<uint8_t> &status_data, uint32_t &addr, uint16_t &length, vector<uint8_t> &return_buffer)
{
	size_t response_start_index = 0;
	addr = 0;
	length = 0;
	return_buffer.clear();

	// Check version number
	if (status_data[3] >= 8)
	{
		response_start_index = 9;
	}
	else
	{
		response_start_index = 6;
	}

	// Get address and length bytes
	addr = this->GetU32FromBuffer(&status_data[response_start_index]);
	length = this->GetU16FromBuffer(&status_data[response_start_index + 4]);

	// Check length
	if ( length > ( this->report_length - response_start_index - 6 ) )
	{
		string errorMsg = "CirqueBootloaderCollection::ParseReadDataFromStatus: bad length: " + to_string(length) + "\n";
		throw std::runtime_error(errorMsg);
	}

	// Create buffer of just the applicable data
	for(size_t i = response_start_index + 6; i < response_start_index + 6 + length; ++i)
	{
		return_buffer.push_back(status_data[i]);
	}
}

uint16_t CirqueBootloaderCollection::Fletcher_16(uint8_t *dataPtr, size_t bytes)
{
	uint16_t sum1 = 0xff;
	uint16_t sum2 = 0xff;

	while (bytes)
	{
		uint32_t tlen = bytes > 20 ? 20 : bytes;
		bytes -= tlen;
		do
		{
			sum2 += sum1 += *dataPtr++;
		} while (--tlen);
		sum1 = (sum1 & 0xff) + (sum1 >> 8);
		sum2 = (sum2 & 0xff) + (sum2 >> 8);
	}

	// Second reduction step to reduce sums to 8 bits
	sum1 = (sum1 & 0xff) + (sum1 >> 8);
	sum2 = (sum2 & 0xff) + (sum2 >> 8);
	return sum2 << 8 | sum1;
}

uint32_t CirqueBootloaderCollection::Fletcher_32(uint8_t *dataPtr, size_t bytes)
{
	uint32_t sum1 = 0xffff;
	uint32_t sum2 = 0xffff;

	while (bytes)
	{
		uint16_t tlen = bytes > 360 ? 360 : bytes;
		bytes -= tlen;
		do
		{
			sum2 += sum1 += *dataPtr++;
		} while (tlen -= sizeof(uint16_t));
		sum1 = (sum1 & 0xffff) + (sum1 >> 16);
		sum2 = (sum2 & 0xffff) + (sum2 >> 16);
	}

	// Second reduction step to reduce sums to 16 bits
	sum1 = (sum1 & 0xffff) + (sum1 >> 16);
	sum2 = (sum2 & 0xffff) + (sum2 >> 16);
	return sum2 << 16 | sum1;
}

int CirqueBootloaderCollection::BootloaderSetFeature(vector<uint8_t> &data)
{
	int sent = 0;
	int ioctl_value = this->GenSETFeatureIOCTL(this->report_length);
	sent = ioctl(this->fd, ioctl_value, &data[0]);
	return sent;
}

int CirqueBootloaderCollection::BootloaderGetFeature(vector<uint8_t> &data)
{
	int received = 0;
	int ioctl_value = this->GenGETFeatureIOCTL(this->report_length);
	received = ioctl(this->fd, ioctl_value, &data[0]);
	return received;
}

vector<uint8_t> CirqueBootloaderCollection::ExtendedRead(uint32_t addr, uint16_t length)
{
	vector<uint8_t> return_buffer;
	this->ExtendedRead(addr, length, return_buffer);
	return return_buffer;
}

void CirqueBootloaderCollection::ExtendedRead(uint32_t addr, uint16_t length, vector<uint8_t> &return_buffer)
{
	vector<uint8_t> buf;

	buf.push_back(this->hid_report_id);
	buf.push_back(this->BL_CMD_READ_MEM);

	this->AppendU32toBuffer(addr,buf);
	this->AppendU16toBuffer(length,buf);
	
	this->report_length = buf.size();

	int bytes_sent = this->BootloaderSetFeature(buf);
	if(bytes_sent != this->report_length)
	{
		string errorMsg = "CirqueBootloaderCollection::ExtendedRead: Sent bytes didn't equal correct number. " + std::to_string(bytes_sent) + "\n";
		throw std::runtime_error(errorMsg);
	}
	
	this->report_length = this->BL_REPORT_LENGTH;
	this->PadBuffer(buf);

	int bytes_received = this->BootloaderGetFeature(buf);
	if(bytes_received != this->report_length)
	{
		string errorMsg = "CirqueBootloaderCollection::ExtendedRead: Received bytes didn't equal correct number. " + std::to_string(bytes_received) + "\n";
		throw std::runtime_error(errorMsg);
	}

	this->ParseReadDataFromStatus(buf, addr, length, return_buffer);
}

void CirqueBootloaderCollection::ExtendedWrite(uint32_t addr, vector<uint8_t> &byte_array)
{
	uint16_t length = byte_array.size();

	vector<uint8_t> buf;
	buf.push_back(this->hid_report_id);
	buf.push_back(this->BL_CMD_WRITE_MEM);

	this->AppendU32toBuffer(addr, buf);
	this->AppendU16toBuffer(length, buf);
	buf.insert(buf.end(), byte_array.begin(), byte_array.end());

	uint16_t checksum = this->Fletcher_16(&buf[1], 1 + 4 + 2 + length);
	this->AppendU16toBuffer(checksum, buf);

	this->PadBuffer(buf);

	int bytes_sent = this->BootloaderSetFeature(buf);
	if(bytes_sent != this->report_length)
	{
		string errorMsg = "CirqueBootloaderCollection::ExtendedWrite: Bytes sent did not match. " + std::to_string(bytes_sent) + "\n";
		throw std::runtime_error(errorMsg);
	}
}

int CirqueBootloaderCollection::GetStatus( CirqueBootloaderStatus& Status )
{
	vector<uint8_t> buf;

	buf.push_back(this->hid_report_id);
	this->PadBuffer(buf);
	
	int bytes_received = this->BootloaderGetFeature(buf);
	if(bytes_received != this->report_length)
	{
		return BL_FAILURE;
	}

	Status.Sentinel = (uint16_t)buf[2] << 8 | buf[1];

	switch( Status.Sentinel )
	{
		case 0x5AC3:
		case 0xC35A:
		case 0x6C42: //'lB'
		case 0x6D49: //'mI'
		case 0x426C: //'Bl' for old firmware
			break;
		default:
			return BL_READ_ERROR;
	}

	Status.Version = buf[3];
	Status.LastError = (ErrorCodes)buf[4];
	Status.Flags = buf[5];
	Status.ImageLayout = (buf[5] & 0x01) != 0 ? Dual : Single;
	Status.ActiveImage = (buf[5] & 0x06) == 0 ? None : ((buf[4] & 0x06) >> 1) == 0 ? One : Two;
	Status.bBusy = (buf[5] & 0x08) != 0;
	Status.bImageValid = (buf[5] & 0x10) != 0;
	Status.bForce = (buf[5] & 0x20) != 0;

	if( Status.Version >= 0x08 )
	{
		Status.AtomicWriteSize = buf[6];
		Status.ByteWriteDelayUs = buf[7];
		Status.RegionFormatDelayMsPer1K = buf[8];
	}
	else
	{
		Status.AtomicWriteSize = Status.ByteWriteDelayUs = Status.RegionFormatDelayMsPer1K = 0;
	}

	return BL_SUCCESS;
}

int CirqueBootloaderCollection::Reset()
{
	vector<uint8_t> buf;

	buf.push_back(this->hid_report_id);
	buf.push_back(this->BL_CMD_RESET);
	this->PadBuffer(buf);

	int bytes_sent = this->BootloaderSetFeature(buf);
	if(bytes_sent != this->report_length)
	{
		return BL_WRITE_ERROR;
	}

	return BL_SUCCESS;
}

int CirqueBootloaderCollection::Invoke()
{
	vector<uint8_t> buf;

	buf.push_back(this->hid_report_id);
	buf.push_back(this->BL_CMD_INVOKE_BL);
	this->PadBuffer(buf);

	int bytes_sent = this->BootloaderSetFeature(buf);
	if(bytes_sent != this->report_length)
	{
		return BL_WRITE_ERROR;
	}

	return BL_SUCCESS;
}

int CirqueBootloaderCollection::FormatImage( uint8_t NumRegions, uint32_t EntryPointAddress, uint8_t I2CAddress, uint16_t HIDDescriptorAddr )
{
	vector<uint8_t> buf;

	buf.push_back(this->hid_report_id);
	buf.push_back(this->BL_CMD_FORMAT_IMAGE);
	buf.push_back(Single);
	buf.push_back(NumRegions);

	this->AppendU32toBuffer(EntryPointAddress, buf);
	this->AppendU16toBuffer(HIDDescriptorAddr, buf);

	buf.push_back(I2CAddress);
	buf.push_back(this->hid_report_id);

	this->PadBuffer(buf);

	int bytes_sent = this->BootloaderSetFeature(buf);
	if(bytes_sent != this->report_length)
	{
		return BL_WRITE_ERROR;
	}

	return BL_SUCCESS;
}

int CirqueBootloaderCollection::FormatRegion( uint8_t RegionNumber, uint32_t RegionOffset, vector<uint8_t>& data )
{
	vector<uint8_t> buf;

	buf.push_back(this->hid_report_id);
	buf.push_back(this->BL_CMD_FORMAT_REGION);
	buf.push_back(RegionNumber);

	this->AppendU32toBuffer(RegionOffset, buf);
	this->AppendU32toBuffer(data.size(), buf);
	this->AppendU32toBuffer(this->Fletcher_32(&data[0], data.size()), buf);

	this->PadBuffer(buf);

	int bytes_sent = this->BootloaderSetFeature(buf);
	if(bytes_sent != this->report_length)
	{
		return BL_WRITE_ERROR;
	}

	return BL_SUCCESS;
}

int CirqueBootloaderCollection::WriteData( uint32_t WriteOffset, uint32_t NumBytes, vector<uint8_t>& data )
{
	vector<uint8_t> buf;

	buf.push_back(this->hid_report_id);
	buf.push_back(this->BL_CMD_WRITE);

	this->AppendU32toBuffer(WriteOffset, buf);
	this->AppendU32toBuffer(NumBytes, buf);

	buf.insert(buf.end(), data.begin(), data.end());

	this->PadBuffer(buf);

	int bytes_sent = this->BootloaderSetFeature(buf);
	if(bytes_sent != this->report_length)
	{
		return BL_WRITE_ERROR;
	}

	return BL_SUCCESS;
}

int CirqueBootloaderCollection::Flush()
{
	vector<uint8_t> buf;

	buf.push_back(this->hid_report_id);
	buf.push_back(this->BL_CMD_FLUSH);
	this->PadBuffer(buf);

	int bytes_sent = this->BootloaderSetFeature(buf);
	if(bytes_sent != this->report_length)
	{
		return BL_WRITE_ERROR;
	}

	return BL_SUCCESS;
}

int CirqueBootloaderCollection::Validate( ValidationType Validation )
{
	vector<uint8_t> buf;

	buf.push_back(this->hid_report_id);
	buf.push_back(this->BL_CMD_VALIDATE);
	buf.push_back(Validation);
	this->PadBuffer(buf);

	int bytes_sent = this->BootloaderSetFeature(buf);
	if(bytes_sent != this->report_length)
	{
		return BL_WRITE_ERROR;
	}

	return BL_SUCCESS;
}

int CirqueBootloaderCollection::GetVersionInfo(uint16_t &vid, uint16_t &pid, uint16_t &rev)
{
	vector<uint8_t> bytes;

	this->ExtendedRead(0x2000080A, 0x20000824 - 0x2000080A + 1, bytes);
	if( bytes.size() < 0x20000824 - 0x2000080A + 1) return BL_READ_ERROR;
	uint8_t is_big_endian = bytes[0x20000824 - 0x2000080A];

	if((is_big_endian & 0x01) == 0)
	{
		vid = bytes[0] | ((bytes[1] << 8) & 0xFF00);
		pid = bytes[2] | ((bytes[3] << 8) & 0xFF00);
		rev = bytes[4] | ((bytes[5] << 8) & 0xFF00);
	}
	else
	{
		vid = bytes[1] | ((bytes[0] << 8) & 0xFF00);
		pid = bytes[3] | ((bytes[2] << 8) & 0xFF00);
		rev = bytes[5] | ((bytes[4] << 8) & 0xFF00);
	}

	return BL_SUCCESS;
}
