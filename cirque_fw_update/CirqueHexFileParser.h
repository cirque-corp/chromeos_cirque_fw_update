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

#ifndef __CIRQUE_HEX_FILE_PARSER_H__
#define __CIRQUE_HEX_FILE_PARSER_H__

#include "CirqueHexFileRecord.h"
#include <string>
#include <vector>

using namespace std;

#define HEX_SUCCESS  ( 0  )
#define HEX_NOFILE   (-101)
#define HEX_CORRUPT  (-102)

class CirqueHexFileParser
{
private:
	string filename;
	bool done = false;
	bool bin = false;
	uint32_t startSegmentAddress;
	uint32_t startLinearAddress;

public:
	CirqueHexFileRecord* rec;
	vector<CirqueHexFileRecord*> recList;

	CirqueHexFileParser( string& Filename );
	~CirqueHexFileParser();
	int Parse();
	int WriteBin(string& Filename);
	int ReadBin();
	uint32_t Fletcher_32(uint16_t *dataPtr, size_t bytes);
};

#endif //__CIRQUE_HEX_FILE_PARSER_H__
