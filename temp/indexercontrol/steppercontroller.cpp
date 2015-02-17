#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "indexercontrol.h"


	
StepperControllerData::StepperControllerData(int nfd, int pStepsInCircle) : SelectTarget(9, nfd)
{
	keepEnergized = 0;
	doingCommand = 0;
	bytesWanted = 1;
	stepsInCircle = pStepsInCircle;

	motorPosition[0] = 0;
	motorPosition[1] = 0;
	motorPosition[2] = 0;
	
	if (fd >= 0)
	{
		struct termios newtio;
		struct termios oldtio;
		
		// allow the process to receive SIGIO
		// fcntl(fd, F_SETOWN, getpid());
		// Make the file descriptor asynchronous (the manual page says only
		// O_APPEND and O_NONBLOCK, will work with F_SETFL...)
		fcntl(fd, F_SETFL, FASYNC);
		
		tcgetattr(fd,&newtio); // save current port settings
		// set new port settings for canonical input processing
		tcflush(fd, TCIFLUSH);
		
		newtio.c_cflag |= CLOCAL | CREAD;
		cfsetospeed(&newtio, B9600);
		cfsetispeed(&newtio, B9600);
		newtio.c_cflag &= ~CSIZE; /* Mask the character size bits */
		newtio.c_cflag |= CS8;    /* Select 8 data bits */
		newtio.c_cflag &= ~PARENB;
		newtio.c_cflag &= ~CSTOPB;
		newtio.c_cflag &= ~CSIZE;
		newtio.c_cflag |= CS8;
//		newtio.c_cflag &= ~CNEW_RTSCTS;
		newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
		newtio.c_iflag &= ~(IXON | IXOFF | IXANY);
		newtio.c_oflag &= ~OPOST;
		
		tcsetattr(fd,TCSANOW,&newtio);
	}
	NewCommand();
};


void StepperControllerData::NewCommand()
{
}

void StepperControllerData::StepAbs(int motor, long pos, unsigned int mindelay, unsigned int maxdelay)
{
	int steps;
	
	while (pos < 0)
		pos += stepsInCircle;
	pos %= stepsInCircle;
	
	printf("Rotate Absolute: motor pos %ld to %ld\n", motorPosition[motor], pos);
	
	if (motorPosition[motor] == pos)
	{
		DoneWithThisEntryInStateQueue();
		return;
	}
	
	int stepsBack, stepsForward;
	stepsForward = (pos - motorPosition[motor] + stepsInCircle)
		% stepsInCircle;
	stepsBack = (motorPosition[motor] - pos + stepsInCircle) 
		% stepsInCircle;
	if (stepsBack > stepsForward)
	{
		steps = stepsForward;
	}
	else
	{
		steps = -stepsBack;
	}
	printf("   Stepping %ld from %ld to %ld\n", steps, motorPosition[motor], pos);
	Step(motor, steps, mindelay, maxdelay);
}

void StepperControllerData::Step(int motor, int steps, unsigned int mindelay, unsigned int maxdelay)
{
	unsigned char achBuffer[256];
	int bufpos;

	bufpos = 0;

	if (motor < 0 || motor > 2)
	{
		fprintf(stderr, "Motor %d out of range\n", motor);
	}

	printf("Motor %d position %ld changing by %d\n",
	       motor,
	       motorPosition[motor],
	       steps);
	
	achBuffer[bufpos++] = 'M';
	achBuffer[bufpos++] = '0'+motor;

       	if (!steps)
		return;
	motorPosition[motor] += steps;
	
	while (motorPosition[motor] < 0)
		motorPosition[motor] += stepsInCircle;
	
	if (steps < 0)
	{
		steps = -steps;
		achBuffer[bufpos++] = 'C';
	}
	else
	{
		achBuffer[bufpos++] = 'c';
	}

	achBuffer[bufpos++] = 'f'; // 'F' rotate full-steps, 'f' half-steps
	achBuffer[bufpos++] = keepEnergized ? 'H' : 'h';

	if (maxdelay < mindelay)
		maxdelay = mindelay;
	
	if ((maxdelay - mindelay) * 2 >= (steps - 2))
		mindelay = maxdelay - (steps - 2) / 2;
	steps -= (maxdelay - mindelay) * 2;
	

	if (maxdelay == mindelay)
	{
		achBuffer[bufpos++] = 'r';
	}
	else
	{
		
		achBuffer[bufpos++] = 'R';
	}

	achBuffer[bufpos++] = 'D';
	achBuffer[bufpos++] = steps / 256;
	achBuffer[bufpos++] = steps % 256;

	achBuffer[bufpos++] = 't';
	achBuffer[bufpos++] = mindelay / 256;
	achBuffer[bufpos++] = mindelay % 256;

	if (maxdelay != mindelay)
	{
		achBuffer[bufpos++] = 'T';
		achBuffer[bufpos++] = maxdelay / 256;
		achBuffer[bufpos++] = maxdelay % 256;
	}
	achBuffer[bufpos++] = 'E';

	if (bufpos != write(fd, achBuffer, bufpos))
	{
		perror("Write failed\n");
	}
};


void StepperControllerData::SendCommand()
{
}

void StepperControllerData::Abort()
{
	unsigned char achBuffer[256];
	int bufpos;
	bufpos = 0;

	achBuffer[bufpos++] = 'S';
	if (bufpos != write(fd, &achBuffer, bufpos))
	{
		perror("Write failed\n");
	}
}

void StepperControllerData::CallStepEnd()
{
	if (stateQueueEntry >= 0
	    && stateQueueEntry < stateQueueEntries
	    && 0 != stateQueue[stateQueueEntry])
	{
		StateBase * nextQueueEntry;
		nextQueueEntry =
			stateQueue[stateQueueEntry]->EndRotation();
		if (nextQueueEntry)
		{
			if (stateQueue[stateQueueEntry] != nextQueueEntry)
			{
				printf("Replacing Queue entry\n");
				delete stateQueue[stateQueueEntry];
				stateQueue[stateQueueEntry] =
					nextQueueEntry;
				stateQueue[stateQueueEntry]->Init();
			}
		}
		else
		{
			DoneWithThisEntryInStateQueue();
		}
	}
}


void StepperControllerData::Execute()
{
	static unsigned char deEnergizeMotors[3] = { 67,67,67};
	int i;
	
	
	for (i = 0; i < bytesGotten; i++)
	{
		printf(" %02.2x", bytes[i]);
	}
	printf("\n");


	printf("\nStepper Controller:");
	if (bytesWanted == 1)
	{
		switch (bytes[0])
		{
		case '?':
			bytesWanted = 9;
			break;
		case 'S' :
			static const unsigned char questionMark = '?';
			if (1 != write(fd, &questionMark, sizeof(questionMark)))
			{
				perror("Unable to request status from controller\n");
			}
		}
	}
	else if (bytesWanted == 9)
	{
		printf("Motor end status: %d %d %d\n",
		       bytes[0 + 2] * 65536
		       + bytes[0 + 1] * 256
		       + bytes[0 + 0],
		       bytes[1 + 2] * 65536
		       + bytes[1 + 1] * 256
		       + bytes[1 + 0],
		       bytes[2 + 2] * 65536
		       + bytes[2 + 1] * 256
		       + bytes[2 + 0]);
		doingCommand = 0;
		bytesWanted = 1;
		CallStepEnd();
	}
	else
	{
		fprintf(stderr, "Stepper state machine corrupt!\n");
		bytesWanted = 1;
	}
}

