// Chapter 3.
// RecordAccessTestDataGenerate.cpp : Generate random data to test performance of
//    RecordAccess   (Chapter 3)      Direct File Access
//    RecordAccessMM (Chapter 5)      Memory Mapped files
// This actually generates commands to the program to read or write records.

#include "Everything.h"
//#include <WINIOCTL.H>

#define STRING_SIZE 256
typedef struct _RECORD { /* File record structure */
	DWORD			ReferenceCount;  /* 0 meands an empty record */
	SYSTEMTIME		RecordCreationTime;
	SYSTEMTIME		RecordLastRefernceTime;
	SYSTEMTIME		RecordUpdateTime;
	TCHAR			DataString[STRING_SIZE];
} RECORD;
typedef struct _HEADER { /* File header descriptor */
	DWORD			NumRecords;
	DWORD			NumNonEmptyRecords;
} HEADER;

#define STRING_SIZE 256

int rimes_rand (unsigned int, unsigned int);
void rand_string (char *, unsigned int);

int main(unsigned int argc, char * argv[])
{
	char cdata[STRING_SIZE];
	unsigned int filesize = atoi(argv[2]);
    unsigned int ndata = atoi(argv[1]);

	cdata[STRING_SIZE-1] = 0;

	if (argc < 5) {
		printf ("Usage: RecordAccessData ndata filesize file command(r|w).\n");
		return 1;
	}
	FILE * file;
	
	if (0 != fopen_s (&file, argv[3], "w")) {
		printf ("Output file: %s, create failed.\n", argv[3]);
		return 2;
	}
	for (int i = 0; i < atoi(argv[1]); ++i) {
		unsigned int irec = rimes_rand(rand()*i*i, filesize);
		if (argv[4][0] == 'w') {
            fprintf (file, "w %d\n", irec);
            rand_string (cdata, STRING_SIZE-1);
		    fprintf (file, "%s\n", cdata);
		} else {
			irec = (irec+1) % filesize;
            fprintf (file, "r %d\n", irec);
		}

	}
	fprintf (file, "qu\n");

	fclose (file);
	return 0;
}

// pseudo random number generator, based on the rimes hash function
// This is too crude; we need a mutli-byte key
int rimes_rand(unsigned int iseed, unsigned int size)
{
    unsigned int hash = ~0u;
	char ascii[10];
	_itoa_s (iseed, ascii, 16);
    for (int i = 0; i < sizeof(ascii) && ascii[i] != 0; i++) hash = hash * 33 + ascii[i];
    return (size > 0) ? hash % size : hash;
}

static char characters[] = "abcdefghijklmnopqrstuvwxyzABCEFGHIJKLMNOPQURSTUVWXYS1234567890";
void rand_string (char *cdata, unsigned int size)
{
	size_t len = strlen (characters);
	for (unsigned int i = 0; i < size; ++i) {
		size_t cn = rand() % len;
        cdata[i] = characters[cn];
	}
    
	return;
}