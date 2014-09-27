#include <stdio.h>
#ifdef __linux

#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

#else
#include <Windows.h>
#endif
#include "UDG_Hokuyo.h"

#ifdef __linux

int OpenSerialPort(const char* device)
{
	int fd = open(device, O_RDWR | O_NOCTTY);
	struct termios options;
  tcgetattr(fd, &options);
  options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  options.c_oflag &= ~(ONLCR | OCRNL);
  tcsetattr(fd, TCSANOW, &options);
  return fd;
}

#else
HANDLE OpenSerialPort(LPWSTR name)
{
	HANDLE portDescriptor;
	DCB deviceControlBlock;
	portDescriptor = CreateFile(name,
				GENERIC_READ|GENERIC_WRITE,
				0,0,OPEN_EXISTING,0,0);
	if(portDescriptor == INVALID_HANDLE_VALUE)
	{
		printf("Failed opening %ls\n", name);
		return INVALID_HANDLE_VALUE;
	}

	FillMemory(&deviceControlBlock,sizeof(DCB),0);
	deviceControlBlock.DCBlength = sizeof(DCB);
	if(BuildCommDCB(L"57600,n,8,1",&deviceControlBlock))
	{
		printf("Successfully configured DCB!\n");
		
	}
	else
	{
		printf("Something went wrong while building the DCB\n");
		return INVALID_HANDLE_VALUE;
	}

	if (!SetCommState(portDescriptor,&deviceControlBlock))
	{
		printf("Error configuring port\n");
		return INVALID_HANDLE_VALUE;
	}
	else
	{
		printf("Serial port ready\n");
	}
		
	return portDescriptor;
}
#endif

int main(int nargs, char* args[])
{
#ifdef __linux
char* portName = "/dev/ttyACM0";
#else
	wchar_t *portName = L"COM8";
#endif
	HANDLE port = OpenSerialPort(portName);
	
	
	Hokuyo laser(port);
	unsigned char status[HOKUYO_BUFFER_SIZE];

	/*if(laser.GetRunningState(status))
		printf("%s\n",status);
	if(laser.GetSpecs(status))
		printf("%s\n",status);
	if(laser.GetVersionDetails(status))
		printf("%s\n",status);
	if(laser.Reset(status))
		printf("%s\n",status);
	if(laser.SwitchLaserOff(status))
		printf("%s\n",status);
	if(laser.SwitchLaserOn(status))
		printf("%s\n",status);*/
	
	laser.GetData(HokuyoEncoding::THREE_CHARACTER,HOKUYO_FIRST_MEASUREMENT_POINT,HOKUYO_LAST_MEASUREMENT_POINT-1,0,0,1,status);
	//printf("%s\n",status);
	LaserData data;
	data.SetData(status);
	for(int i = data.firstStep; i <= data.lastStep;i++)
		printf("%i ",data.data[i]);

	
	

}
