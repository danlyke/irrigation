#include <termios.h>
#include <unistd.h>
#include <stdio.h>

#include "indexercontrol.h"
#define MOUSE_SEND_ID    0xF2
#define MOUSE_ID_ERROR   -1
#define MOUSE_ID_PS2     0
#define MOUSE_ID_IMPS2   3

#define MOUSE_SET_RES        0xE8  /* Set resolution */
#define MOUSE_SET_SCALE11    0xE6  /* Set 1:1 scaling */ 
#define MOUSE_SET_SCALE21    0xE7  /* Set 2:1 scaling */
#define MOUSE_GET_SCALE      0xE9  /* Get scaling factor */
#define MOUSE_SET_STREAM     0xEA  /* Set stream mode */
#define MOUSE_SET_SAMPLE     0xF3  /* Set sample rate */ 
#define MOUSE_ENABLE_DEV     0xF4  /* Enable aux device */
#define MOUSE_DISABLE_DEV    0xF5  /* Disable aux device */
#define MOUSE_RESET          0xFF  /* Reset aux device */
#define MOUSE_ACK            0xFA  /* Command byte ACK. */ 

                                                      


/**
 * Writes the given data to the ps2-type mouse.
 * Checks for an ACK from each byte.
 * 
 * Returns 0 if OK, or >0 if 1 or more errors occurred.
 */
static int write_to_mouse(int fd, unsigned char *data, size_t len)
{
  int i;
  int error = 0;
  for (i = 0; i < len; i++) {
    unsigned char c;
    write(fd, &data[i], 1);
    read(fd, &c, 1);
    if (c != MOUSE_ACK) {
      error++;
    }
  }

  /* flush any left-over input */
  usleep (30000);
  tcflush (fd, TCIFLUSH);
  return(error);
}

/*
 * Sends the SEND_ID command to the ps2-type mouse.
 * Return one of MOUSE_ID_...
 */
static int read_mouse_id(int fd)
 {
  unsigned char c = MOUSE_SEND_ID;
  unsigned char id;

  write(fd, &c, 1);
  read(fd, &c, 1);
  if (c != MOUSE_ACK) {
    return(MOUSE_ID_ERROR);
  }
  read(fd, &id, 1);

  return(id);
}



int initmouse(int fd)
{
	int id;
	static unsigned char basic_init[] = { MOUSE_ENABLE_DEV, MOUSE_SET_SAMPLE, 100 };
	static unsigned char imps2_init[] = { MOUSE_SET_SAMPLE, 200, MOUSE_SET_SAMPLE, 100, MOUSE_SET_SAMPLE, 80, };
	static unsigned char ps2_init[] = { MOUSE_SET_SCALE11, MOUSE_ENABLE_DEV, MOUSE_SET_SAMPLE, 20, MOUSE_SET_RES, 3, };

	/* Do a basic init in case the mouse is confused */
	write_to_mouse(fd, basic_init, sizeof (basic_init));
	
	/* Now try again and make sure we have a PS/2 mouse */
	if (write_to_mouse(fd, basic_init, sizeof (basic_init)) != 0) {
		fprintf(stderr, "imps2: PS/2 mouse failed init");
		return 3;
	}

	/* Try to switch to 3 button mode */
	if (write_to_mouse(fd, imps2_init, sizeof (imps2_init)) != 0) {
		fprintf(stderr, "imps2: PS/2 mouse failed (3 button) init");
		return 3;
	}

	/* Read the mouse id */
	id = read_mouse_id(fd);
	if (id == MOUSE_ID_ERROR) {
		fprintf(stderr, "imps2: PS/2 mouse failed to read id, assuming standard PS/2");
		id = MOUSE_ID_PS2;
	}

	/* And do the real initialisation */
	if (write_to_mouse(fd, ps2_init, sizeof (ps2_init)) != 0) {
		fprintf(stderr, "imps2: PS/2 mouse failed setup, continuing...");
	}

	if (id == MOUSE_ID_IMPS2) {
		/* Really an intellipoint, so initialise 3 button mode (4 byte packets) */
		fprintf(stderr, "imps2: Auto-detected intellimouse PS/2");

		return 4;
	}
	if (id != MOUSE_ID_PS2) {
		fprintf(stderr, "imps2: Auto-detected unknown mouse type %d, assuming standard PS/2", id);
	}
	else {
		fprintf(stderr, "imps2: Auto-detected standard PS/2");
	}
//  for (type=mice; type->fun; type++) {
//    if (strcmp(type->name, "ps2") == 0) {
//     return(type);
//    }
// }
	/* ps2 was not found!!! */
	return 3;
}




MouseData::MouseData(int fdin) : SelectTarget(4,fdin)
{
	mousePrevious = -1;
	positionKnown = 0;
	mousePosition = 0;

	bytesWanted = 3 ; // initmouse(fd);
};

void
MouseData::ProcessMousePositionReset()
{
	if (stateQueueEntry >= 0
	    && stateQueueEntry < stateQueueEntries
	    && 0 != stateQueue[stateQueueEntry])
	{
		StateBase * nextQueueEntry;
		nextQueueEntry =
			stateQueue[stateQueueEntry]->HomeSwitch();
		if (nextQueueEntry)
		{
			if (stateQueue[stateQueueEntry] != nextQueueEntry)
			{
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
	printf("Reset mouse position\n");
	fflush(stdout);
	positionKnown = 1;
	mousePosition = 0;
}
void MouseData::Execute()
{
	
	if (mouseStatusTextPrinting)
	{
		int i;
		printf("Mouse Bytes:");
		for (i = 0; i < bytesWanted; i++)
		{
			printf(" %02.2x", bytes[i]);
		}
	}

	if (bytes[0] & 0x01)
	{
		if (mousePrevious == 0 && (bytes[0] & 0x20))
		{
			ProcessMousePositionReset();
		}
		mousePrevious = 1;
		if (mouseStatusTextPrinting)
			printf("Down ");
	}
	else
	{
		if (mousePrevious == 1 && !(bytes[0] & 0x20))
		{
			ProcessMousePositionReset();
			positionKnown = 1;
			mousePosition = 0;
		}
		mousePrevious = 0;
		if (mouseStatusTextPrinting)
			printf("Up   ");
	}
	
	if (mouseStatusTextPrinting)
	{
		if (bytes[0] & 0x20)
			printf("Backwards ");
		else
			printf("Forwards ");
	}
	
	
	mousePosition -= (signed char)bytes[2];
	if (mouseStatusTextPrinting)
		printf(" %d pos %ld              \n", (signed char)bytes[2], mousePosition);
	fflush(stdout);
}
