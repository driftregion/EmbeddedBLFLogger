meta:
  id: blf
  file-extension: blf
  endian: le
  imports:
    - /archive/zip
  title: Vector Binary Logging Format
  license: CC0-1.0
seq:
  - id: file_header
    type: file_header
    size: 144
  - id: obj_header_base
    type: obj_header_base
    repeat: eos
    
enums:
  object_type:
    1: can_message
    2: can_error
    10: log_container
    73: can_error_ext
    86: can_message2
    96: global_marker
    100: can_fd_message
    101: can_fd_mes_64
    115: restore_point_container
  
  compression_method:
    0: no_compression
    2: zlib_deflate
    
types:
  systemtime:
    seq:
    - id: year
      type: u2
    - id: month
      type: u2
    - id: isoweekday
      type: u2
    - id: day
      type: u2
    - id: hour
      type: u2
    - id: minute
      type: u2
    - id: second
      type: u2
    - id: millisecond
      type: u2

  file_header:
    seq:
    - id: signature
      contents: "LOGG"
    - id: header_size
      type: u4
    - id: application_id
      type: u1
    - id: application_major
      type: u1
    - id: application_minor
      type: u1
    - id: application_build
      type: u1
    - id: bin_log_major
      type: u1
    - id: bin_log_minor
      type: u1
    - id: bin_log_build
      type: u1
    - id: bin_log_patch
      type: u1
    - id: file_size
      type: u8
    - id: uncompressed_size
      type: u8
    - id: count_of_objects
      type: u4
    - id: count_of_objects_read
      type: u4
    - id: time_start
      type: systemtime
    - id: time_stop
      type: systemtime
  
  obj_header_base:
    seq:
      - id: signature
        contents: "LOBJ"
      - id: header_size
        type: u2
      - id: header_version
        type: u2
      - id: object_size
        type: u4
      - id: object_type
        type: u4
        enum: object_type
        valid:
          any-of:
            - object_type::can_message
            - object_type::log_container
            - object_type::can_error_ext
            - object_type::can_message2
            - object_type::can_fd_message
            - object_type::restore_point_container
      - id: object
        # size: object_size - 16
        type:
          switch-on: object_type
          cases:
            'object_type::can_message': can_message
            'object_type::log_container': log_container
            'object_type::can_error_ext': can_error_ext
            'object_type::can_message2': can_message
            'object_type::can_fd_message': can_fd_message
            'object_type::restore_point_container': restore_point_container
            
  

  obj_header_v1:
    seq:
      - id: flags
        type: u4
      - id: client_index
        type: u2
      - id: object_version
        type: u2
      - id: timestamp
        type: u8

  obj_list:
    seq:
      - id: log
        type: obj_header_base
        repeat: eos
        # repeat: expr
        # repeat-expr: 7

  log_container:
    seq:
      - id: compression
        type: u2
        enum: compression_method
      - id: pad0
        size: 6
      - id: size_uncompressed
        type: u4
      - id: pad1
        size: 4
      - id: uncompressed_logs
        type: obj_list
        size: size_uncompressed
        if: compression == compression_method::no_compression
      - id: zlib_compressed_logs
        process: zlib
        type: obj_list
        size: _parent.object_size - 29
        # size-eos: true
        # type: obj_header_base
        if: compression == compression_method::zlib_deflate
            
  can_message:
    seq:
      - id: obj_header
        type: obj_header_v1
      - id: channel
        type: u2
      - id: flags
        type: u1
      - id: dlc
        type: u1
      - id: arbitration_id
        type: u4
      - id: data
        size: 8
  
  can_fd_message:
    seq:
      - id: obj_header
        type: obj_header_v1
      - id: channel
        type: u2
      - id: flags
        type: u1
      - id: dlc
        type: u1
      - id: arbitration_id
        type: u4
      - id: frame_length
        type: u4
      - id: bit_count
        type: u1
      - id: fd_flags
        type: u1
      - id: valid_data_bytes
        type: u1
      - id: reserved0
        size: 5
      - id: data
        size: 64
      
  can_error_ext:
    seq:
      - id: obj_header
        type: obj_header_v1
      - id: channel
        type: u2
      - id: length
        type: u2
      - id: flags
        type: u4
      - id: ecc
        type: u1
      - id: position
        type: u1
      - id: dlc
        type: u1
      - id: reserved0
        type: u1
      - id: frame_length
        type: u4
      - id: arbitration_id
        type: u4
      - id: flags_ext
        type: u2
      - id: reserved1
        size: 2
      - id: data
        size: 8
    
  restore_point_container:
    seq:
      - id: reserved0
        size: 40