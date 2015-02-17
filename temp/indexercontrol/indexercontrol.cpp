#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <netdb.h>
#include <ctype.h>
#include "indexercontrol.h"



int stepsMinDelay = 8;
int stepsMaxDelay = 30;
int indexerZeroOffset = 0;
int stepsInCircle = 4114; // 400 * 14 / 72

int mouseStepsInCircle = 500;
int mouseVariationInCircle = 100;
int mouseStaticMoveThresholdLocked = 16;
int mouseStaticMoveThresholdUnlocked = 3;
int mouseStaticMoveThreshold = 3;
int lastKnownMousePosition = INT_MIN;

ResponseMovement *responseCurrentMovementObject = NULL;

int GetNum(char *s)
{
	while (*s == ' ')
		s++;
	return atoi(s);
}

int StringMatches(const char *s, const char *c, int len)
{
	int slen;
	slen = strlen(s);
	return (len >= slen
		&& !memcmp(s,c,slen));
}
void
ReadIndexerConfig()
{
	FILE *f;
	char s[256];
	static const char  configString_stepsMinDelay[] = "Step Delay Min";
	static const char  configString_stepsMaxDelay[] = "Step Delay Max";
	static const char  configString_indexerZeroOffset[] = "Zero Offset";
	static const char  configString_stepsInCircle[] = "Steps In Circle";
	static const char  configString_mouseStepsInCircle[] = "Mouse Steps";
	static const char  configString_mouseVariationInCircle[] = "Mouse Variation";
	static const char  configString_mouseStaticMoveThreshold[] = "Mouse Threshold";
	static const char  configString_mouseStaticMoveThresholdLocked[] = "Mouse Locked Threshold";

	f = fopen("/etc/indexerrc", "r");
	if (f)
	{
		while (fgets(s, sizeof(s), f))
		{
			int start;

			start = 0;
			while (' ' == s[start])
				start++;
			if (s[start] != '#' && s[start] != '\n')
			{
				if (StringMatches(configString_stepsMinDelay, s + start,
						  sizeof(s) - start))
				{
					stepsMinDelay = GetNum(s + start + strlen(configString_stepsMinDelay));
				}
				else if (StringMatches(configString_stepsMaxDelay, s + start,
							  sizeof(s) - start))
				{
					stepsMaxDelay = GetNum(s + start + strlen(configString_stepsMaxDelay));
				}
				else if (StringMatches(configString_indexerZeroOffset, s + start,
						       sizeof(s) - start))
				{
					indexerZeroOffset = GetNum(s + start + strlen(configString_indexerZeroOffset));
				}
				else if (StringMatches(configString_stepsInCircle, s + start,
						       sizeof(s) - start))
				{
					stepsInCircle = GetNum(s + start + strlen(configString_stepsInCircle));
				}
				else if (StringMatches(configString_mouseStepsInCircle, s + start,
						       sizeof(s) - start))
				{
					mouseStepsInCircle = GetNum(s + start + strlen(configString_mouseStepsInCircle));
				}
				else if (StringMatches(configString_mouseVariationInCircle, s + start,
						       sizeof(s) - start))
				{
					mouseVariationInCircle = GetNum(s + start + strlen(configString_mouseVariationInCircle));
				}
				else if (StringMatches(configString_mouseStaticMoveThreshold, s + start,
						       sizeof(s) - start))
				{
					mouseStaticMoveThreshold = GetNum(s + start 
									  + strlen(configString_mouseStaticMoveThreshold));
					mouseStaticMoveThresholdUnlocked = mouseStaticMoveThreshold;
				}
				else if (StringMatches(configString_mouseStaticMoveThresholdLocked, s + start,
						       sizeof(s) - start))
				{
					mouseStaticMoveThresholdLocked = GetNum(s + start 
									  + strlen(configString_mouseStaticMoveThresholdLocked));
				}
			}
		}
		fclose(f);
	}
}

int mouseStatusTextPrinting = 0;




int stateQueueEntries = 0;
int stateQueueEntry = -1;
StateBase *stateQueue[MAXSTATEQUEUEENTRIES];



#define NUMFILECONTROLS 16

static struct FileControl 
{
	int file;
	SelectTarget *target;
} fileControls[NUMFILECONTROLS];
int numFileControls;

StepperControllerData *stepperdata;
MouseData *mousedata;


int termsig = 0;
int got_sighup = 0;
int listensocket;

static void catchsignal(int sig)
{
    if (SIGTERM == sig || SIGINT == sig)
	termsig = sig;
    if (SIGHUP == sig)
	got_sighup = 1;
}

#define MAX_CONNECTIONS 16


#include "request.h"



void
mainloop()
{
	Request *connections = NULL;
	int current_connections = 0;
	time_t now;
	Request *request, *requestPrev, *requestTmp;
	struct timeval      tv;
	int                 max_fd,length;
	fd_set              read_fds,write_fds;

	for (;!termsig;) 
	{
		FD_ZERO(&read_fds);
		FD_ZERO(&write_fds);
		max_fd = 0;
		/* add listening socket */
		if (current_connections < MAX_CONNECTIONS) {
			FD_SET(listensocket,&read_fds);
			max_fd = listensocket;
		}

		int i;
		for (i = 0; i < NUMFILECONTROLS; i++)
		{
			if (fileControls[i].target)
			{
				if (fileControls[i].file > max_fd)
					max_fd = fileControls[i].file;
				FD_SET(fileControls[i].file, &read_fds);
			}
		}

		/* add connection sockets */
		for (request = connections; request; request = request->nextRequest) 
		{
			if (request->WantsRead())
			{
				FD_SET(request->fd, &read_fds);
				if (request->fd > max_fd)
					max_fd = request->fd;
			}
			if (request->WantsWrite())
			{
				FD_SET(request->fd, &write_fds);
				if (request->fd > max_fd)
					max_fd = request->fd;
			}
		}
		/* go! */
		tv.tv_sec  = KEEPALIVE_TIME;
		tv.tv_usec = 0;
		if (-1 == select(max_fd+1,
				 &read_fds,
				 &write_fds,
				 NULL,&tv))
		{
			continue;
		}
		now = time(NULL);
		
		/* new connection ? */
		if (FD_ISSET(listensocket,&read_fds)) 
		{
			request = new Request(now);
			if (request)
			{
				request->fd = accept(listensocket, NULL, NULL);
				if (-1 == request->fd)
				{
					if (EAGAIN != errno)
						perror("accept");
					delete request;
				}	
				else
				{
					fcntl(request->fd,F_SETFL,O_NONBLOCK);
					request->nextRequest = connections;
					connections = request;
					current_connections++;
					fprintf(stderr,"%03d: new request (%d)\n",request->fd,current_connections);
				}
			}
		}
		

		for (i = 0; i < NUMFILECONTROLS; i++)
		{
			if (fileControls[i].target && FD_ISSET(fileControls[i].file, &read_fds))
			{
				fileControls[i].target->Read(fileControls[i].file);
			}
		}

		/* check active connections */
		for (request = connections, requestPrev = NULL; request != NULL;) 
		{
			/* I/O */
			if (FD_ISSET(request->fd,&read_fds)) 
			{
				request->Read(now);
			}
			if (FD_ISSET(request->fd,&write_fds)) {
				request->Write(now);
			}
			
			if (request->WantsClose(now)) 
			{
				close(request->fd);
				current_connections--;
				/* unlink from list */
				requestTmp = request;
				if (requestPrev == NULL) {
					connections = request->nextRequest;
					request = connections;
				} else {
					requestPrev->nextRequest = request->nextRequest;
					request = request->nextRequest;
				}
				delete requestTmp;
			} 
			else 
			{
				requestPrev = request;
				request = request->nextRequest;
			}
		}
	}
}


void
AllocateFileControls()
{
	fileControls[numFileControls].file = open("/dev/psaux", O_RDWR | O_NONBLOCK | O_NDELAY);

	if (-1 == fileControls[numFileControls].file)
	{
		fprintf(stderr,"Unable to open mouse\n");
		exit(-1);
	}
	
	mousedata = new MouseData(fileControls[numFileControls].file);
	fileControls[numFileControls].target = mousedata;

	numFileControls++;



	fileControls[numFileControls].file = open("/dev/ttyS0", 
						  O_RDWR | O_NOCTTY | O_NONBLOCK | O_NDELAY);
	stepperdata = new StepperControllerData(fileControls[numFileControls].file, stepsInCircle);
	fileControls[numFileControls].target = stepperdata;

	numFileControls++;





}

int
main(int argc, char *argv[])
{
	int socket_opt;
	struct addrinfo          ask,*res;
	memset(&ask, 0, sizeof(ask));
	ask.ai_flags = AI_PASSIVE;
	ask.ai_socktype = SOCK_STREAM;
	ask.ai_family = PF_INET;

	int resultCode;


	ReadIndexerConfig();

	int i;
	for (i = 0; i < NUMFILECONTROLS; i++)
	{
		fileControls[i].file = -1;
		fileControls[i].target = 0;
	}
	numFileControls = 0;

	AllocateFileControls();

	if (0 != (resultCode = getaddrinfo(NULL, "80", &ask, &res))) 
	{
		fprintf(stderr,"getaddrinfo (ipv4): %s\n",gai_strerror(resultCode));
		exit(1);
	}
	
	listensocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	
	if (-1 == listensocket)
	{
		perror("Socket");
		exit(-1);
	}
	


	socket_opt = 1;
	setsockopt(listensocket,SOL_SOCKET,SO_REUSEADDR,&socket_opt,sizeof(socket_opt));
	fcntl(listensocket,F_SETFL,O_NONBLOCK);
	struct sockaddr_storage  ss;
	memcpy(&ss,res->ai_addr,res->ai_addrlen);
	
	if (-1 == bind(listensocket, (struct sockaddr*)&ss, res->ai_addrlen)) 
	{
		perror("bind");
		exit(1);
	}
	
	if (-1 == listen(listensocket, 2*MAX_CONNECTIONS)) 
	{
		perror("listen");
		exit(1);
	}
	struct sigaction         signal_action, signal_old;
	
	
	/* setup signal handler */
	memset(&signal_action,0,sizeof(signal_action));
	sigemptyset(&signal_action.sa_mask);
	signal_action.sa_handler = SIG_IGN;
	sigaction(SIGPIPE,&signal_action,&signal_old);
	sigaction(SIGCHLD,&signal_action,&signal_old);
	signal_action.sa_handler = catchsignal;
	sigaction(SIGHUP,&signal_action,&signal_old);
	sigaction(SIGTERM,&signal_action,&signal_old);
	
	mainloop();
	exit(0);
}
