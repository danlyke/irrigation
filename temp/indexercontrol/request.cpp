#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "documents.h"
#include "request.h"
#include "indexercontrol.h"

static char templateTest_cgi[] =
"<html><head><title>GVS Rig Controller - Test Info</title></head>\n"
"<body><h1>GVS Rig Controller - Test Info</h1>\n"
"<table>\n"
"<tr><th>Minimum step delay</th><td>%d</td></tr>\n"
"<tr><th>Maximum step delay</th><td>%d</td></tr>\n"
"<tr><th>Indexer zero offset</th><td>%d</td></tr>\n"
"</table></body></html>";

class StateFHSeekBack : public StateBase {
private:
	StepperControllerData *controller;
	MouseData *mouse;
	long mouseBeforeSeekBack;
	long mouseAtHomeSwitch;
public:

	StateFHSeekBack(StepperControllerData *pcontroller, MouseData *pmouse)
		: StateBase()
	{
		mouse = pmouse;
		controller = pcontroller;
	};

	void Init()
	{
		printf("StateFHSeekBack::Init\n");
		mouseBeforeSeekBack = mouse->GetMousePosition();
		controller->NewCommand();
		controller->Step(0, -STEPSTOSTOP, STEPSMINDELAY, STEPSMAXDELAY);
		controller->SendCommand();
	}
	StateBase *HomeSwitch()
	{
		printf("StateFHSeekBack::HomeSwitch()\n");
		mouseAtHomeSwitch = mouse->GetMousePosition();
		return this;
	}
	StateBase *EndRotation()
	{
		printf("StateFHSeekBack::EndRotation() mouse: %d home %d seekback %d\n",
		       mouse->GetMousePosition(), mouseAtHomeSwitch, mouseBeforeSeekBack);
		
		if (mouseBeforeSeekBack
		    && mouseBeforeSeekBack != mouse->GetMousePosition())
		{
			printf("Setting motor position: STEPSTOSTOP is %d mouse pos is %ld seek pos is %ld result %ld\n",
			       STEPSTOSTOP,
			       mouse->GetMousePosition(),
			       mouseBeforeSeekBack,
			       (STEPSTOSTOP * mouse->GetMousePosition() 
				/ (mouse->GetMousePosition() - mouseBeforeSeekBack)));

			controller->SetMotorPosition(0, 
						     (STEPSTOSTOP * mouse->GetMousePosition() 
						      / (mouse->GetMousePosition() - mouseBeforeSeekBack)));
		}
		else
		{
			controller->SetMotorPosition(0,0); 
		}
		return NULL;
	}
};


class StateFHLongRotate : public StateBase {
private:
	StepperControllerData *controller;
	MouseData *mouse;
public:

	StateFHLongRotate(StepperControllerData *pcontroller, MouseData *pmouse)
		: StateBase()
	{
		mouse = pmouse;
		controller = pcontroller;
	};
	void Init()
	{
		printf("StateFHLongRotate::Init\n");
		controller->NewCommand();
		controller->Step(0, stepsInCircle * 2, STEPSMINDELAY, STEPSMAXDELAY);
		controller->SendCommand();
	}
	StateBase *HomeSwitch()
	{
		controller->Abort();
		printf("StateFHLongRotate::HomeSwitch()\n");
		return this;
	};
	StateBase *EndRotation()
	{
		printf("StateFHLongRotate::EndRotation()\n");
		return new StateFHSeekBack(controller, mouse);
	}
};


class StateSeekTo : public StateBase {
private:
	StepperControllerData *controller;
	MouseData *mouse;
	int pos;
	long motorPositionStart;

	long mousePositionStart;
	long mousePositionOverrun;

public:

	StateSeekTo(StepperControllerData *pcontroller,
		    MouseData *pmouse,
		    int ppos)
		: StateBase()
	{
		mouse = pmouse;
		controller = pcontroller;
		pos = ppos;
	};
	void Init()
	{
		printf("StateSeekTo::Init\n");
		mousePositionStart = mouse->GetMousePosition();
		mousePositionOverrun = 0;
		motorPositionStart = controller->GetMotorPosition(0);

		controller->NewCommand();
		controller->StepAbs(0, pos + indexerZeroOffset, STEPSMINDELAY, STEPSMAXDELAY);
		controller->SendCommand();
	}
	StateBase *HomeSwitch()
	{
		mousePositionOverrun = mouse->GetMousePosition() - mousePositionStart;
		mousePositionStart = 0;

		printf("StateSeekTo::HomeSwitch()\n");
		return this;
	};
	StateBase *EndRotation()
	{
		mousePositionOverrun += mouse->GetMousePosition() - mousePositionStart;
		lastKnownMousePosition = mouse->GetMousePosition();

		while (lastKnownMousePosition < 0)
			lastKnownMousePosition += mouseStepsInCircle;

		if (abs(controller->GetMotorPosition(0) * mouseStepsInCircle
			- lastKnownMousePosition * stepsInCircle)
		    > mouseVariationInCircle * stepsInCircle)
		{
//			mouse->InvalidatePosition();
//			if (outfd > -1)
//				writeToBoth(outfd, "Movement Mismatch\n",18);
		}
		printf("StateSeekTo::EndRotation()\n");
		char achStuff[256];
		
		snprintf(achStuff, sizeof(achStuff), "\n<p>Motor at %ld deg steps %ld  mouse steps %ld motor change %d motor steps per mouse %lf</p>\n",
		       ((controller->GetMotorPosition(0) * 360 + stepsInCircle/2) / stepsInCircle),
		       controller->GetMotorPosition(0),
		       mousePositionOverrun,
		       controller->GetMotorPosition(0) - motorPositionStart,
		       (double)(controller->GetMotorPosition(0) - motorPositionStart) / (double)mousePositionOverrun);
		if (responseCurrentMovementObject)
			responseCurrentMovementObject->AddContent(achStuff);
		return  NULL;
	}
};




void Request::CreateRotateObject(int degrees, bool lock)
{
	int mouseComparisonPosition;
	int toPos;

	stepperdata->SetKeepEnergized(1);
	if (responseCurrentMovementObject != NULL)
	{
		responseCurrentMovementObject->AddContent("<p><strong>\nInterrupted!\n</strong></p>\n");
		responseCurrentMovementObject->WriteFinishedBlock();
	}




	toPos = degrees;
	toPos *= stepsInCircle;
	toPos /= 360;

	mouseComparisonPosition = mousedata->GetMousePosition();
	while (mouseComparisonPosition < 0)
		mouseComparisonPosition += mouseStepsInCircle;

	if (mousedata->GetPositionKnown()
	    && abs(lastKnownMousePosition - mouseComparisonPosition) 
	    < (stepperdata->GetKeepEnergized() ? mouseStaticMoveThresholdLocked : mouseStaticMoveThresholdUnlocked))
	{
		if (toPos == stepperdata->GetMotorPosition(0))
		{
			char ach[512];
			sprintf(ach,
				"<html><head><title>GVS Indexer Rig - Movement to %d</title></head>\n"
				"<body><h1>GVS Indexer Rig - Movement to %d</h1>\n"
				"<p>\nHolding at current position\n</p>\n"
				"<p><strong>\nFinished\n</strong></p>\n"
				"<!-- \nFinished\nFinished\nFinished\n -->\n"
				"</body></html>\n",
				degrees, degrees);

			response = new ResponseStatic("text/html",ach);
		}
		else
		{
			char ach[256];
			sprintf(ach,
				"<html><head><title>GVS Indexer Rig - Movement to %d</title></head>\n"
				"<body><h1>GVS Indexer Rig - Movement to %d</h1>\n",
				degrees, degrees);

			responseCurrentMovementObject = new ResponseMovement("text/html",ach);
			response = responseCurrentMovementObject;
			responseCurrentMovementObject->AddContent("<p>\nPosition Known\n</p>");
		
			stateQueue[0] = new StateSeekTo(stepperdata,
							mousedata,
							toPos);
			stateQueueEntries = 1;
			stateQueueEntry = 0;
			stateQueue[0]->Init();
		}
	}
	else
	{
		if (mousedata->GetPositionKnown())
		{
			char ach[256];
			sprintf(ach,
				"<html><head><title>GVS Indexer Rig - Movement to %d</title></head>\n"
				"<body><h1>GVS Indexer Rig - Movement to %d</h1>\n",
				degrees, degrees);

			responseCurrentMovementObject = new ResponseMovement("text/html",ach);
			response = responseCurrentMovementObject;
			char achStuff[256];
			sprintf(achStuff,
				"<p>Movement sensed: last %d compare %d diff %d energized %d locked %d unlocked %d</p>\n",
				lastKnownMousePosition, mouseComparisonPosition,
				lastKnownMousePosition - mouseComparisonPosition,
				stepperdata->GetKeepEnergized(),
				mouseStaticMoveThresholdLocked, mouseStaticMoveThresholdUnlocked);
			responseCurrentMovementObject->AddContent(achStuff);
			
		}
		else
		{
			char ach[256];
			sprintf(ach,
				"<html><head><title>GVS Indexer Rig - Movement to %d</title></head>\n"
				"<body><h1>GVS Indexer Rig - Movement to %d</h1>\n",
				degrees, degrees);

			responseCurrentMovementObject = new ResponseMovement("text/html",ach);
			response = responseCurrentMovementObject;
			responseCurrentMovementObject->AddContent("<p>\nPosition Unknown\n</p>");
		}
		stateQueue[0] = new StateFHLongRotate(stepperdata, mousedata);
		stateQueue[1] = new StateSeekTo(stepperdata, mousedata,toPos);
		stateQueueEntries = 2;
		stateQueueEntry = 0;
		stateQueue[0]->Init();
	}
}


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
							if (strlen(documents[i].path) ==  docEnd-docStart
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
								CreateRotateObject(atoi(docStart+12), 1);
							}
							else if (docEnd - docStart == 11
							    && !strncmp("/rotate.cgi",
									docStart, 11))
							{
								CreateRotateObject(0,1);
								
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
								stepperdata->SetKeepEnergized(0);
								stateQueue[0] = new StateSeekTo(stepperdata, mousedata,
												stepperdata->GetMotorPosition(0) + 2);
								stateQueueEntries = 1;
								stateQueueEntry = 0;
								stateQueue[0]->Init();
							}
							else if (docEnd - docStart == 15
								&& !strncmp("/invalidate.cgi",
									    docStart, 15))
							{
								mousedata->InvalidatePosition();
								response = new ResponseStatic("text/html",
											      "<html>\n"
											      "<head><title>GVS Rig Controller - Position Invalidated</title></head>\n"
											      "<body>\n"
											      "<h1>GVS Rig Controller - Position Invalidated</h1>\n"
											      "\n"
											      "<p>The current position of the GVS Rig Controller has been invalidated.\n"
											      "Next time it has to find a position it will seek to the home position first.\n"
											      "</p>\n"
											      "\n"
											      "<p>Working...</p>\n"
											      "<p><strong>\n"
											      "Finished\n"
											      "</strong></p>\n"
											      "</body></html>\n"
											      "\n");
							}
							else if (docEnd - docStart == 9
								&& !strncmp("/test.cgi",
									    docStart, 9))
							{
								char *s;
								s  = new char[strlen(templateTest_cgi) + 256];
								sprintf(s, templateTest_cgi,
									stepsMinDelay,
									stepsMaxDelay,
									indexerZeroOffset);
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
		}
	}
