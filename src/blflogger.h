#ifndef BLFLOGGER_H
#define BLFLOGGER_H

#include "can_common.h"
#include "stdio.h"
#include "unistd.h"

#define APPLICATION_ID 0xf00

typedef enum {
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
#define TIME_TEN_MICS 0x00000001
#define TIME_ONE_NANS 0x00000002
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
#define CAN_MSG_FLAG_TX 1
#define CAN_MSG_FLAG_NERR 6
#define CAN_MSG_FLAG_WU 7
#define CAN_MSG_FLAG_RTR 8
    uint8_t flags;
    uint8_t dlc;
    uint32_t arbitration_id;
    uint8_t data[CAN_FRAME_LENGTH_BYTES];
} can_msg_t;

class BLFWriter {
  public:
    BLFWriter(const char *filepath);
    const char *filepath;
    void log(frameobject_t &frame, uint64_t abs_timestamp_ns);
    void set_start_timestamp_ns(uint64_t abs_timestamp_ns);
    void sync();
    void stop();
    static systemtime_t timestamp_to_systemtime(const uint64_t timestamp);

  protected:
    const static uint64_t FILE_HEADER_SIZE = 144;
    const static uint64_t MAX_CACHE_SIZE = 16 * 1024;
    uint8_t cache[MAX_CACHE_SIZE];
    uint64_t cache_size;
    uint64_t uncompressed_size;
    FILE *file;
    uint64_t start_timestamp;
    uint64_t stop_timestamp;
    uint64_t count_of_objects;

    void _flush();
    void _add_object(blf_objtype_t, void *data, size_t size, uint64_t timestamp);
    void _write_header();
};

#endif //BLFLOGGER_H
