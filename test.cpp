#include "blflogger.h"

int main()
{
	BLFWriter writer("foo.blf");

	frameobject_t fobj;
	fobj.frame.timestamp = 12312;
	fobj.frame.id = 0x123;
	fobj.frame.length = 3;
	fobj.bus_number = 22;
	for (int i = 0; i < 10000; i++)
	{
		writer.log(fobj);
	}
	writer.stop();
	return 0;
}

