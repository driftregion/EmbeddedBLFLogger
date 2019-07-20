#include "blflogger.h"

int main()
{
	BLFWriter writer("foo.blf");

	frameobject_t fobj;
	fobj.frame.timestamp = 12312;
	fobj.frame.id = 0x123;
	fobj.frame.length = 3;
	fobj.frame.data.byte[0] = 0x12;
	fobj.frame.data.byte[1] = 0x34;
	fobj.frame.data.byte[2] = 0x56;

	for (int i = 0; i < 10000; i++)
	{
		fobj.bus_number = i;
		fobj.direction = (i % 2 ? FRAME_DIRECTION_RX : FRAME_DIRECTION_TX);
		writer.log(fobj);
	}
	writer.stop();
	return 0;
}

