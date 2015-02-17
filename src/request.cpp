#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>

#include "documents.h"
#include "request.h"
#include "irrigationcontrol.h"

static char templateTest_cgi[] =
"<html><head><title>GVS Rig Controller - Test Info</title></head>\n"
"<body><h1>GVS Rig Controller - Test Info</h1>\n"
"<table>\n"
"<tr><th>Minimum step delay</th><td>%d</td></tr>\n"
"<tr><th>Maximum step delay</th><td>%d</td></tr>\n"
"<tr><th>Indexer zero offset</th><td>%d</td></tr>\n"
"</table></body></html>";




void Request::Read(time_t now)
{	
	int readLen;
    last_access = now;
    switch (state)
    {
    case STATE_READ_HEADER :
        do
        {
            readLen = read(fd, headerBytes + headerPos,
					       MAX_HEADER_LENGTH - headerPos);
        }
        while (readLen < 0 &&  EINTR == errno);
        if (readLen < 0)
        {
        }
        else if (0 == readLen)
        {
            state = STATE_CLOSE;
        }
        else
        {
            char *headerEnd;
            headerPos += readLen;
            headerBytes[headerPos] = 0;
			
            if (NULL != (headerEnd = strstr(headerBytes, "\n\n")) ||
                NULL != (headerEnd = strstr(headerBytes, "\r\n\r\n")))
            {
                if (headerPos > 5
                    && !strncmp(headerBytes, "GET ", 4))
                {
                    char *docStart, *docEnd;
                    docStart = headerBytes + 4;
                    while(isspace(*docStart))
                        docStart++;
                    docEnd = docStart;
                    while (!isspace(*docEnd))
                        docEnd++;
                    
                    int i;
                    for (i = 0; documents[i].path; i++)
                    {
                        if (strlen(documents[i].path) ==  (size_t)(docEnd-docStart)
                            && !strncmp(documents[i].path, docStart,
                                        docEnd - docStart))
                        {
                            response = new ResponseStatic(documents[i].type, documents[i].document);
                            break;
                        }
                    }
                    if (NULL == response)
                    {
                        if (docEnd - docStart > 12
                            && !strncmp("/rotate.cgi?",
                                        docStart, 12))
                        {
                        }
                        else if (docEnd - docStart == 11
                                 && !strncmp("/rotate.cgi",
                                             docStart, 11))
                        {
                        }
                        else if (docEnd - docStart == 11
                                 && !strncmp("/unlock.cgi",
                                             docStart, 11))
                        {
                            response = new ResponseStatic("text/html",
											      "<html>\n"
											      "<head><title>GVS Rig Controller - Unlock</title></head>\n"
											      "<body>\n"
											      "<h1>GVS Rig Controller - Unlock</h1>\n"
											      "\n"
											      "<p>\nThe GVS Rig Controller has been unlocked\n</p>"
											      "</p>\n"
											      "\n"
											      "<p><strong>\n"
											      "Finished\n"
											      "</strong></p>\n"
											      "</body></html>\n"
											      "\n");
                        }
                        else if (docEnd - docStart == 15
                                 && !strncmp("/invalidate.cgi",
                                             docStart, 15))
                        {
                        }
                        else if (docEnd - docStart == 9
                                 && !strncmp("/test.cgi",
                                             docStart, 9))
                        {
                            char *s;
                            s  = new char[strlen(templateTest_cgi) + 256];
                            sprintf(s, templateTest_cgi);
                            response = new ResponseStatic("text/html", s);
                            delete[] s;
                        }
                    }
                    
                    if (NULL == response)
                        response = new ResponseError(404);
                }
                else if (headerPos > 5
						 && strncmp(headerBytes,"PUT ",4)
						 && strncmp(headerBytes,"HEAD ",5)
						 && strncmp(headerBytes,"POST ",5)) 
                {
                    response = new ResponseError(400);
                }
                else
                {
                    response = new ResponseError(500);
                }
                
            }
        }
    case STATE_WRITE:
    case STATE_CLOSE:
        fprintf(stderr, "Unknown state %s %d %d\n", __FILE__, __LINE__, state);
        break;
    }
}
