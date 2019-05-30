// Copyright (c) 2019 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

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
	uint32_t startSegmentAddress;
	uint32_t startLinearAddress;

public:
	CirqueHexFileRecord* rec;
	vector<CirqueHexFileRecord*> recList;

	CirqueHexFileParser( string& Filename );
	~CirqueHexFileParser();
	int Parse();
};

#endif //__CIRQUE_HEX_FILE_PARSER_H__
