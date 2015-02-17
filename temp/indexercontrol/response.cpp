#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "indexercontrol.h"
#include "response.h"

const char *responseStart = "HTTP/1.0 %s\r\nContent-Type: %s\nConnection: Close\r\n";


Response::~Response()
{
};
bool Response::Done()
{
	return !HasData();
}

struct ErrorCodes
{
	int code;
	const char *response;
	const char *text;
} errorCodes[] =
{
    { 400, "400 Bad Request",              "*PLONK*\n" },
    { 401, "401 Authentication required",  "Authentication required\n" },
    { 403, "403 Forbidden",                "Access denied\n" },
    { 404, "404 Not Found",                "File or directory not found\n" },
    { 408, "408 Request Timeout",          "Request Timeout\n" },
    { 412, "412 Precondition failed.",     "Precondition failed\n" },
    { 500, "500 Internal Server Error",    "Sorry folks\n" },
    { 501, "501 Not Implemented",          "Sorry folks\n" },
    { 0, NULL, NULL }
};

ResponseError::~ResponseError()
{
	delete[] data;
}
ResponseError::ResponseError(int errorCode)
{
	int i;
	dataPos = 0;

	for (i = 0; errorCodes[i].code; i++)
	{
		if (errorCodes[i].code == errorCode)
			break;
	}
	if (errorCodes[i].code)
	{
		data = new char[strlen(errorCodes[i].response) * 2
				+ strlen(errorCodes[i].text)
				+ strlen(responseStart)
				+ 80 // date
				+ 128 // for good luck
			];
		if (NULL == data)
		{
			fprintf(stderr, "Out of memory %d\n", __LINE__);
			exit(-1);
		}
		dataLength = sprintf(data,
				     responseStart,
				     errorCodes[i].response,
				     "text/plain");
		dataLength +=
			sprintf(data + dataLength, "\r\n%s\n%s\n",
				errorCodes[i].response, errorCodes[i].text);
	}
	else
	{
		fprintf(stderr, "Unknown code %d\n", errorCode);
		exit(-1);
	}
}
bool ResponseError::HasData()
{
	return dataLength > dataPos;
}
bool ResponseError::Write(int fd)
{
	int ret;
	ret = write(fd, data + dataPos, dataLength - dataPos);
	if (ret > 0)
	{
		dataPos += ret;
	}
}





ResponseStatic::~ResponseStatic()
{
	delete[] data;
}
ResponseStatic::ResponseStatic(const char *contentType, const char *content)
{
	int i;
	dataPos = 0;

	data = new char[strlen(responseStart) + strlen(contentType) + strlen(content)
			+ 128 // for good luck
			];
	if (NULL == data)
	{
		fprintf(stderr, "Out of memory %d\n", __LINE__);
		exit(-1);
	}
	dataLength = sprintf(data,
			     responseStart,
			     "200 OK",
			     contentType);
	dataLength +=
		sprintf(data + dataLength, "\r\n%s\n",
			content);
}
bool ResponseStatic::HasData()
{
	return dataLength > dataPos;
}
bool ResponseStatic::Write(int fd)
{
	int ret;
	ret = write(fd, data + dataPos, dataLength - dataPos);
	if (ret > 0)
	{
		dataPos += ret;
	}
}



ResponseMovement::~ResponseMovement()
{
	delete[] data;
	if (responseCurrentMovementObject == this)
		responseCurrentMovementObject = NULL;
}
ResponseMovement::ResponseMovement(const char *contentType, const char *content)
{
	int i;
	dataPos = 0;
	done = 0;

	data = new char[strlen(responseStart) + strlen(contentType) + strlen(content)
			+ 128 // for good luck
			];
	if (NULL == data)
	{
		fprintf(stderr, "Out of memory %d\n", __LINE__);
		exit(-1);
	}
	dataLength = sprintf(data,
			     responseStart,
			     "200 OK",
			     contentType);
	dataLength +=
		sprintf(data + dataLength, "\r\n%s\n",
			content);
	printf("Created response movement with data '%s'\n",content);
}
bool ResponseMovement::HasData()
{
	return dataLength > dataPos;
}
bool ResponseMovement::Done()
{
	return done && !HasData();
}
bool ResponseMovement::Write(int fd)
{
	int ret;
	ret = write(fd, data + dataPos, dataLength - dataPos);
	if (ret > 0)
	{
		dataPos += ret;
	}
}
void ResponseMovement::AddContent(const char *content)
{
	int newDataLength;
	char *newData;
	newDataLength = 1 + strlen(content) + dataLength - dataPos;
	newData = new char[newDataLength];
	if (newData)
	{
		memcpy(newData,
		       data + dataPos,
		       dataLength - dataPos);
		dataLength -= dataPos;
		dataPos = 0;
		strcpy(newData + dataLength, content);
		dataLength = newDataLength;
		delete[] data;
		data = newData;
	}
	else
	{
		fprintf(stderr, "Out of memory %s %d\n", __FILE__, __LINE__);
		exit(-1);
	}
}

void ResponseMovement::WriteFinishedBlock()
{
	AddContent("<p><strong>\nFinished\n</strong></p>\n"
		   "<!-- \nFinished\nFinished\nFinished\n -->\n"
		   "</body></html>\n");
	done = !0;
}

