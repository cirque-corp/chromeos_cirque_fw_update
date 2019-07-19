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

#include <string>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <sys/stat.h>
#include "dirent.h"
#include "CirqueBootloaderCollection.h"
#include "CirqueDevData.h"
#include "CirqueHexFileParser.h"

using namespace std;

vector<string> find_cirque_devices(void)
{
	DIR * hidraw_dir = opendir("/sys/class/hidraw");
	vector<string> devices;

	if(hidraw_dir != NULL)
	{
		struct dirent * entry = readdir(hidraw_dir);
		
		while (entry != NULL)
		{
			if(strncmp("hidraw",entry->d_name, 6) == 0)
			{
				// Find the device that is pointed to by the "device" symlink
				string device_link = string("/sys/class/hidraw/");
				device_link += entry->d_name;
				device_link += "/device";

				char link_buffer[512] = {0};
				
				ssize_t count = readlink(device_link.c_str(), link_buffer, sizeof(link_buffer));
				string link = string(link_buffer);
				size_t last_slash = link.find_last_of("/");
				string device_id_name = link.substr(last_slash + 1);

				// device_id_name is of the form BusID:VID:PID.#
				// split them up into a vector
				while(device_id_name.find_first_of(".") != string::npos)
				{
					device_id_name.replace(device_id_name.find_first_of("."), 1, ":");
				}
				
				vector<string> device_id_elements;
				size_t start_position = 0;
				while(start_position != string::npos)
				{
					size_t search_start = (start_position == 0) ? 0 : start_position + 1;
					size_t delimiter_pos = device_id_name.find(":", search_start);
					size_t length = (delimiter_pos == string::npos) ? string::npos : delimiter_pos - search_start;
					
					device_id_elements.push_back( device_id_name.substr(search_start, length) );
					start_position = delimiter_pos;
				}

				// Check if the VID matches Cirque's VID
				if(device_id_elements[1].compare("0488") == 0)
				{
					string dev_path = "/dev/";
					dev_path += entry->d_name;
					devices.push_back(dev_path);
				}
			}
			
			entry = readdir(hidraw_dir);
		}

		closedir(hidraw_dir);
	}

	return devices;
}

void dump_raw_data(string hid_device_path)
{
	CirqueBootloaderCollection bl(hid_device_path);
	if( !bl.SanityCheck() ) return;

	CirqueDevData dev_data(&bl);

	uint32_t rev = 0;
	int is_dirty = 1;
	int is_branch = 1;
	dev_data.GetVersionInfo(rev, is_dirty, is_branch);
	printf("  rev: %d, %s %s\n",
		rev,
		(is_dirty)?"Dirty":"Pristine",
		(is_branch)?"Branch":"Trunk");

	// Disable normal feeds while gathering data
	uint8_t feed_cfg2 = bl.ExtendedRead(0x200E0009,1)[0];
	uint8_t feed_control = bl.ExtendedRead(0x200E000A,1)[0];
	vector<uint8_t> write_data = {(uint8_t)(feed_control & 0xF8)};
	bl.ExtendedWrite(0x200E000A, write_data);

	// Sleep to allow touch buffer to empty out
	usleep(50*1000);

	vector<vector<int16_t>> image = dev_data.GetCompensationImage();
	cout << dev_data.PrintImageArray(string("Current Compensation Matrix"),
		image);

	image = dev_data.GetRawMeasurementImage();
	cout << dev_data.PrintImageArray(string("Live Raw Measurements"),
		image);

	image = dev_data.GetUncompensatedImage();
	cout << dev_data.PrintImageArray(string("Live Uncompensated Image"),
		image);

	image = dev_data.GetCompensatedImage();
	cout << dev_data.PrintImageArray(string("Live Compensated Image"),
		image);

	// Restore feed control settings
	write_data[0] = (uint8_t) ((feed_control & 0xF8) | (1 << (feed_cfg2 & 0x03)));
	bl.ExtendedWrite(0x200E000A, write_data);
}

uint16_t get_device_attributes(string& hid_device_path)
{
	int retval = BL_FAILURE;
	CirqueBootloaderCollection bl(hid_device_path);

	uint16_t vid = 0, pid = 0, rev = 0;
	if( bl.GetVersionInfo(vid, pid, rev) == BL_SUCCESS )
	{
		printf("  1: VID %04X  PID %04X  REV %04X\n", vid, pid, rev );
	}
	else
	{
		printf("Failed to get device firmware version.\n" );
	}
	return rev;
}

uint16_t get_fw_version(string& hid_device_path)
{
	int retval = BL_FAILURE;
	CirqueBootloaderCollection bl(hid_device_path);

	uint16_t vid = 0, pid = 0, rev = 0;
	if( bl.GetVersionInfo(vid, pid, rev) == BL_SUCCESS )
	{
		printf("%02X.%02X\n", rev >> 8, rev & 0xFF );
	}
	return retval;
}

int update_firmware(string& hid_device_path, string& hex_file_path)
{
	CirqueBootloaderCollection bl(hid_device_path);

	// Load and parse the hex file.
	CirqueHexFileParser hfp(hex_file_path);
	int retval = hfp.Parse();
	switch( retval )
	{
		case HEX_NOFILE:
			printf("Firmware file %s does not exist.\nFirmware update failed.\n", hex_file_path.c_str());
			return retval;
		case HEX_CORRUPT:
			printf("Firmware file %s is corrupted.\nFirmware update failed.\n", hex_file_path.c_str());
			return retval;
		default:
			break;
	}
	printf("Finished parsing %s: %d records.\n", hex_file_path.c_str(), (int)hfp.recList.size());

	// Get timing values.
	uint32_t FormatImageDelay = 100;
	uint32_t FormatRegionsPageDelay = 50;
	uint32_t PageWriteDelay = 10;

	CirqueBootloaderStatus status;
	retval = bl.GetStatus( status );
	printf("GetStatus returned %d.\n", retval);
	if( retval != BL_SUCCESS ) return retval;

	printf( "Status: Sentinel 0x%04X Version 0x%02X Error %d\n", status.Sentinel, status.Version, status.LastError );

	if( status.LastError != NV_err_none )
	{
		// Clear the error and retry.
		retval = bl.Reset();
		printf("Reset returned %d.\n", retval);
		if( retval != BL_SUCCESS ) return retval;
		usleep(10000);

		// Check status.
		if (bl.GetStatus( status ) != BL_SUCCESS || status.LastError != NV_err_none)
		{
			printf("GetStatus after reset failed with error %d.\n", status.LastError);
			return BL_FAILURE;
		}
	}

	// Invoke bootloader.
	if( status.Sentinel == 0x5AC3 || status.Sentinel == 0x6D49 || status.Sentinel == 0x426C )
	{
		retval = bl.Invoke();
		printf("Invoke bootloader returned %d.\n", retval);
		if( retval != BL_SUCCESS ) return retval;
		usleep(100000);

		retval = bl.GetStatus( status );
		printf("GetStatus returned %d.\n", retval);
		if( retval != BL_SUCCESS ) return retval;

		printf( "Status: Sentinel 0x%04X Version 0x%02X Error %d\n", status.Sentinel, status.Version, status.LastError );

	}
	if( status.Version >= 0x08)
	{
		FormatRegionsPageDelay = status.RegionFormatDelayMsPer1K;
		PageWriteDelay = status.ByteWriteDelayUs;
	}
	printf("Timing values: FormatImageDelay %d, FormatRegionsPageDelay %d, PageWriteDelay %d.\n", FormatImageDelay, FormatRegionsPageDelay, PageWriteDelay);

	// Format image.
	uint32_t EntryPoint = hfp.recList[0]->buf[4] |
						( hfp.recList[0]->buf[5] << 8 ) |
						( hfp.recList[0]->buf[6] << 16 ) |
						( hfp.recList[0]->buf[7] << 24 );

	uint8_t TargetI2CAddress = 0x2C;
	uint16_t TargetHIDDescAddr = 0x0020;
	if( status.Version >= 0x09)
	{
		TargetI2CAddress = 0xFF;
		TargetHIDDescAddr = 0xFFFF;
	}

	printf("FormatImage called with size %d, entry point 0x%08X, I2C address 0x%02X, HID descriptor address 0x%04X.\n", (uint8_t)hfp.recList.size(), EntryPoint, TargetI2CAddress, TargetHIDDescAddr );
	retval = bl.FormatImage( (uint8_t)hfp.recList.size(), EntryPoint, TargetI2CAddress, TargetHIDDescAddr );
	printf("FormatImage returned %d.\n", retval);
	if( retval != BL_SUCCESS ) return retval;

	usleep(FormatImageDelay * 1000);

	// Format regions.
	for( int temp = 0; temp < hfp.recList.size(); temp++ )
	{
		retval = bl.FormatRegion( (uint8_t)temp, hfp.recList[temp]->getAddress(), hfp.recList[temp]->buf );
		printf("FormatRegion returned %d.\n", retval);
		if( retval != BL_SUCCESS ) return retval;

		usleep((FormatRegionsPageDelay * 1000 * ((hfp.recList[temp]->buf.size() / 1024) + 1)));

		if (bl.GetStatus( status ) != BL_SUCCESS || status.LastError != NV_err_none)
		{
			printf("GetStatus failed with error %d.\n", status.LastError);
			return BL_FAILURE;
		}
		printf("GetStatus returned %d.\n", BL_SUCCESS);
	}

	// Write data.
	uint32_t Length, MaxDataPayloadSize = 520; // must be even, better if multiple of 4 (520 / 4 = 130)

	for (int i = 0; i < hfp.recList.size(); i++)
	{
		Length = (uint32_t)hfp.recList[i]->buf.size();
		printf("Writing %d bytes of data.\n", Length);

		while (Length > 0)
		{
			uint32_t PayloadSize = ( Length > MaxDataPayloadSize ) ? MaxDataPayloadSize : Length;

			vector<uint8_t> Payload;
			Payload.clear();
			Payload.insert( Payload.end(), hfp.recList[i]->buf.begin() + hfp.recList[i]->buf.size() - Length, hfp.recList[i]->buf.begin() + hfp.recList[i]->buf.size() - Length + PayloadSize );

			retval = bl.WriteData( (uint32_t)hfp.recList[i]->getAddress() + (hfp.recList[i]->buf.size() - Length), PayloadSize, Payload );
			if( retval != BL_SUCCESS ) return retval;

			usleep( ( PageWriteDelay * PayloadSize > 1000 ) ? PageWriteDelay * PayloadSize : 1000 );

			if (bl.GetStatus( status ) != BL_SUCCESS || status.LastError != NV_err_none)
			{
				printf("GetStatus while writing data failed with error %d.\n", status.LastError);
				return BL_FAILURE;
			}
			// printf("WriteData length %d.\n", Length);

			Length = ( Length > MaxDataPayloadSize ) ? Length - MaxDataPayloadSize : 0;
		}
	}

	// Flush.
	bl.Flush();
	usleep(10000);
	if (bl.GetStatus( status ) != BL_SUCCESS || status.LastError != NV_err_none)
	{
		printf("GetStatus after flushing failed with error %d.\n", status.LastError);
		return BL_FAILURE;
	}
	printf("Flush successful.\n");

	// Validate.
	bl.Validate(EntireImage);
	usleep(10000);
	if (bl.GetStatus( status ) != BL_SUCCESS || status.LastError != NV_err_none)
	{
		printf("GetStatus after image validation failed with error %d.\n", status.LastError);
		return BL_FAILURE;
	}
	printf("Validation successful.\n");

	// Reset.
	retval = bl.Reset();
	printf("Reset returned %d.\n", retval);
	if( retval != BL_SUCCESS ) return retval;
	usleep(10000);

	// Check status.
	if (bl.GetStatus( status ) != BL_SUCCESS || status.LastError != NV_err_none)
	{
		printf("GetStatus after reset failed with error %d.\n", status.LastError);
		return BL_FAILURE;
	}
	printf("Firmware update successful.\n");

	return 0;
}

int main (int argc, char * argv[])
{
	string device, fw_file;
	int ret = 0;

	if( argc > 1 && strcmp( argv[1], "-r" ) == 0 )
	{
		vector<string> devices;

		if (argc < 3)
		{
			devices = find_cirque_devices();
		}
		else
		{
			devices.push_back(string(argv[2]));
		}
		
		for(int i = 0; i < devices.size(); ++i)
		{
			printf("Querying device %s\n", devices[i].c_str());
			dump_raw_data(devices[i]);
		}

		return 0;
	}

	switch( argc )
	{
		case 3:
			struct stat mode;
			mode.st_mode = 020660;
			stat( argv[2], &mode );
			chmod( argv[2], mode.st_mode | S_IROTH | S_IWOTH );
			device = argv[2];
			if( strcmp( argv[1], "-a" ) == 0 )
			{
				// Get and output the version number.
				ret = get_fw_version(device);
			}
			else if( strcmp( argv[1], "-n" ) == 0 )
			{
				// Get the version number.
				printf("Querying device %s\n", device.c_str());
				ret = get_device_attributes(device);
			}
			else
			{
				// Update firmware.
				fw_file = argv[1];
				printf("Updating device %s with firmware from %s\n", device.c_str(), fw_file.c_str());
				ret = update_firmware(device, fw_file);
			}
			chmod( argv[2], mode.st_mode );
			return ret;
		default:
			printf("Bad syntax.\n");
			return -1;
	}
	
	return 0;
}
