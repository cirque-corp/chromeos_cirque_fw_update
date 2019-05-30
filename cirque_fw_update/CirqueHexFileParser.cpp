// Copyright (c) 2019 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include <fstream>
#include "CirqueHexFileParser.h"

CirqueHexFileParser::CirqueHexFileParser( string& Filename )
{
	filename = Filename;
}

CirqueHexFileParser::~CirqueHexFileParser()
{
	for( unsigned i = 0; i < recList.size(); ++i )
		delete recList[i];
}

int CirqueHexFileParser::Parse()
{
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
