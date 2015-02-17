#include <unistd.h>

#include "irrigationcontrol.h"

SelectTarget::SelectTarget(size_t maxBytes, int fdin)
    :
    fd(fdin), totalBytes(maxBytes),
    bytesWanted(maxBytes), bytesGotten(0),
    bytes(new unsigned char [totalBytes])
{
}

SelectTarget::~SelectTarget()
{
	delete[] bytes;
}


ssize_t SelectTarget::Read(int rfd)
{
	ssize_t s;
	
	if (rfd < 0)
		rfd = fd;
	else if (fd < 0)
		fd = rfd;
	
	s = read(rfd, bytes + bytesGotten, bytesWanted - bytesGotten);
	bytesGotten += s;
	if (bytesGotten == bytesWanted)
	{
		Execute();
		bytesGotten = 0;
	}
	return s;
}




