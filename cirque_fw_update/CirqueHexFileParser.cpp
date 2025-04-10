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

#include <fstream>
#include "CirqueHexFileParser.h"

CirqueHexFileParser::CirqueHexFileParser( string& Filename )
{
	filename = Filename;
	// Check the file signature.
	char buffer[7] = { 0 };
	ifstream file(filename, ios::binary | ios::in);
	if (file.is_open() && file.read(buffer, 6) && string(buffer) == "Cirque")
	{
		// We have a preparsed binary file. Set the flag to treat it accordingly.
		bin = true;
		file.close();
	}
}

CirqueHexFileParser::~CirqueHexFileParser()
{
	for( unsigned i = 0; i < recList.size(); ++i )
		delete recList[i];
}

int CirqueHexFileParser::Parse()
{
	// If the firmware file is preparsed, skip parsing and read the records.
	if (bin) return ReadBin();

	string str;
	ifstream file( filename );
	printf("Parsing %s (stream %d)\n", filename.c_str(), file.is_open());
	if( !file.is_open() ) return HEX_NOFILE;
	while( getline( file, str ) )
	{
		if( done )
			break;

		rec = new CirqueHexFileRecord( str );

		if( rec->isValid() )
		{
			switch( rec->getRecordType() )
			{
				case rt_data:
					if( !recList.empty() && recList.back()->merge( rec ) )
						delete rec;
					else
						recList.push_back( rec );
					break;
				case rt_end_of_file:
					delete rec;
					done = true;
					break;
				case rt_extended_segment_address:
					delete rec;
					break;
				case rt_start_segment_address:
					startSegmentAddress = rec->getAddress();
					delete rec;
					break;
				case rt_extended_linear_address:
					delete rec;
					break;
				case rt_start_linear_address:
					startLinearAddress = rec->getAddress();
					delete rec;
					break;
			}
			// Debug
			// printf( "%s\n", str.c_str() );
		}
		else
		{
			delete rec;
			return HEX_CORRUPT;
		}
	}
	return HEX_SUCCESS;
}

int CirqueHexFileParser::WriteBin(string& Filename)
{
	ofstream file(Filename, ios::binary | ios::out);
	if (file.is_open() && recList.size() > 0)
	{
		// Write the header.
		file.write("Cirque\0\0", 8);
		// Write record count.
		uint32_t temp = recList.size();
		file.write((char*)&temp, 4);
		// Write records one by one.
		for (int i = 0; i < recList.size(); i++)
		{
			// Write the address for the record.
			temp = recList[i]->getAddress();
			file.write((char*)&temp, 4);
			// Write the length of the record.
			temp = recList[i]->getSize();
			file.write((char*)&temp, 4);
			// Write the data.
			file.write((char*)recList[i]->buf.data(), recList[i]->buf.size());
			// Write checksum.
			temp = Fletcher_32((uint16_t*)recList[i]->buf.data(), recList[i]->buf.size());
			file.write((char*)&temp, 4);
		}
		file.close();
	}
	return HEX_SUCCESS;
}

int CirqueHexFileParser::ReadBin()
{
	ifstream file(filename, ios::binary | ios::in);
	if (!file.is_open()) return HEX_NOFILE;
	// Check the file signature.
	char buffer[7] = { 0 };
	if (file.read(buffer, 6) && string(buffer) == "Cirque")
	{
		// Get the binary format version.
		uint16_t version = 0;
		if (file.read((char*)&version, 2))
		{
			if (version == 0)
			{
				// Read record count.
				uint32_t count = 0;
				file.read((char*)&count, 4);
				for (int i = 0; i < count; i++)
				{
					// Create a record object.
					rec = new CirqueHexFileRecord();
					// Set record address.
					file.read((char*)&rec->address, 4);
					// Read record length.
					uint32_t temp = 0;
					file.read((char*)&temp, 4);
					// Read data.
					uint8_t* buffer = new uint8_t[temp];
					file.read((char*)buffer, temp);
					rec->buf.assign(buffer, buffer + temp);
					delete[] buffer;
					// Read checksum.
					file.read((char*)&temp, 4);
					if (temp == Fletcher_32((uint16_t*)rec->buf.data(), rec->buf.size()))
					{
						recList.push_back(rec);
					}
					else
					{
						delete rec;
						return HEX_CORRUPT;
					}
				}
			}
		}
		file.close();
	}

	return HEX_SUCCESS;
}

uint32_t CirqueHexFileParser::Fletcher_32(uint16_t *dataPtr, size_t bytes)
{
	uint32_t sum1 = 0xffff;
	uint32_t sum2 = 0xffff;
	uint16_t data = 0;

	while (bytes)
	{
		uint16_t tlen = bytes > 360 ? 360 : bytes;
		bytes -= tlen;
		do
		{
			data = *dataPtr++;
			sum2 += sum1 += data;
		} while (tlen -= sizeof(uint16_t));
		sum1 = (sum1 & 0xffff) + (sum1 >> 16);
		sum2 = (sum2 & 0xffff) + (sum2 >> 16);
	}

	// Second reduction step to reduce sums to 16 bits
	sum1 = (sum1 & 0xffff) + (sum1 >> 16);
	sum2 = (sum2 & 0xffff) + (sum2 >> 16);
	return sum2 << 16 | sum1;
}

