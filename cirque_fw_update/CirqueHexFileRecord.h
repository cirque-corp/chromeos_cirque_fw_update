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

#ifndef __CIRQUE_HEX_FILE_RECORD_H__
#define __CIRQUE_HEX_FILE_RECORD_H__

#include <string>
#include <vector>
using namespace std;

enum RecordType
{
	rt_data,
	rt_end_of_file,
	rt_extended_segment_address,
	rt_start_segment_address,
	rt_extended_linear_address,
	rt_start_linear_address
};

class CirqueHexFileRecord
{
public:
	CirqueHexFileRecord();
	CirqueHexFileRecord( string& s );
	~CirqueHexFileRecord();

	vector<uint8_t> buf;
	uint32_t address;

private:
	RecordType rtype;
	static uint32_t extendedAddress;
	static uint32_t segmentAddress;
	bool valid = false;

public:
	bool merge( CirqueHexFileRecord* rec );

	bool isValid() { return valid; }
	RecordType getRecordType() { return rtype; }
	uint32_t getAddress() { return address; }
	int getSize() { return buf.size(); }

private:
	bool valid_hex( uint8_t c )
	{
		return ( c >= '0' && c <= '9' || c >= 'a' && c <= 'f' || c >= 'A' && c <= 'F' );
	}
	unsigned hex_to_nibble( uint8_t c )
	{
		c = toupper( c );
		return c <= '9' ? c - '0' : c - 'A' + 10;
	}
};

#endif //__CIRQUE_HEX_FILE_RECORD_H__
