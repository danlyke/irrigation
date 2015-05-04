#ifndef FBYHTTPSERVER_H
#define FBYHTTPSERVER_H

#define KEEPALIVE_TIME 20


#define UNCOPYABLE(T) private: T(const T&); T& operator=(const T&)


class Response;
class ResponseMovement;
extern ResponseMovement *responseCurrentMovementObject;


extern int stepsMinDelay;
extern int stepsMaxDelay;
extern int indexerZeroOffset;
extern int stepsInCircle;
extern int mouseStepsInCircle;
extern int mouseVariationInCircle;
extern int mouseStaticMoveThresholdLocked;
extern int mouseStaticMoveThresholdUnlocked;
extern int mouseStaticMoveThreshold;
extern int lastKnownMousePosition;


#define STEPSTOSTOP 30
#define STEPSMINDELAY (stepsMinDelay)
#define STEPSMAXDELAY (stepsMaxDelay)

extern int mouseStatusTextPrinting;



class StateBase {
protected:
public:
	virtual void Init() { };
	virtual StateBase *HomeSwitch()
	{
		return 0;
	}
	virtual StateBase *EndRotation()
	{
		return 0;
	}
	virtual StateBase *MouseRotationError()
	{
		return 0;
	}
};

#define MAXSTATEQUEUEENTRIES 16
extern int stateQueueEntries;
extern int stateQueueEntry;
extern StateBase *stateQueue[MAXSTATEQUEUEENTRIES];

class RWTarget
{
    UNCOPYABLE(RWTarget);

protected:
	int fd;
	size_t totalBytes;
	size_t bytesWanted;
	size_t bytesGotten;
	unsigned char *bytes;

public:
	RWTarget(size_t maxBytes, int fdin = -1);
	~RWTarget();
	virtual void Execute() = 0;
	ssize_t Read(int rfd = -1);

	void DoneWithThisEntryInStateQueue();

};




class MouseData : public RWTarget {
private:
	long mousePosition;
	int positionKnown;
	int mousePrevious;
	void ProcessMousePositionReset();

public:
	void InvalidatePosition() { positionKnown = 0; }
	long GetMousePosition() { return mousePosition; }
	int GetPositionKnown() { return positionKnown; }
	MouseData(int fdin);
	void Execute();
};



class StepperControllerData : public RWTarget {
private:
	int stepsInCircle;
	int doingCommand;
	long motorPosition[3];

public:
	int keepEnergized;
	void SetMotorPosition(int motor, long pos)
		{
			while (pos < 0)
				pos += stepsInCircle;
			motorPosition[motor] = pos;
		}
	long GetMotorPosition(int motor)
		{
			return motorPosition[motor];
		}
	int GetKeepEnergized()
		{
			return keepEnergized;
		}

	void SetKeepEnergized(int e)
		{
			keepEnergized = e;
		}

	int DoingCommand()
		{
			return doingCommand;
		}
	
 	StepperControllerData(int nfd, int stepsInCircle);
	void NewCommand();
	void StepAbs(int motor, long pos, unsigned int mindelay, unsigned int maxdelay = 0);
	void Step(int motor, int steps, unsigned int mindelay, unsigned int maxdelay = 0);
	void CallStepEnd();
	void SendCommand();
	void Abort();
	void Execute();
};

extern StepperControllerData *stepperdata;
extern MouseData *mousedata;
extern int stepsMinDelay;
extern int stepsMaxDelay;
extern int indexerZeroOffset;
#endif
