#include<stdio.h>
#include<string.h>
#include<time.h>

#define MAX_MSG_LEN 1024

typedef unsigned int uint;

void processEventLog(char *fileName, uint lineNo, FILE *fp, const char *argList, ...){
	char sLogParamString[MAX_MSG_LEN]; 
	memset(sLogParamString,0x00,MAX_MSG_LEN);
	char sLogBuffer[MAX_MSG_LEN];
	memset(sLogBuffer,0x00,MAX_MSG_LEN);
	va_list vErrorList;
	uint iStrLen;

	time_t lTime;
	struct tm tm; 
	time(&lTime);
	localtime_r(&lTime, &tm);

	tm.tm_mon += 1;
	tm.tm_year += 1900;


	strcpy(sLogParamString,"[%4d-%02d-%02d %02d:%02d:%02d] [%s] [%d] {");
	iStrLen = sprintf(sLogBuffer,
		sLogParamString,
		tm.tm_year,
		tm.tm_mon,
		tm.tm_mday,
		tm.tm_hour,
		tm.tm_min,
		tm.tm_sec,
		fileName,
		lineNo
	);

	va_start(vErrorList, argList);
	iStrLen += vsprintf((sLogBuffer+iStrLen),argList,vErrorList);
	va_end(vErrorList);
	iStrLen += sprintf(sLogBuffer+iStrLen,"}");
	sLogBuffer[iStrLen]='\n';
	sLogBuffer[iStrLen+1]=NULL;
	
	fprintf(fp, sLogBuffer);
	fflush(fp);
}

