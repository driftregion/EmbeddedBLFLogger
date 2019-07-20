#include "can_common.h"
#include "stdio.h"
#include "unistd.h"
#include "zlib.h"

#define APPLICATION_ID 0xf00

typedef enum 
{
	CAN_MESSAGE = 1,
	CAN_ERROR = 2,
	LOG_CONTAINER = 10,
	CAN_ERROR_EXT = 73,
	CAN_MESSAGE2 = 86,
	GLOBAL_MARKER = 96,
	CAN_FD_MESSAGE = 100,
	CAN_FD_MESSAGE_64 = 101,
} blf_objtype_t;

typedef struct
{
	char signature[4];
	uint16_t header_size;
	uint16_t header_version;
	uint32_t object_size;
	uint32_t object_type;
} obj_header_base_t;

typedef struct 
{
	uint32_t flags;
	uint16_t client_index;
	uint16_t object_version;
	uint64_t timestamp;
} obj_header_v1_t;

#define NO_COMPRESSION 0
#define ZLIB_DEFLATE 2

typedef struct
{
	uint16_t compression_method;
	uint8_t _pad0[6];
	uint32_t size_uncompressed;
	uint8_t _pad1[4];
} log_container_t;

/* Vector's spin on `struct tm` */
typedef struct {
	uint16_t year;
	uint16_t month;
	uint16_t isoweekday;
	uint16_t day;
	uint16_t hour;
	uint16_t minute;
	uint16_t second;
	uint16_t millisecond;
} systemtime_t;

typedef struct {
	char signature[4];
	uint32_t header_size;
	uint8_t application_id;
	uint8_t application_major;
	uint8_t application_minor;
	uint8_t application_build;
	uint8_t bin_log_major;
	uint8_t bin_log_minor;
	uint8_t bin_log_build;
	uint8_t bin_log_patch;
	uint64_t file_size;
	uint64_t uncompressed_size;
	uint32_t count_of_objects;
	uint32_t count_of_objects_read;
	systemtime_t time_start;
	systemtime_t time_stop;
} file_header_t;

const int CAN_FRAME_LENGTH_BYTES = 8;

typedef struct {
	uint16_t channel;
	uint8_t flags;
	uint8_t dlc;
	uint32_t arbitration_id;
	uint8_t data[CAN_FRAME_LENGTH_BYTES];
} blf_can_msg_t;

class BLFWriter
{
	public:
		BLFWriter(const char *filepath);
		const char *filepath;
		void log(frameobject_t &frame);
		void set_start_timestamp(uint64_t timestamp);
		void stop();
		static systemtime_t timestamp_to_systemtime(const uint64_t timestamp);

	protected:
		const static uint64_t FILE_HEADER_SIZE = 144;
		const static uint64_t MAX_CACHE_SIZE = 128 * 1024;
		uint8_t cache[MAX_CACHE_SIZE];
		uint64_t cache_size;
		uint64_t uncompressed_size;
		FILE *file;
		uint64_t start_timestamp;
		uint64_t stop_timestamp;
		uint64_t count_of_objects;

		void _add_object(blf_objtype_t, void *data, size_t size, uint64_t timestamp);
		void _flush();
		void _write_header();
};

BLFWriter::BLFWriter(const char *filepath) :
	filepath(filepath),
	cache_size(0),
	uncompressed_size(FILE_HEADER_SIZE),
	start_timestamp(0),
	stop_timestamp(0),
	count_of_objects(0),
	file(fopen(filepath, "w+b"))
{
	for (int i = 0; i < FILE_HEADER_SIZE; i++)
	{
		fwrite("\0", 1, 1, file);
	}
}

void BLFWriter::log(frameobject_t &fobj)
{
	static blf_can_msg_t msg;

	msg.arbitration_id = fobj.frame.id;
	msg.dlc = fobj.frame.length;
	msg.flags = 0;
	msg.channel = fobj.bus_number;
	for (uint8_t i = 0; i < fobj.frame.length; i++)
	{
		msg.data[i] = fobj.frame.data.bytes[i];
	}

	_add_object(CAN_MESSAGE, &msg, sizeof(blf_can_msg_t), fobj.frame.timestamp);
}

void BLFWriter::set_start_timestamp(uint64_t timestamp)
{
	start_timestamp = timestamp;
}

void BLFWriter::_add_object(blf_objtype_t type, void *data, size_t size, uint64_t timestamp)
{
	auto header_size = sizeof(obj_header_base_t) + sizeof(obj_header_v1_t);
	auto obj_size = header_size + size;

	obj_header_base_t base_header = {
		.signature = {'L', 'O', 'B', 'J'},
		.header_size = header_size,
		.header_version = 1,
		.object_size = obj_size,
		.object_type = type,
	};

	obj_header_v1_t obj_header = {
		.flags = 0x02,
		.client_index = 0,
		.object_version = 0,
		.timestamp = timestamp,
	};

	char buf[obj_size];
	auto offset = 0;
	memcpy(&buf[offset], &base_header, sizeof(base_header));
	offset += sizeof(base_header);
	memcpy(&buf[offset], &obj_header, sizeof(obj_header));
	offset += sizeof(obj_header);
	memcpy(&buf[offset], data, size);
	offset += size;

	auto padding_size = offset % 4;
	if (padding_size)
	{
		memset(&buf[offset], 0, padding_size);
		offset += padding_size;
	}

	if (cache_size + offset > MAX_CACHE_SIZE)
		_flush();
	
	memcpy(&cache[cache_size], buf, offset);
	cache_size += offset;
	count_of_objects++;
}


void BLFWriter::_flush()
{
	printf("FLUSHD\n");
	if (file == NULL)
		return;
	Bytef data[MAX_CACHE_SIZE];

	uLongf destLen = MAX_CACHE_SIZE;
	compress(data, &destLen, cache, cache_size);

	auto obj_size = sizeof(obj_header_v1_t) + sizeof(log_container_t) + destLen;

	obj_header_base_t base_header = {
		.signature = {'L', 'O', 'B', 'J'},
		.header_size = sizeof(obj_header_base_t),
		.header_version = 1,
		.object_size = obj_size,
		.object_type = LOG_CONTAINER,
	};

	log_container_t container = {
		.compression_method = ZLIB_DEFLATE,
		._pad0 = {0},
		.size_uncompressed = cache_size,
		._pad1 = {0},
	};

	auto padding_size = (sizeof(obj_header_base_t) + obj_size) % 4;

	fwrite(&base_header, sizeof(obj_header_base_t), 1, file);
	fwrite(&container, sizeof(log_container_t), 1, file);
	fwrite(data, destLen, 1, file);
	if (padding_size)
	{
		fwrite("\x0", 1, padding_size, file);
	}

	uncompressed_size += cache_size;
	cache_size = 0;
	memset(cache, 0, MAX_CACHE_SIZE);
}


systemtime_t BLFWriter::timestamp_to_systemtime(const uint64_t timestamp)
{
	systemtime_t systemtime;
	systemtime.day = 2;
	return systemtime;
}


void BLFWriter::_write_header()
{
	auto file_size = ftell(file);

	file_header_t header = {
		.signature = {'L', 'O', 'G', 'G'},
		.header_size = FILE_HEADER_SIZE,
		.application_id = 0,
		.application_minor = 0,
		.application_build = 0,
		.bin_log_major = 0,
		.bin_log_minor = 0,
		.bin_log_build = 0,
		.bin_log_patch = 0,
		.file_size = file_size,
		.uncompressed_size = uncompressed_size,
		.count_of_objects = count_of_objects,
		.count_of_objects_read = 0,
		.time_start = timestamp_to_systemtime(start_timestamp),
		.time_stop = timestamp_to_systemtime(stop_timestamp),
	};
	fseek(file, 0, SEEK_SET);

	fwrite(&header, sizeof(file_header_t), 1, file);

	fseek(file, 0, SEEK_END);
}

void BLFWriter::stop()
{
	_write_header();
}