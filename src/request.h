#ifndef REQUEST_H
#define REQUEST_H
#include "irrigationcontrol.h"
#include "response.h"
#include <time.h>
#define MAX_HEADER_LENGTH 16383

class Request
{
    UNCOPYABLE(Request);

private:
	char headerBytes[MAX_HEADER_LENGTH+1];
	int headerPos;
	Response* response;

	time_t last_access;
	enum { 
		STATE_READ_HEADER,
		STATE_WRITE,
		STATE_CLOSE
	} state;

public:
	int fd;
	Request *nextRequest;
Request(time_t now)
    :
        headerPos(0),
        response(NULL),
        last_access(now),
        state(STATE_READ_HEADER),
        fd(-1),
        nextRequest(NULL)
	{
	}
	~Request()
	{
		if (response)
			delete response;
	}

	void Read(time_t now);
	void Write(time_t now)
	{
		last_access = now;
		if (response && response->HasData())
			response->Write(fd);
			
	}
	bool WantsClose(time_t now)
	{
		return (now > last_access + KEEPALIVE_TIME)
			|| (response && response->Done());
	}
	bool WantsRead()
	{
		return STATE_READ_HEADER == state;
	}
	bool WantsWrite()
	{
		return response && response->HasData();
	}
};
#endif
