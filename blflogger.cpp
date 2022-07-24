#include "blflogger.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <algorithm>

#include "miniz/miniz.h"

BLFWriter::BLFWriter(const char *filepath, int8_t compression_level) : 
                                            //  cache_size(0),
                                             _uncompressed_size(FILE_HEADER_SIZE),
                                            //  start_timestamp(0),
                                            //  stop_timestamp(0),
                                             _count_of_objects(0),
                                             _fd(fopen(filepath, "w+b")),
                                             _start_timestamp(0),
                                             _stop_timestamp(0),
                                             _compression_level(compression_level),
                                             _pCmpSize(_compression_level ? compressBound(sizeof(_buffer)) : 0),
                                             _pCmp(_compression_level ? (unsigned char *)malloc(_pCmpSize) : NULL) {
    for (auto i = 0; i < FILE_HEADER_SIZE; i++) {
        fwrite("\0", 1, 1, _fd);
    }
}

BLFWriter::BLFWriter(const char *filepath) : BLFWriter(filepath, -1) {}

BLFWriter::~BLFWriter() {
    _flush();
    _write_header();
    free(_pCmp);
    fclose(_fd);
}


void BLFWriter::on_message_received(uint64_t timestamp_ns, uint32_t arbitration_id, uint8_t *data, uint8_t dlc) {
    on_message_received(timestamp_ns, arbitration_id, data, dlc, 1, false, false, false, false, true, false, false);
}

void BLFWriter::on_message_received(uint64_t timestamp_ns, uint32_t arbitration_id, uint8_t *data, uint8_t dlc, uint16_t channel, bool is_extended_id, bool is_remote_frame, bool is_error_frame, bool is_fd, bool is_rx, bool bitrate_switch, bool error_state_indicator) {
    if (is_error_frame) {
        can_error_ext_t msg;
        memset(&msg, 0, sizeof(msg));
        printf("error frame: 0x%x\n", arbitration_id);
        msg.channel = channel;
        msg.dlc = dlc;
        msg._reserved0 = 0xFF;
        msg.frame_length = 1;
        msg.arbitration_id = arbitration_id;
        memmove(msg.data, data, std::min(dlc, (uint8_t)sizeof(msg.data)));
        _add_object(CAN_ERROR_EXT, &msg, sizeof(msg), timestamp_ns);
    } else if (is_fd) {
        can_fd_msg_t msg;
        memset(&msg, 0, sizeof(msg));
        assert(dlc <= sizeof(msg.data));
        if (!is_rx) msg.flags |= CAN_MSG_FLAG_TX;
        if (is_remote_frame) msg.flags |= CAN_MSG_FLAG_RTR; 
        msg.arbitration_id = arbitration_id;
        msg.channel = channel;
        msg.dlc = dlc;
        memmove(msg.data, data, dlc);
        _add_object(CAN_FD_MESSAGE, &msg, sizeof(msg), timestamp_ns);
    } else {
        can_msg_t msg;
        memset(&msg, 0, sizeof(msg));
        assert(dlc <= sizeof(msg.data));
        msg.arbitration_id = arbitration_id;
        msg.dlc = dlc;
        msg.flags = 0;
        if (!is_rx) msg.flags |= CAN_MSG_FLAG_TX;
        if (is_remote_frame) msg.flags |= CAN_MSG_FLAG_RTR;
        msg.channel = channel; 
        memmove(msg.data, data, dlc);
        _add_object(CAN_MESSAGE, &msg, sizeof(can_msg_t), timestamp_ns);
    }
}

/*
Takes absolute timestamp in nanoseconds
*/
void BLFWriter::_add_object(blf_objtype_t type, void *data, size_t size, uint64_t timestamp_ns) {
    constexpr uint16_t header_size = sizeof(obj_header_base_t) + sizeof(obj_header_v1_t);
    uint32_t obj_size = header_size + size;
    auto offset = 0;

    if (0 == _start_timestamp) {
        _start_timestamp = timestamp_ns;
    }
    _stop_timestamp = timestamp_ns;
    uint64_t timedelta = timestamp_ns - _start_timestamp;

    obj_header_base_t base_header = {
        .signature = {'L', 'O', 'B', 'J'},
        .header_size = header_size,
        .header_version = 1,
        .object_size = obj_size,
        .object_type = type,
    };

    obj_header_v1_t obj_header = {
        .flags = TIME_ONE_NANS,
        .client_index = 0,
        .object_version = 0,
        .timestamp =  timedelta,
    };

    _buffer_append(&base_header, sizeof(base_header));
    _buffer_append(&obj_header, sizeof(obj_header));
    _buffer_append(data, size);
    auto padding_size = (sizeof(base_header) + sizeof(obj_header) + size) % 4;
    while (padding_size--) {
        _buffer_append("\0", 1);
    }
    _count_of_objects++;
    printf("%d, %d,  %d, %d\n", _count_of_objects, size, padding_size, offset);
}

void BLFWriter::_buffer_append(const void *data, size_t size) {
    assert(size < sizeof(_buffer));

    if (size > sizeof(_buffer) - _buffer_size) {
        _flush();
    }
    memmove(_buffer + _buffer_size, data, size);
    _buffer_size += size;
}

/**
 * compresses and writes data in the buffer to file
 */
void BLFWriter::_flush() {
    if (NULL == _fd || 0 == _buffer_size) {
        return;
    }

    uint16_t compression_method = _compression_level ? ZLIB_DEFLATE : NO_COMPRESSION;

    unsigned char *data = NULL;
    unsigned long data_size = 0;

    if (compression_method) {
        data_size = _pCmpSize;
        printf("pCmpSize: %d\n", _pCmpSize);
        auto cmp_status = compress(_pCmp, &data_size, (const unsigned char *)_buffer, _buffer_size);
        if (cmp_status  != Z_OK) {
            printf("%p, %d\n", _pCmp, _pCmpSize);
            fprintf(stderr, "compress failed\n");
        }
        data = _pCmp;
    } else {
        data = _buffer;
        data_size = _buffer_size;
    }

    assert(data);
    auto obj_size =  sizeof(obj_header_base_t) + sizeof(log_container_t) + data_size;

    obj_header_base_t base_header = {
        .signature = {'L', 'O', 'B', 'J'},
        .header_size = sizeof(obj_header_base_t),
        .header_version = 1,
        .object_size = obj_size,
        .object_type = LOG_CONTAINER,
    };

    log_container_t container = {
        .compression_method = compression_method,
        ._pad0 = {0},
        .size_uncompressed = _buffer_size,
        ._pad1 = {0},
    };

    fwrite(&base_header, sizeof(obj_header_base_t), 1, _fd);
    fwrite(&container, sizeof(log_container_t), 1, _fd);
    fwrite(data, data_size, 1, _fd);
    // write padding bytes
    auto padding_size =  obj_size % 4;
    printf("writing %d padding bytes\n", padding_size);
    while (padding_size--) {
        printf("%d\n", ftell(_fd));
        fwrite("\00", 1, 1, _fd);
    }
    printf("%d\n", ftell(_fd));

    _uncompressed_size += sizeof(obj_header_base_t);
    _uncompressed_size += sizeof(log_container_t);
    _uncompressed_size += _buffer_size;
    _buffer_size = 0;
    memset(_buffer, 0, MAX_CONTAINER_SIZE);
}

systemtime_t BLFWriter::timestamp_to_systemtime(uint64_t timestamp_ns) {
    systemtime_t systemtime;
    memset(&systemtime, 0, sizeof(systemtime));
    if (timestamp_ns < 631152000) {
        // Probably not a Unix timestamp
        return systemtime;
    }

    time_t timestamp_s = (time_t)(timestamp_ns / (1000 * 1000 * 1000));
    struct tm *tm;
    tm = gmtime(&timestamp_s);

    systemtime.year = tm->tm_year + 1900;
    systemtime.month = tm->tm_mon + 1;
    systemtime.isoweekday = tm->tm_wday;
    systemtime.day = tm->tm_mday;
    systemtime.hour = tm->tm_hour;
    systemtime.minute = tm->tm_min;
    systemtime.second = tm->tm_sec;
    systemtime.millisecond = (timestamp_ns / (1000 * 1000)) % 1000;

    return systemtime;
}

void BLFWriter::_write_header() {
    uint64_t file_size = ftell(_fd);

    file_header_t header = {
        .signature = {'L', 'O', 'G', 'G'},
        .header_size = FILE_HEADER_SIZE,
        .application_id = 5,
        .application_major = 0,
        .application_minor = 0,
        .application_build = 0,
        .bin_log_major = 2,
        .bin_log_minor = 5,
        .bin_log_build = 8,
        .bin_log_patch = 1,
        .file_size = file_size,
        .uncompressed_size = _uncompressed_size,
        .count_of_objects = _count_of_objects,
        .count_of_objects_read = 0,
        .time_start = timestamp_to_systemtime(_start_timestamp),
        .time_stop = timestamp_to_systemtime(_stop_timestamp),
    };

    fseek(_fd, 0, SEEK_SET);

    fwrite(&header, sizeof(file_header_t), 1, _fd);

    fseek(_fd, 0, SEEK_END);
}

// void BLFWriter::sync() {
//     _flush();
//     _write_header();
//     ::fsync(fileno(_fd));
// }

// void BLFWriter::stop() {
//     sync();
//     fclose(_fd);
// }