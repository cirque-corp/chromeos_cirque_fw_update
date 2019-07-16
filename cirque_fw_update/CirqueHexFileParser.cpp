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
