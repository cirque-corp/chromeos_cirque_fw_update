// Copyright (c) 2019 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#ifndef __CIRQUE_BOOTLOADER_COLLECTION_H__
#define __CIRQUE_BOOTLOADER_COLLECTION_H__

#include <string>
#include <vector>
using namespace std;

#define BL_SUCCESS		   ( 0)
#define BL_FAILURE		   (-1)
#define BL_NOT_SUPPORTED   (-2)
#define BL_NOT_IMPLEMENTED (-3)
#define BL_READ_ERROR	   (-5)
#define BL_WRITE_ERROR	   (-6)

enum ErrorCodes : uint8_t
{
	NV_err_none,
	NV_err_not_initialized,
	NV_err_sector_out_of_range,
	NV_err_offset_out_of_range,
	NV_err_null_ptr,
	NV_err_timeout,
	NV_err_unknown,
	NV_err_no_recent_image,
	NV_err_access_violation,
	NV_err_protection_violation,
	NV_err_misaligned_address,
	NV_err_cmd_unknown,
	NV_err_chksum_mismatch
};

enum ImageLayouts : uint8_t
{
	Single,
	Dual
};

enum ActiveImages : uint8_t
{
	None,
	One,
	Two
};

enum ValidationType : uint8_t
{
	Headers,
	EntireImage
};

struct CirqueBootloaderStatus
{
	uint16_t Sentinel;
	uint8_t Version;
	ErrorCodes LastError;
	uint8_t Flags;
	ImageLayouts ImageLayout;
	ActiveImages ActiveImage;
	bool bBusy;
	bool bImageValid;
	bool bForce;
	uint8_t AtomicWriteSize;
	uint8_t ByteWriteDelayUs;		  // The microsecond byte write delay
	uint8_t RegionFormatDelayMsPer1K; // The region format delay in ms/1k bytes
};

class CirqueBootloaderCollection
{
	private:
	static const int BL_REPORT_LENGTH = 531;
	static const int BL_CMD_WRITE = 0;
	static const int BL_CMD_FLUSH = 1;
	static const int BL_CMD_VALIDATE = 2;
	static const int BL_CMD_RESET = 3;
	static const int BL_CMD_FORMAT_IMAGE = 4;
	static const int BL_CMD_FORMAT_REGION = 5;
	static const int BL_CMD_INVOKE_BL = 6;
	static const int BL_CMD_WRITE_MEM = 7;
	static const int BL_CMD_READ_MEM = 8;

	int report_length = 531;
	int hid_report_id;
	int fd;

	int GenFeatureIOCTL(int length, int get_not_set);
	int GenGETFeatureIOCTL(int length);
	int GenSETFeatureIOCTL(int length);

	void AppendU32toBuffer(uint32_t value, vector<uint8_t> &data);
	void AppendU16toBuffer(uint16_t value, vector<uint8_t> &data);

	uint32_t GetU32FromBuffer(uint8_t * buffer);
	uint16_t GetU16FromBuffer(uint8_t * buffer);

	void PadBuffer(vector<uint8_t> &data);
	void ParseReadDataFromStatus(vector<uint8_t> &status_data, uint32_t &addr, uint16_t &length, vector<uint8_t> &return_buffer);
	uint16_t Fletcher_16(uint8_t *dataPtr, size_t bytes);
	uint32_t Fletcher_32(uint8_t *dataPtr, size_t bytes);

	int BootloaderSetFeature(vector<uint8_t> &data);
	int BootloaderGetFeature(vector<uint8_t> &data);

	public:
	CirqueBootloaderCollection(string& device_path, int report_id = 7);
	~CirqueBootloaderCollection();

	int IS_BIG_ENDIAN;

	vector<uint8_t> ExtendedRead(uint32_t addr, uint16_t length);
	void ExtendedRead(uint32_t addr, uint16_t length, vector<uint8_t> &return_buffer);
	void ExtendedWrite(uint32_t addr, vector<uint8_t> &data);

	bool SanityCheck();
	int GetStatus( CirqueBootloaderStatus& Status );
	int Reset( void );
	int Invoke( void );
	int FormatImage( uint8_t NumRegions, uint32_t EntryPointAddress, uint8_t I2CAddress, uint16_t HIDDescriptorAddr );
	int FormatRegion( uint8_t RegionNumber, uint32_t RegionOffset, vector<uint8_t>& data );
	int WriteData( uint32_t WriteOffset, uint32_t NumBytes, vector<uint8_t>& data );
	int Flush( void );
	int Validate( ValidationType Validation );

	int GetVersionInfo(uint16_t &vid, uint16_t &pid, uint16_t &rev);
};

#endif //__CIRQUE_BOOTLOADER_COLLECTION_H__
