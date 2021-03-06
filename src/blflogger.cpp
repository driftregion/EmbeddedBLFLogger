#include "blflogger.h"
#include <time.h>

BLFWriter::BLFWriter(const char *filepath) : filepath(filepath),
                                             cache_size(0),
                                             uncompressed_size(FILE_HEADER_SIZE),
                                             start_timestamp(0),
                                             stop_timestamp(0),
                                             count_of_objects(0),
                                             file(fopen(filepath, "w+b")) {
    for (int i = 0; i < FILE_HEADER_SIZE; i++) {
        fwrite("\0", 1, 1, file);
    }
}

void BLFWriter::log(frameobject_t &fobj, uint64_t abs_timestamp_ns) {
    can_msg_t msg;

    msg.arbitration_id = fobj.frame.id;
    msg.dlc = fobj.frame.length;
    msg.flags = 0;
    if (fobj.direction == FRAME_DIRECTION_TX)
        msg.flags |= CAN_MSG_FLAG_TX;
    msg.channel = (uint16_t)fobj.bus_number; 
    for (uint8_t i = 0; i < fobj.frame.length; i++) {
        msg.data[i] = fobj.frame.data.bytes[i];
    }

    stop_timestamp = abs_timestamp_ns;

    _add_object(CAN_MESSAGE, &msg, sizeof(can_msg_t), abs_timestamp_ns);
}

/*
Start timestamp as nanoseconds since epoch
*/
void BLFWriter::set_start_timestamp_ns(uint64_t timestamp_ns) {
    start_timestamp = timestamp_ns;
}

/*
Takes absolute timestamp in nanoseconds
*/
void BLFWriter::_add_object(blf_objtype_t type, void *data, size_t size, uint64_t timestamp_ns) {
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
        .flags = TIME_ONE_NANS,
        .client_index = 0,
        .object_version = 0,
        .timestamp =  timestamp_ns,
    };

    char buf[obj_size];
    uint64_t offset = 0;
    memcpy(&buf[offset], &base_header, sizeof(base_header));
    offset += sizeof(base_header);
    memcpy(&buf[offset], &obj_header, sizeof(obj_header));
    offset += sizeof(obj_header);
    memcpy(&buf[offset], data, size);
    offset += size;

    auto padding_size = offset % 4;
    if (padding_size) {
        memset(&buf[offset], 0, padding_size);
        offset += padding_size;
    }

    if (cache_size + offset > MAX_CACHE_SIZE)
        _flush();

    memcpy(&cache[cache_size], buf, offset);
    cache_size += offset;
    count_of_objects++;
}

void BLFWriter::_flush() {
    if (file == NULL)
        return;

    auto obj_size = sizeof(obj_header_v1_t) + sizeof(log_container_t) + cache_size;

    obj_header_base_t base_header = {
        .signature = {'L', 'O', 'B', 'J'},
        .header_size = sizeof(obj_header_base_t),
        .header_version = 1,
        .object_size = obj_size,
        .object_type = LOG_CONTAINER,
    };

    log_container_t container = {
        .compression_method = NO_COMPRESSION,
        ._pad0 = {0},
        .size_uncompressed = cache_size,
        ._pad1 = {0},
    };

    auto padding_size = (sizeof(obj_header_base_t) + obj_size) % 4;

    fwrite(&base_header, sizeof(obj_header_base_t), 1, file);
    fwrite(&container, sizeof(log_container_t), 1, file);
    fwrite(cache, cache_size, 1, file);
    if (padding_size) {
        fwrite("\x0", 1, padding_size, file);
    }

    uncompressed_size += cache_size;
    cache_size = 0;
    memset(cache, 0, MAX_CACHE_SIZE);
}

systemtime_t BLFWriter::timestamp_to_systemtime(const uint64_t timestamp_ns) {
    systemtime_t systemtime;

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
    auto file_size = ftell(file);

    file_header_t header = {
        .signature = {'L', 'O', 'G', 'G'},
        .header_size = FILE_HEADER_SIZE,
        .application_id = 0,
        .application_major = 0,
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

void BLFWriter::sync() {
    _flush();
    _write_header();
    ::fsync(fileno(file));
}

void BLFWriter::stop() {
    sync();
    fclose(file);
}