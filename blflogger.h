#ifndef BLFLOGGER_H
#define BLFLOGGER_H

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

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
} __attribute__((packed)) obj_header_v1_t;

#define NO_COMPRESSION 0
#define ZLIB_DEFLATE 2

constexpr uint32_t CAN_MSG_EXT = 0x80000000;

typedef struct
{
    uint16_t compression_method;
    uint8_t _pad0[6];
    uint32_t size_uncompressed;
    uint8_t _pad1[4];
} __attribute__((packed)) log_container_t;

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
} __attribute__((packed)) systemtime_t;

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
} __attribute__((packed)) file_header_t;

typedef struct {
    uint16_t channel;
#define CAN_MSG_FLAG_TX 1
#define CAN_MSG_FLAG_NERR 6
#define CAN_MSG_FLAG_WU 7
#define CAN_MSG_FLAG_RTR 8
    uint8_t flags;
    uint8_t dlc;
    uint32_t arbitration_id;
    uint8_t data[8];
} __attribute__((packed)) can_msg_t;

typedef struct {
    uint16_t channel;
    uint8_t flags;
    uint8_t dlc;
    uint32_t arbitration_id;
    uint32_t frame_length;
    uint8_t bit_count;
    uint8_t fd_flags;
    uint8_t valid_data_bytes;
    uint8_t _reserved[5];
    uint8_t data[64];
} __attribute__((packed)) can_fd_msg_t;

typedef struct {
    uint16_t channel;
    uint16_t length;
    uint32_t flags;
    uint8_t ecc;
    uint8_t position;
    uint8_t dlc;
    uint8_t _reserved0;
    uint32_t frame_length;
    uint32_t arbitration_id;
    uint16_t flags_ext;
    uint8_t _reserved1[2];
    uint8_t data[8];
} __attribute__((packed)) can_error_ext_t;
    
enum frame_direction_e {
    FRAME_DIRECTION_RX = 0,
    FRAME_DIRECTION_TX
};

typedef struct {
    can_msg_t frame;
    int bus_number;
    frame_direction_e direction;
} frameobject_t;

// Max log container size of uncompressed data
constexpr auto MAX_CONTAINER_SIZE = 16 * 1024;
constexpr auto FILE_HEADER_SIZE = 144;

class BLFWriter {
  public:
    BLFWriter(const char *filepath);
    BLFWriter(const char *filepath, int8_t compression_level);
    ~BLFWriter();
    void on_message_received(uint64_t timestamp_ns, uint32_t arbitration_id, uint8_t *data, uint8_t dlc, uint16_t channel, bool is_extended_id, bool is_remote_frame, bool is_error_frame, bool is_fd, bool is_rx, bool bitrate_switch, bool error_state_indicator);
    void on_message_received(uint64_t timestamp_ns, uint32_t arbitration_id, uint8_t *data, uint8_t dlc);

  protected:
    size_t _uncompressed_size;
    uint32_t _count_of_objects;
    FILE *_fd;
    uint32_t _buffer_size;
    uint8_t _buffer[MAX_CONTAINER_SIZE];
    uint64_t _start_timestamp, _stop_timestamp;
    int8_t _compression_level;
    const size_t _pCmpSize;
    unsigned char *_pCmp; 

    systemtime_t timestamp_to_systemtime(uint64_t timestamp_ns);
    void _add_object(blf_objtype_t, void *data, size_t size, uint64_t timestamp);
    void _write_header();
    void _flush();
    void _buffer_append(const void *data, size_t size);
};

#endif //BLFLOGGER_H
