#include <unistd.h>
#include <stdio.h>
#include "response.h"
#include "indexercontrol.h"

void SelectTarget::DoneWithThisEntryInStateQueue()
{
	delete stateQueue[stateQueueEntry];
	stateQueueEntry++;
	if (stateQueueEntry < stateQueueEntries)
		stateQueue[stateQueueEntry]->Init();
	else if (responseCurrentMovementObject)
	{
		responseCurrentMovementObject->WriteFinishedBlock();
		responseCurrentMovementObject = NULL;
	}
}
