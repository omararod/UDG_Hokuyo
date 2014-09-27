#ifndef UDG_HOKUYO_H
#define UDG_HOKUYO_H

#define DEFAULT_READ_DELAY 25
#define HOKUYO_BUFFER_SIZE 4096
#define HOKUYO_FIRST_MEASUREMENT_POINT 0
#define HOKUYO_MIDLE_MEASUREMENT_POINT 540
#define HOKUYO_LAST_MEASUREMENT_POINT 1079

#include <stdio.h>
#include <math.h>
#ifdef __linux
typedef int HANDLE;
#include <unistd.h>
#include <string.h>
#define READ_DELAY() usleep(readDelayMs * 1000)
#else
#include <Windows.h>
#define READ_DELAY() Sleep(readDelayMs)
#endif


#define SIMPLE_COMMAND(x) WriteToSerialPort(buffer,x);\
	READ_DELAY();\
	int bytesRead;\
	bytesRead = ReadFromSerialPort(buffer);\
	buffer[bytesRead] = '\0';\
	memcpy(response,buffer,bytesRead+1);\
	if(bytesRead > 0)\
		return true;\
	else\
		return false;\

enum class HokuyoEncoding {TWO_CHARACTER, THREE_CHARACTER};



class Hokuyo
{
public:
	HANDLE serialPortDescriptor;
	int readDelayMs;
	int bufferSize;
	//function prototypes
	Hokuyo();
	Hokuyo(HANDLE portDescriptor);
	~Hokuyo();
	bool GetData(HokuyoEncoding encoding, unsigned short startingStep,
		unsigned short endStep, unsigned short clusterCount,unsigned short scanInterval, 
		unsigned short nScans, unsigned char* response); //A.K.A "MDMS-Command"
	bool SwitchLaserOn(unsigned char* response); //A.K.A "BM-Command"
	bool SwitchLaserOff(unsigned char* response); //A.K.A "QT-Command"
	bool Reset(unsigned char* response); //A.K.A "RS-Command"
	bool GetVersionDetails(unsigned char* response); //A.K.A "VV-Command"
	bool GetSpecs(unsigned char* response); //A.K.A "PP-Command"
	bool GetRunningState(unsigned char* response); //A.K.A "II-Command"
	


private:
	bool WriteToSerialPort(unsigned char* buffer, int bytesToWrite);
	int ReadFromSerialPort(unsigned char* buffer);
	void NumberToChar(unsigned short number, int nCharacters,unsigned char string[]);

};

class LaserData
{
public:
	int firstStep;
	int lastStep;
	HokuyoEncoding encoding;
	int data[HOKUYO_LAST_MEASUREMENT_POINT];

	LaserData();
	bool SetData(unsigned char* buffer);
	int GetReadingsCount();

private:
	bool CharToNumber(unsigned char* string,int nDigits,int* result);
	


	
};

#endif
