// Copyright (c) 2019 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

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
	CirqueHexFileRecord( string& s );
	~CirqueHexFileRecord();

	vector<uint8_t> buf;

private:
	RecordType rtype;
	uint32_t address;
	uint32_t extendedAddress = 0;
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
