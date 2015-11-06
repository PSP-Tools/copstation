#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifndef WIN32
#include <unistd.h>
#else
#include <windows.h>
#endif
#include <zlib.h>

#include "iniparser.h"

#include "port.h"

#ifndef WIN32
typedef struct  __attribute__((packed))
#else
#pragma pack(1)
typedef struct
#endif
{
	unsigned char adr: 4;
	unsigned char control: 4;
	unsigned char tno;
	unsigned char point;
	unsigned char amin;
	unsigned char asec;
	unsigned char aframe;
	unsigned char zero;
	unsigned char pmin;
	unsigned char psec;
	unsigned char pframe;
}
tocentry;
#ifdef WIN32
#pragma pack()
#endif

unsigned char bcd(unsigned char value)
{
	unsigned int i;
	unsigned char result = 0;

	for (i = 0; value; i++)
	{
		result += (value % 10) * (int)pow(16, i);
		value = value / 10;
	}

	return result;
}

//Tinnus
void *create_toc(char *isoname, int *size)
{
	FILE *f;
	dictionary *ini;
	tocentry *entries;
	char *ccdname;
	char entryname[64];
	int length, count, i;
	unsigned char point;
	
	length = strlen(isoname);
	ccdname = (char *)malloc((length + 1) * sizeof(char));
	memset(ccdname, 0, (length + 1) * sizeof(char));
	strncpy(ccdname, isoname, length);
	
	ccdname[length - 3] = 'c';
	ccdname[length - 2] = 'c';
	ccdname[length - 1] = 'd';
	
	printf("Checking if %s exists...\n", ccdname);

	f = fopen(ccdname, "rb");

	if (f == NULL)
	{
		free(ccdname);
		return NULL;
	}
	
	fclose(f);
	
	printf("  Using %s for toc\n", ccdname);
	
	ini = iniparser_load(ccdname);
	count = iniparser_getint(ini, "Disc:TocEntries", -1);
	
	if (count == -1)
	{
		printf("Failed to get TOC count from CCD, are you sure it's a valid CCD file?\n");
		return NULL;
	}
	
	entries = (tocentry *)malloc(sizeof(tocentry) * count);
	
	for (i = 0; i < count; i++)
	{
		snprintf(entryname, 64, "Entry %d:Control", i);
		entries[i].control  = iniparser_getint(ini, entryname, -1) & 0xF;
		
		snprintf(entryname, 64, "Entry %d:ADR", i);
		entries[i].adr  = iniparser_getint(ini, entryname, -1) & 0xF;
		
		snprintf(entryname, 64, "Entry %d:TrackNo", i);
		entries[i].tno  = iniparser_getint(ini, entryname, -1);

		snprintf(entryname, 64, "Entry %d:Point", i);
		
		point = (unsigned char)iniparser_getint(ini, entryname, -1);
		
		if (point > 0x99)
		{
			entries[i].point = point;
		}
		else
		{
			entries[i].point = bcd(point);
		}
		
		snprintf(entryname, 64, "Entry %d:AMin", i);
		entries[i].amin  = bcd ((unsigned char)iniparser_getint(ini, entryname, -1));
		
		snprintf(entryname, 64, "Entry %d:ASec", i);
		entries[i].asec  = bcd ((unsigned char)iniparser_getint(ini, entryname, -1));
		
		snprintf(entryname, 64, "Entry %d:AFrame", i);
		entries[i].aframe  = bcd ((unsigned char)iniparser_getint(ini, entryname, -1));
		
		snprintf(entryname, 64, "Entry %d:Zero", i);
		entries[i].zero  = (unsigned char)iniparser_getint(ini, entryname, -1);
		
		snprintf(entryname, 64, "Entry %d:PMin", i);
		entries[i].pmin  = bcd ((unsigned char)iniparser_getint(ini, entryname, -1));
		
		snprintf(entryname, 64, "Entry %d:PSec", i);
		
		entries[i].psec = (unsigned char)iniparser_getint(ini, entryname, -1);
		
		if (point != 0xA0)
		{
			entries[i].psec = bcd(entries[i].psec);
		}
		
		snprintf(entryname, 64, "Entry %d:PFrame", i);
		entries[i].pframe = bcd ((unsigned char)iniparser_getint(ini, entryname, -1));
	}
	
	(*size) = sizeof(tocentry) * count;
	
	free(ccdname);
	iniparser_freedict(ini);
	
	return entries;
}
