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

#include <deque>
#include "CirqueHexFileRecord.h"

uint32_t CirqueHexFileRecord::extendedAddress = 0;
uint32_t CirqueHexFileRecord::segmentAddress = 0;

CirqueHexFileRecord::CirqueHexFileRecord()
{
}

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
				address += segmentAddress + extendedAddress;
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
                        segmentAddress = Q.front();
                        segmentAddress <<= 8;
                        Q.pop_front();
                        segmentAddress |= Q.front();
                        segmentAddress *= 16;
						break;
					case rt_start_segment_address:
						break;
					case rt_extended_linear_address:
                        extendedAddress = Q.front();
                        extendedAddress <<= 8;
                        Q.pop_front();
                        extendedAddress |= Q.front();
                        extendedAddress <<= 16;
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
