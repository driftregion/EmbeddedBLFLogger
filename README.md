# EmbeddedBLFLogger

Write uncompressed Vector BLF CAN log files on embedded devices.

## Status: Experimental

## Usage
```cpp
#include "can_common.h"
#include "blflogger.h"

BLFWriter writer('name.blf');

int main()
{
  frameobject_t fobj;
  fobj.frame.id = 0x123;
  fobj.bus_number = 5;

  writer.log(&fobj);
  
  writer.stop();
}

```

## Credit
Most of this is transcribed verbatim from the [python-can](https://python-can.readthedocs.io/) [BLF module](https://python-can.readthedocs.io/en/3.1.1/_modules/can/io/blf.html).  That module credits TobyLorenz' comprehensive [vector_blf](https://bitbucket.org/tobylorenz/vector_blf/).

### Why not just use `vector_blf` directly?
I made this for a project that uses the Arduino libraries and toolchain on ESP32.  I tried cross-compiling , but found that the version of ESP-IDF pinned by Arduino [doesn't support C++ exceptions](https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/error-handling.html#c-exceptions), a language feature which `vector_blf` makes use of.