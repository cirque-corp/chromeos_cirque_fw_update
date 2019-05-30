// Copyright (c) 2019 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include <deque>
#include "CirqueHexFileRecord.h"

CirqueHexFileRecord::CirqueHexFileRecord( string& s )
{
	uint32_t checksum = 0;

	valid = false;

	if( s[0] == ':' )
	{
		uint8_t u;
		deque<uint8_t> Q;

		// build a stream of bytes
		for( int i = 1; i < s.size() && valid_hex( s[i] ) && valid_hex( s[i + 1] ); i += 2 )
		{
			u = hex_to_nibble( s[i] ) << 4;
			u |= hex_to_nibble( s[i + 1] );
			Q.push_back( u );
		}

		if( !Q.empty() )
		{
			u = 0;

			for( int i = 0; i < Q.size() - 1; i++ )
			{
				checksum += Q[i];
			}
			checksum &= 0xff;
			checksum = ~checksum + 1;
			checksum &= 0xff;

			valid = checksum == Q.back();
			if( valid )
			{
				Q.pop_front();
				address = Q.front();
				Q.pop_front();
				address <<= 8;
				address |= Q.front();
				Q.pop_front();
				address |= extendedAddress;
				rtype = (RecordType)Q.front();
				Q.pop_front();

				switch( rtype )
				{
					case rt_data:
						Q.pop_back();
						buf.assign( Q.begin(), Q.end() );
						break;
					case rt_end_of_file:
						break;
					case rt_extended_segment_address:
						extendedAddress = getAddress();
						break;
					case rt_start_segment_address:
						break;
					case rt_extended_linear_address:
						extendedAddress = getAddress();
						break;
					case rt_start_linear_address:
						break;
				}
			}
		}
	}
}

CirqueHexFileRecord::~CirqueHexFileRecord()
{
}

bool CirqueHexFileRecord::merge( CirqueHexFileRecord* rec )
{
	bool merged = false;
	if( getAddress() + getSize() == rec->getAddress() )
	{
		buf.insert( buf.end(), rec->buf.begin(), rec->buf.end() );
		merged = true;
	}
	return merged;
}
