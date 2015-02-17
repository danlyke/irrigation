#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

ssize_t
writeToBoth(int fd, const void *p, size_t l)
{
	
	printf("Writing: %.*s", (int)l, (const char *)p);
	return write(fd, p, l);
}
int StringMatches(const char *s, const char *c, int len)
{
	int slen;
	slen = strlen(s);
	return (len >= slen
		&& !memcmp(s,c,slen));
}

int GetNum(char *s)
{
	while (*s == ' ')
		s++;
	return atoi(s);
}

void
ReadIndexerConfig()
{
	FILE *f;
	char s[256];
	static const char * configString_stepsMinDelay = "Step Delay Min";
	static const char * configString_stepsMaxDelay = "Step Delay Max";
	static const char * configString_indexerZeroOffset = "Zero Offset";
	static const char * configString_stepsInCircle = "Steps In Circle";
	static const char * configString_mouseStepsInCircle = "Mouse Steps";
	static const char * configString_mouseVariationInCircle = "Mouse Variation";
	static const char * configString_mouseStaticMoveThreshold = "Mouse Threshold";

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
				if (StringMatches(configString_stepsMinDelay, s,
						  sizeof(s) - start))
				{
					stepsMinDelay = GetNum(s + start + strlen(configString_stepsMinDelay));
				}
				else if (StringMatches(configString_stepsMaxDelay, s,
							  sizeof(s) - start))
				{
					stepsMaxDelay = GetNum(s + start + strlen(configString_stepsMaxDelay));
				}
				else if (StringMatches(configString_indexerZeroOffset, s,
						       sizeof(s) - start))
				{
					indexerZeroOffset = GetNum(s + start + strlen(configString_indexerZeroOffset));
				}
				else if (StringMatches(configString_stepsInCircle, s,
						       sizeof(s) - start))
				{
					stepsInCircle = GetNum(s + start + strlen(configString_stepsInCircle));
				}
				else if (StringMatches(configString_mouseStepsInCircle, s,
						       sizeof(s) - start))
				{
					mouseStepsInCircle = GetNum(s + start + strlen(configString_mouseStepsInCircle));
				}
				else if (StringMatches(configString_mouseVariationInCircle, s,
						       sizeof(s) - start))
				{
					mouseVariationInCircle = GetNum(s + start + strlen(configString_mouseVariationInCircle));
				}
				else if (StringMatches(configString_mouseStaticMoveThreshold, s,
						       sizeof(s) - start))
				{
					mouseStaticMoveThreshold = GetNum(s + start 
									  + strlen(configString_mouseStaticMoveThreshold));
				}
			}
		}
		fclose(f);
	}
}
