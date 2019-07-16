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

#ifndef __CIRQUE_DEVELOPMENT_DATA_H__
#define __CIRQUE_DEVELOPMENT_DATA_H__

#include <string>
#include <vector>
#include "CirqueBootloaderCollection.h"

using namespace std;

class CirqueDevData
{
	private:
	CirqueBootloaderCollection *bl;
	uint8_t X_COUNT;
	uint8_t Y_COUNT;
	int INVERT_X;
	int INVERT_Y;

	static const int DEV_DATA_COMP = 1 << 16;
	static const int DEV_DATA_PRE_DEMUX = 2 << 16;
	static const int DEV_DATA_PRE_COMP = 3 << 16;
	static const int DEV_DATA_POST_COMP = 4 << 16;

	static const uint16_t MAX_IMAGE_TRANSFER_LENGTH = 256;

	vector<int16_t> ConvertStreamToInt16Array(vector<uint8_t> &data_bytes);
	void CorrectAxisInversion(vector<vector<int16_t>> &array_2d);
	vector<vector<int16_t>> GetImage(uint32_t image_index);

	public:
	CirqueDevData(CirqueBootloaderCollection *cirque_bl);
	~CirqueDevData();

	vector<vector<int16_t>> GetCompensationImage();
	vector<vector<int16_t>> GetRawMeasurementImage();
	vector<vector<int16_t>> GetUncompensatedImage();
	vector<vector<int16_t>> GetCompensatedImage();
	string PrintImageArray(string title, vector<vector<int16_t>> &image);
	void GetVersionInfo(uint32_t &fw_revision, int &is_dirty, int &is_branch);
};

#endif //__CIRQUE_DEVELOPMENT_DATA_H__
