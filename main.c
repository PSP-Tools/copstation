#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#ifndef WIN32
#include <unistd.h>
#else
#include <windows.h>
#endif
#include <zlib.h>

#include "data.h"
#include "logo.h"
#include "toc.h"

int getsize(FILE *f)
{
	int size;

	fseek(f, 0, SEEK_END);
	size = ftell(f);

	fseek(f, 0, SEEK_SET);
	return size;
}

void ErrorExit(char *fmt, ...)
{
	va_list list;
	char msg[256];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	printf(msg);
	exit(-1);
}

z_stream z;

int deflateCompress(void *inbuf, int insize, void *outbuf, int outsize, int level)
{
	int res;

	z.zalloc = Z_NULL;
	z.zfree  = Z_NULL;
	z.opaque = Z_NULL;

	if (deflateInit2(&z, level , Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY) != Z_OK)
		return -1;

	z.next_out  = (Bytef *)outbuf;
	z.avail_out = outsize;
	z.next_in   = (Bytef *)inbuf;
	z.avail_in  = insize;

	if (deflate(&z, Z_FINISH) != Z_STREAM_END)
	{
		return -1;
	}

	res = outsize - z.avail_out;

	if (deflateEnd(&z) != Z_OK)
		return -1;

	return res;
}

int inflateDecompress(void *inbuf, int insize, void *outbuf, int outsize)
{
	z.zalloc = Z_NULL;
	z.zfree  = Z_NULL;
	z.opaque = Z_NULL;

	if (inflateInit2(&z, -15) != Z_OK)
		return -1;

	z.next_in   = (Bytef *)inbuf;
	z.avail_in  = insize;
	z.next_out  = (Bytef *)outbuf;
	z.avail_out = outsize;

	if (inflate(&z, Z_FINISH) != Z_STREAM_END)
      return -1;

	if (inflateEnd(&z) != Z_OK)
		return -1;

	return 0;
}

#ifndef WIN32
typedef struct  __attribute__((packed))
#else
#pragma pack(1)
typedef struct
#endif
{
	unsigned int signature;
	unsigned int version;
	unsigned int fields_table_offs;
	unsigned int values_table_offs;
	int nitems;
} SFOHeader;
#ifdef WIN32
#pragma pack()
#endif

#ifndef WIN32
typedef struct  __attribute__((packed))
#else
#pragma pack(1)
typedef struct
#endif
{
	unsigned short field_offs;
	unsigned char  unk;
	unsigned char  type; // 0x2 -> string, 0x4 -> number
	unsigned int length;
	unsigned int size;
	unsigned short val_offs;
	unsigned short unk4;
} SFODir;
#ifdef WIN32
#pragma pack()
#endif

void SetSFOTitle(char *sfo, char *title)
{
	SFOHeader *header = (SFOHeader *)sfo;
	SFODir *entries = (SFODir *)(sfo+0x14);
	int i;

	for (i = 0; i < header->nitems; i++)
	{
		if (strcmp(sfo+header->fields_table_offs+entries[i].field_offs, "TITLE") == 0)
		{
			strncpy(sfo+header->values_table_offs+entries[i].val_offs, title, entries[i].size);

			if (strlen(title)+1 > entries[i].size)
			{
				entries[i].length = entries[i].size;
			}
			else
			{
				entries[i].length = strlen(title)+1;
			}
		}
	}
}

char buffer[1*1048576];
char buffer2[0x9300];

//I'm Rick James - Bitch! ArHArHArH! :D
int logo = 0, pic0 = 0, pic1 = 0, icon0 = 0, icon1 = 0, snd = 0, toc = 0, prx = 0;
int sfo_size, logo_size, pic0_size, pic1_size, icon0_size, icon1_size, snd_size, toc_size, prx_size;
int start_dat = 0;
unsigned int* base_header = (unsigned int*)(&base_header_c);
unsigned int header[0x28/4];
unsigned int dummy[6];

#ifndef WIN32
typedef struct __attribute__((packed))
#else
#pragma pack(1)
typedef struct
#endif
{
	unsigned int offset;
	unsigned int length;
	unsigned int dummy[6];
} IsoIndex;
#ifdef WIN32
#pragma pack()
#endif

void convert(char *input, char *output, char *title, char *code, int complevel)
{
	FILE *in, *out, *t;
	int i, offset, isosize, isorealsize, x;
	int index_offset, p1_offset, p2_offset, end_offset;
	IsoIndex *indexes;

	void* tocptr;

	in = fopen (input, "rb");
	if (!in)
	{
		ErrorExit("Unable to Open Input PSX ISO File [%s]\n", input);
	}

	isosize = getsize(in);
	isorealsize = isosize;

	if ((isosize % 0x9300) != 0)
	{
		isosize = isosize + (0x9300 - (isosize%0x9300));
	}

	//printf("isosize, isorealsize %08X  %08X\n", isosize, isorealsize);

	out = fopen(output, "wb");
	if (!out)
	{
		ErrorExit("Unable to Create Output PSX EBOOT File [%s]\n", output);
	}

	printf("Writing Header...\n");

	sfo_size = base_header[3] - base_header[2];

	t = fopen("LOGO.PNG", "rb");
	if (t)
	{
		logo_size = getsize(t);
		logo = 1;
		fclose(t);
	}

	t = fopen("ICON0.PNG", "rb");
	if (t)
	{
		icon0_size = getsize(t);
		icon0 = 1;
		fclose(t);
	}
	else
	{
		icon0_size = 0;
	}

	t = fopen("ICON1.PMF", "rb");
	if (t)
	{
		icon1_size = getsize(t);
		icon1 = 1;
		fclose(t);
	}
	else
	{
		icon1_size = 0;
	}

	t = fopen("PIC0.PNG", "rb");
	if (t)
	{
		pic0_size = getsize(t);
		pic0 = 1;
		fclose(t);
	}
	else
	{
		pic0_size = 0; //base_header[6] - base_header[5];
	}

	t = fopen("PIC1.PNG", "rb");
	if (t)
	{
		pic1_size = getsize(t);
		pic1 = 1;
		fclose(t);
	}
	else
	{
		pic1_size = 0; // base_header[7] - base_header[6];
	}

	t = fopen("SND0.AT3", "rb");
	if (t)
	{
		snd_size = getsize(t);
		snd = 1;
		fclose(t);
	}
	else
	{
		snd = 0;
	}

	t = fopen("DATA.PSP", "rb");
	if (t)
	{
		prx_size = getsize(t);
		prx = 1;
		fclose(t);
	}
	else
	{
		prx_size = sizeof(datapspbody);
	}

	t = fopen("ISO.TOC", "rb");
	if (t)
	{
		toc_size = getsize(t);
		toc = 1;
		fclose(t);
	}
	else if ((tocptr = create_toc(input, &toc_size)) != NULL)
	{
		toc = 2;
	}
	else
	{
		toc = 0;
	}

	int curoffs = 0x28;

	header[0] = 0x50425000;
	header[1] = 0x10000;

	header[2] = curoffs;

	curoffs += sfo_size;
	header[3] = curoffs;

	curoffs += icon0_size;
	header[4] = curoffs;

	curoffs += icon1_size;
	header[5] = curoffs;

	curoffs += pic0_size;
	header[6] = curoffs;

	curoffs += pic1_size;
	header[7] = curoffs;

	curoffs += snd_size;
	header[8] = curoffs;

	x = header[8] + prx_size;

	if ((x % 0x10000) != 0)
	{
		x = x + (0x10000 - (x % 0x10000));
	}

	header[9] = x;

	fwrite(header, 1, 0x28, out);

	printf("Applying EBOOT Name Patch on PARAM.SFO [%s]...\n",title);

	SetSFOTitle(parambody, title);
	strcpy(parambody+0x108, code);

	printf("Writing PARAM.SFO...\n");

	fwrite(parambody, 1, sfo_size, out);

	if (icon0)
	{
		printf("Writing ICON0.PNG [%d]...\n",icon0_size);

		t = fopen("ICON0.PNG", "rb");
		fread(buffer, 1, icon0_size, t);
		fwrite(buffer, 1, icon0_size, out);
		fclose(t);
	}

	if (icon1)
	{
		printf("Writing ICON1.PMF [%d]...\n",icon1_size);

		t = fopen("ICON1.PMF", "rb");
		fread(buffer, 1, icon1_size, t);
		fwrite(buffer, 1, icon1_size, out);
		fclose(t);
	}

	if (pic0)
	{
		printf("Writing PIC0.PNG [%d]...\n",pic0_size);

		t = fopen("PIC0.PNG", "rb");
		fread(buffer, 1, pic0_size, t);
		fwrite(buffer, 1, pic0_size, out);
		fclose(t);
	}

	if (pic1)
	{
		printf("Writing PIC1.PNG [%d]...\n",pic1_size);

		t = fopen("PIC1.PNG", "rb");
		fread(buffer, 1, pic1_size, t);
		fwrite(buffer, 1, pic1_size, out);
		fclose(t);
	}

	if (snd)
	{
		printf("Writing SND0.AT3 [%d]...\n",snd_size);

		t = fopen("SND0.AT3", "rb");
		fread(buffer, 1, snd_size, t);
		fwrite(buffer, 1, snd_size, out);
		fclose(t);
	}

	printf("Writing Main-Executeable DATA.PSP...\n");

	if (prx)
	{
      printf("  Found External DATA.PSP - Including External DATA.PSP\n");
		t = fopen("DATA.PSP", "rb");
		fread(buffer, 1, prx_size, t);
		fwrite(buffer, 1, prx_size, out);
		fclose(t);
	}
   else
   {
      printf("  No External DATA.PSP Found - Including Internal DarkAlex DATA.PSP\n");
	   fwrite(datapspbody, 1, prx_size, out);
   }

	offset = ftell(out);

	for (i = 0; i < header[9]-offset; i++)
	{
		fputc(0, out);
	}

	printf("Writing PSX ISO Header...\n");

	fwrite("PSISOIMG0000", 1, 12, out);

	p1_offset = ftell(out);

	x = isosize + 0x100000;
	fwrite(&x, 1, 4, out);

	x = 0;
	for (i = 0; i < 0xFC; i++)
	{
		fwrite(&x, 1, 4, out);
	}

	memcpy(data1+1, code, 4);
	memcpy(data1+6, code+4, 5);

	//if (toc)
	if (toc == 1)
	{
		printf("Copying TOC to ISO Header...\n");

		t = fopen("ISO.TOC", "rb");
		fread(buffer, 1, toc_size, t);
		memcpy(data1+1024, buffer, toc_size);
		fclose(t);
	}
	else if (toc == 2)
	{
		memcpy(data1+1024, tocptr, toc_size);
		free(tocptr);
	}

	fwrite(data1, 1, sizeof(data1), out);

	p2_offset = ftell(out);
	x = isosize + 0x100000 + 0x2d31;
	fwrite(&x, 1, 4, out);

	strcpy((char *)data2+8, title);
	fwrite(data2, 1, sizeof(data2), out);

	index_offset = ftell(out);

	printf("Writing Indexes...\n");

	memset(dummy, 0, sizeof(dummy));

	offset = 0;

	if (complevel == 0)
	{
		x = 0x9300;
	}
	else
	{
		x = 0;
	}

	for (i = 0; i < isosize / 0x9300; i++)
	{
		fwrite(&offset, 1, 4, out);
		fwrite(&x, 1, 4, out);
		fwrite(dummy, 1, sizeof(dummy), out);

		if (complevel == 0)
			offset += 0x9300;
	}

	offset = ftell(out);

	for (i = 0; i < (header[9]+0x100000)-offset; i++)
	{
		fputc(0, out);
	}

	printf("Writing PSX CD Dump...\n");
   printf("  Processing... [%04dMB|0000MB]",(isorealsize/(1024*1024)));

   int written=0;

	if (complevel == 0)
	{
		while ((x = fread(buffer, 1, 1048576, in)) > 0)
		{
			fwrite(buffer, 1, x, out);
         written+=x;
         printf("\010\010\010\010\010\010\010");
         printf("%04dMB]",((isorealsize-written)/(1024*1024)));
		}

		for (i = 0; i < (isosize-isorealsize); i++)
		{
			fputc(0, out);
		}

      printf("\n");
	}
	else
	{
		indexes = (IsoIndex *)malloc(sizeof(IsoIndex) * (isosize/0x9300));

		if (!indexes)
		{
			fclose(in);
			fclose(out);

			ErrorExit("Failed in allocating Memory for Indexes!\n");
		}

		i = 0;
      int read=0;
		offset = 0;

		while ((x = fread(buffer2, 1, 0x9300, in)) > 0)
		{
			if (x < 0x9300)
			{
				memset(buffer2+x, 0, 0x9300-x);
			}

         read=x;

			x = deflateCompress(buffer2, 0x9300, buffer, sizeof(buffer), complevel);

			if (x < 0)
			{
				fclose(in);
				fclose(out);
				free(indexes);

				ErrorExit("Error in Compression!\n");
			}

			memset(&indexes[i], 0, sizeof(IsoIndex));

			indexes[i].offset = offset;

			if (x >= 0x9300) /* Block didn't compress */
			{
				indexes[i].length = 0x9300;
				fwrite(buffer2, 1, 0x9300, out);
				offset += 0x9300;
			}
			else
			{
				indexes[i].length = x;
				fwrite(buffer, 1, x, out);
				offset += x;
			}

         written+=read;
         printf("\010\010\010\010\010\010\010");
         printf("%04dMB]",((isosize-written)/(1024*1024)));
			i++;
		}

      printf("\010\010\010\010\010\010\010");
      printf(" DONE ]\n");

		if (i != (isosize/0x9300))
		{
			fclose(in);
			fclose(out);
			free(indexes);

			ErrorExit("Some Error happened...\n");
		}

		x = ftell(out);

		if ((x % 0x10) != 0)
		{
			end_offset = x + (0x10 - (x % 0x10));

			for (i = 0; i < (end_offset-x); i++)
			{
				fputc('0', out);
			}
		}
		else
		{
			end_offset = x;
		}

		end_offset -= header[9];
	}

	printf("Writing STARTDAT Header...\n");
	fwrite(startdatheader, 1, sizeof(startdatheader), out);

	if (logo)
	{
		printf("Writing LOGO.PNG [%d]...\n",logo_size);

		t = fopen("LOGO.PNG", "rb");
		fread(buffer, 1, logo_size, t);
		fwrite(buffer, 1, logo_size, out);
		fclose(t);
	}
	else
	{
		printf("Writing P.O.P.S. Standard Logo...\n");

		fwrite(logo_buffer, 1, sizeof(logo_buffer), out);
	}

	printf("Writing STARTDAT Footer...\n");

	fwrite(startdatfooter, 1, sizeof(startdatfooter), out);

	if (complevel != 0)
	{
		printf("Updating Compressed Indexes...\n");

		fseek(out, p1_offset, SEEK_SET);
		fwrite(&end_offset, 1, 4, out);

		end_offset += 0x2d31;
		fseek(out, p2_offset, SEEK_SET);
		fwrite(&end_offset, 1, 4, out);

		fseek(out, index_offset, SEEK_SET);
		fwrite(indexes, 1, sizeof(IsoIndex) * (isosize/0x9300), out);
	}

	fclose(in);
	fclose(out);
}

#define N_GAME_CODES	12

char *gamecodes[N_GAME_CODES] =
{
	"SCUS",
	"SLUS",
	"SLES",
	"SCES",
	"SCED",
	"SLPS",
	"SLPM",
	"SCPS",
	"SLED",
	"SLPS",
	"SIPS",
	"ESPM"
};

//COLDBIRDIE @ WORK
int FindPSISOFlag(char * eboot)
{
   FILE * fp = fopen(eboot,"rb");

   if(!fp)
      ErrorExit("Failed to Open [%s] for a Flag-Scan.\n",eboot);

   int offset=-1;
   char buffer[12];

   while(strcmp(buffer,"PSISOIMG0000"))
   {
      if((fread(buffer, 1, 12, fp))<12) return 0;
      buffer[12]='\0';
      fseek(fp,-11L,SEEK_CUR);
      offset++;
   }

   fclose(fp);

   return offset;
}

int GetISOSize(char * eboot, int offset)
{
   int indexoffset=(offset+0x4000);
   int isooffset=(offset+0x100000);

   FILE * fp = fopen(eboot,"rb");

   if(!fp)
      ErrorExit("Failed to Open [%s] for a ISO-Size Calculation.\n",eboot);

   int size=0;
   int pointervalue[8];

   fseek(fp, indexoffset+32, SEEK_SET);
   fread(&pointervalue, 1, (8*4), fp);
   fseek(fp, isooffset+pointervalue[0], SEEK_SET);

   if(pointervalue[1]==0x9300)
   {
      //Uncompressed
      fread(buffer, 1, pointervalue[1], fp);
      size=(buffer[104] + (buffer[105] << 8) + (buffer[106] << 16) + (buffer[107] << 24)) * 0x0930;
   }
   else
   {
      //Compressed
      fread(buffer, 1, pointervalue[1], fp);
      inflateDecompress(buffer, pointervalue[1], buffer2, 0x9300);
      size=(buffer2[104] + (buffer2[105] << 8) + (buffer2[106] << 16) + (buffer2[107] << 24)) * 0x0930;
   }

   fclose(fp);

   return size;
}

/*
 * And Again Coldbird did it... :)
 * This Nice Function Extracts a EBOOT, not minding if compressed or not to a PSX ISO...
 * The Filesize of the Extracted ISO in 90% of the Cases aint 100% identical to the real ISO...
 * This is due to the Fact that the Original Filesize gets lost on the ISO -> EBOOT Conversion~
 * So we have to guess the Filesize by taking a Multiple of 0x9300 (The Blocksize for POPS ISO-Data)
 */
int ExtractISO(char * eboot, char * output)
{
   int offset;
   offset=FindPSISOFlag(eboot);

   if(!offset)
      ErrorExit("Failed to Locate PSISO Flag - Make sure your [%s] is a PSX-EBOOT!\n",eboot);

   int indexoffset=(offset+0x4000);
   int isooffset=(offset+0x100000);

   int i=0;
   int pointervalue[8];

   int compressed=0;
   int write=0;
   int isosize=GetISOSize("EBOOT.PBP",offset);

   FILE * fp = fopen(eboot,"rb");
   FILE * out = fopen(output,"wb");
   if((!fp)||(!out)) return 0;

   printf("  Processing... [%04dMB|0000MB]",(isosize/(1024*1024)));

   do
   {
      fseek(fp, (indexoffset+(i*32)), SEEK_SET);
      fread(&pointervalue, 1, (8*4), fp);

      if((i==0)&&(pointervalue[1]<0x9300)) compressed=1;

      fseek(fp, (isooffset+pointervalue[0]), SEEK_SET);
      fread(buffer, 1, pointervalue[1], fp);

      if((isosize-pointervalue[1])<0)
      {
         write=isosize;
      }
      else
      {
         write=0x9300;
      }

      if(compressed)
      {
         inflateDecompress(buffer, pointervalue[1], buffer2, 0x9300);
         fwrite(buffer2, 1, write, out);
         isosize-=write;
         printf("\010\010\010\010\010\010\010");
         printf("%04dMB]",(isosize/(1024*1024)));
      }
      else
      {
         fwrite(buffer, 1, write, out);
         isosize-=write;
         printf("\010\010\010\010\010\010\010");
         //if(isosize<0) isosize=0;
         printf("%04dMB]",(isosize/(1024*1024)));
      }

      i++;
   } while(pointervalue[1]);

   printf("\010\010\010\010\010\010\010");
   printf(" DONE ]\n");

   if(compressed)
   {
      return 1;
   }
   else
   {
      return 0;
   }
}

char * GetGID(char * filename, char * output)
{
   FILE * file = fopen(filename, "r");
   if(!file) ErrorExit("Couldn't Open PSX Game [%s] for GameID Scan!\n",filename);

   int i;
   int x;

   char buffer[13];

	while ((x = fread(buffer, 1, 13, file)) == 13)
	{
   	for (i = 0; i < N_GAME_CODES; i++)
   	{
   		if ((strncmp(buffer, gamecodes[i], 4) == 0)&&
             (buffer[4]=='_')&&(buffer[8]=='.')&&(buffer[11]==';')&&(buffer[12]=='1'))
   			break;
   	}
      if(i!=N_GAME_CODES)
      {
         output[0]=gamecodes[i][0];
         output[1]=gamecodes[i][1];
         output[2]=gamecodes[i][2];
         output[3]=gamecodes[i][3];
         output[4]=buffer[5];
         output[5]=buffer[6];
         output[6]=buffer[7];
         output[7]=buffer[9];
         output[8]=buffer[10];
         output[9]='\0';
         break;
      }
      fseek(file,-12L,SEEK_CUR);
   }

   fclose(file);

   if(x!=13) return (char*)(0);
   else return output;
}

int main(int argc, char *argv[])
{
	int i;

   printf("\n");
	printf(" .|'''',                        ||              ||                         \n");
	printf(" ||                             ||              ||     ''                  \n");
	printf(" ||      .|''|, '||''|, ('''' ''||''   '''|.  ''||''   ||  .|''|, `||''|,  \n");
	printf(" ||      ||  ||  ||  ||  `'')   ||    .|''||    ||     ||  ||  ||  ||  ||  \n");
	printf(" `|....' `|..|'  ||..|' `...'   `|..' `|..||.   `|..' .||. `|..|' .||  ||. \n");
	printf("                 ||                                                        \n");
	printf("                .||              Coldbird Popstation Mod V2.21             \n");
	printf("\n");
   printf("          Coded and Maintained by Coldbird [vanburace@gmail.com]\n");
   printf("\n");
	printf("Credits go to... [Dark_Alex] - for the Basic Idea and Conversion Method\n");
	printf("                 [Tinnus]    - for the CDDA Fix - AKA TOC Converter\n");
	printf("                 [Coldbird]  - for EVERYTHING else\n");
   printf("\n");
   printf("          Now 3.03 OE-C (and newer) only! If you want to use this\n");
   printf("          with a lower Firmware Version, extract the DATA.PSP from\n");
   printf("          a BASE.PBP of your Choice and place it in the same Place\n");
   printf("          as your PSX Game / Images.\n");
   printf("\n");
   printf("          Thanks to Dark_Alex' 3.03 OE-C, no more KEYS.BIN is needed.\n");
   printf("\n");

	if ((argc != 5)&&(argc != 3))
	{
      help:
      fprintf(stderr,"Invalid Number of Arguments.\n");
      printf("For Packing        : %s title gamecode compressionlevel inputfile.iso\n",argv[0]);
		ErrorExit("For ISO-Extraction : %s -iso outputfile.iso\n", argv[0]);
	}

   if(argc==3)
   {
      if(strcmp(argv[1],"-iso")) goto help;

      printf("Attempting to Extract PSX ISO [%s] out of EBOOT.PBP... Please Wait!\n",argv[2]);

      int exresult = ExtractISO("EBOOT.PBP",argv[2]);

      if(exresult)
      {
         printf("Extracted ISO from [EBOOT.PBP] to [%s]\n",argv[2]);
         printf("The ISO File was compressed using zlib\n");
      }
      else
      {
         printf("Extracted ISO from [EBOOT.PBP] to [%s]\n",argv[2]);
         printf("The ISO File was uncompressed\n");
      }
   }

   char gameid[9];

   if(argc==5)
   {
      if(strcmp(argv[2],"AUTO"))
      {
         strcpy(gameid,argv[2]);

      	if (strlen(argv[2]) != 9)
      	{
      		ErrorExit("Invalid game code.\n");
      	}

      	for (i = 0; i < N_GAME_CODES; i++)
      	{
      		if (strncmp(argv[2], gamecodes[i], 4) == 0)
      			break;
      	}

      	if (i == N_GAME_CODES)
      	{
      		ErrorExit("Invalid game code.\n");
      	}

      	for (i = 4; i < 9; i++)
      	{
      		if (argv[2][i] < '0' || argv[2][i] > '9')
      		{
      			ErrorExit("Invalid game code.\n");
      		}
      	}
      }
      else
      {
         if(!GetGID(argv[4],gameid)) ErrorExit("Unable to find GameID inside the ISO...\nThis could be the Case if you are using PSX Game Beta Versions or Homebrew PSX Games.\nPlease specify a ID yourself!\n");
         else printf("Automatically Extracted GameID [%s]\n",gameid);
      }

   	if (strlen(argv[3]) != 1)
   	{
   		ErrorExit("Invalid compression level.\n");
   	}

   	if (argv[3][0] < '0' || argv[3][0] > '9')
   	{
   		ErrorExit("Invalid compression level.\n");
   	}

   	convert(argv[4], "EBOOT.PBP", argv[1], gameid, argv[3][0]-'0');
   }

	printf("Done.\n");
	return 0;
}

