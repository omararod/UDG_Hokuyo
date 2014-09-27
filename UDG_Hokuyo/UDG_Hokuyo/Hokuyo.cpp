
#include "UDG_Hokuyo.h"
Hokuyo::Hokuyo(HANDLE portDescriptor)
{
	serialPortDescriptor = portDescriptor;
	readDelayMs = DEFAULT_READ_DELAY;
	
}

Hokuyo::Hokuyo()
{
	serialPortDescriptor = 0;
	readDelayMs = DEFAULT_READ_DELAY;
	
}

Hokuyo::~Hokuyo()
{
#ifdef __linux
	//linux operations
#else
	CloseHandle(serialPortDescriptor);
#endif
}
bool Hokuyo::WriteToSerialPort(unsigned char* buffer,int bytesToWrite)
{
	if(!serialPortDescriptor)
		return false;
#ifdef __linux
	return (bool)write(serialPortDescriptor,buffer,bytesToWrite);
#else
	DWORD bytesWritten;
	return WriteFile(serialPortDescriptor,buffer,bytesToWrite,&bytesWritten,NULL);
#endif
}
int Hokuyo::ReadFromSerialPort(unsigned char* buffer)
{
	if(!serialPortDescriptor)
		return -1;
#ifdef __linux
	return read(serialPortDescriptor,buffer,HOKUYO_BUFFER_SIZE);
#else
	DWORD bytesRead;
	ReadFile(serialPortDescriptor,buffer,HOKUYO_BUFFER_SIZE,&bytesRead,NULL);
	return (int)bytesRead;
#endif
}
bool Hokuyo::GetData(HokuyoEncoding encoding, unsigned short startingStep, unsigned short endStep,
							 unsigned short clusterCount,unsigned short scanInterval, unsigned short nScans,
							 unsigned char* response) //A.K.A "MDMS-Command"
{
	unsigned char buffer[HOKUYO_BUFFER_SIZE];
	buffer[0] = 'M';
	if(encoding == HokuyoEncoding::THREE_CHARACTER)
		buffer[1] = 'D';
	if(encoding == HokuyoEncoding::TWO_CHARACTER)
		buffer[1] = 'S';
	if(startingStep > 9999 || endStep > HOKUYO_LAST_MEASUREMENT_POINT ||clusterCount > 99 || scanInterval > 9||nScans > 99)
		return false;
	//convert starting step to char string. 4 bytes
	NumberToChar(startingStep,4,&buffer[2]);
	//convert end step to char string. 4 bytes
	NumberToChar(endStep,4,&buffer[6]);
	//convert cluster count to char string. 2 bytes
	NumberToChar(clusterCount,2,&buffer[10]);
	//convert scan interval to char string. 1 bytes
	NumberToChar(scanInterval,1,&buffer[12]);
	//convert number of scans to char string. 2 bytes
	NumberToChar(nScans,2,&buffer[13]);
	buffer[15] = '\n';

	WriteToSerialPort(buffer,16);
	READ_DELAY();
	int bytesRead=0;
	bytesRead += ReadFromSerialPort(buffer);
	READ_DELAY();
	bytesRead += ReadFromSerialPort(buffer+bytesRead);
	buffer[bytesRead] = '\0';
	memcpy(response,buffer,bytesRead+1);
	if(bytesRead > 0)
		return true;
	else
		return false;
}
bool Hokuyo::SwitchLaserOn(unsigned char* response)//A.K.A "BM-Command"
{
	
	unsigned char buffer[HOKUYO_BUFFER_SIZE];
	buffer[0] = 'B';
	buffer[1] = 'M';
	buffer[2] = '\n';

	SIMPLE_COMMAND(3);
}
bool Hokuyo::SwitchLaserOff(unsigned char* response)//A.K.A "QT-Command"
{
	
	unsigned char buffer[HOKUYO_BUFFER_SIZE];
	buffer[0] = 'Q';
	buffer[1] = 'T';
	buffer[2] = '\n';

	SIMPLE_COMMAND(3);
}
bool Hokuyo::Reset(unsigned char* response)//A.K.A "RS-Command"
{
	
	unsigned char buffer[HOKUYO_BUFFER_SIZE];
	buffer[0] = 'R';
	buffer[1] = 'S';
	buffer[2] = '\n';

	SIMPLE_COMMAND(3);
}
bool Hokuyo::GetVersionDetails(unsigned char* response)//A.K.A "VV-Command"
{
	
	unsigned char buffer[HOKUYO_BUFFER_SIZE];
	buffer[0] = 'V';
	buffer[1] = 'V';
	buffer[2] = '\n';

	SIMPLE_COMMAND(3);
}
bool Hokuyo::GetSpecs(unsigned char* response) //A.K.A "PP-Command"
{
	unsigned char buffer[HOKUYO_BUFFER_SIZE];
	buffer[0] = 'P';
	buffer[1] = 'P';
	buffer[2] = '\n';

	SIMPLE_COMMAND(3);
}
bool Hokuyo::GetRunningState(unsigned char* response) //A.K.A "II-Command"
{
	unsigned char buffer[HOKUYO_BUFFER_SIZE];
	buffer[0] = 'I';
	buffer[1] = 'I';
	buffer[2] = '\n';

	SIMPLE_COMMAND(3);

}

void Hokuyo::NumberToChar(unsigned short number, int nDigits,unsigned char result[])
{
	
	int remaining = number;
	int aux;
	float power;
	for(int i = nDigits-1; i >= 0;i--)
	{
		power = pow(10.0f,i);
		aux = remaining/power; 
		result[(nDigits-1) - i] = aux + 48;//add 48 to get the ascii equivalent
		if(aux>0)
			remaining = remaining- (aux * power);
		
	}
	
}

LaserData::LaserData()
{
	//set all data to -1
	memset(data,-1,HOKUYO_LAST_MEASUREMENT_POINT*sizeof(int));
}
bool LaserData::SetData(unsigned char* buffer)
{
	int count=0;
	int bufferCount = 0;
	if(buffer[0] != 'M')
		return false;
	if(buffer[1] == 'D')
		encoding = HokuyoEncoding::THREE_CHARACTER;
	else if(buffer[1] = 'S')
		encoding = HokuyoEncoding::TWO_CHARACTER;
	else
			return false;

	//get first measurment step
	if(!CharToNumber(&buffer[2],4,&firstStep))
		return false;
	//get last measurement step
	if(!CharToNumber(&buffer[6],4,&lastStep))
		return false;

	int expectedReadings = lastStep - firstStep +1;
	int expectedBytes;
	if(encoding == HokuyoEncoding::THREE_CHARACTER)
		expectedBytes = expectedReadings * 3;
	if(encoding == HokuyoEncoding::TWO_CHARACTER)
		expectedBytes = expectedReadings * 2;
	unsigned char* auxBuffer = new unsigned char[expectedBytes];
	int remainingBytes = expectedBytes;
	
	//first data byte is stored at byte 47
	bufferCount = 47;
	//copy all data bytes to auxBuffer in order to obtain contiguous data bytes
	while(remainingBytes != 0)
	{
		//each reading contains at most 64 bytes of data
		if(remainingBytes > 64)
		{
			//each reading contains 64 bytes of data + 1 byte sum + LF = 66 bytes
			memcpy(&auxBuffer[count],&buffer[bufferCount],64);
			bufferCount+=66;
			remainingBytes -= 64;
			count+=64;

		}
		else
		{
			memcpy(&auxBuffer[count],&buffer[bufferCount],remainingBytes);
			bufferCount+=remainingBytes;
			remainingBytes -= remainingBytes;
			count+=remainingBytes;
		}
	}
	
	/*for(int i = 0; i <= expectedBytes; i++)
	{
		printf("%c",auxBuffer[i]);
	}*/
	for(int i = firstStep; i <= lastStep; i++)
	{
		if(encoding == HokuyoEncoding::THREE_CHARACTER)
		{
			data[i] = ((auxBuffer[(i-firstStep)*3]-0x30)<<12) + ((auxBuffer[(i-firstStep)*3+1]-0x30)<<6) + ((auxBuffer[(i-firstStep)*3+2]-0x30));
		}
		
		if(encoding == HokuyoEncoding::TWO_CHARACTER)
		{
			data[i] = ((auxBuffer[i*2]-0x30)<<6) + (auxBuffer[i*2+1]-0x30);
		}
	}

	
}

bool LaserData::CharToNumber(unsigned char* string,int nDigits,int* result)
{
	*result = 0;
	for(int  i = 0; i < nDigits; i++)
	{
		if(string[i] >= '0' && string[i]<='9')
		{
			*result += (string[i] - '0') * pow(10.0f,((nDigits - 1) -i));
		}
		else return false;
	}
	return true;
}
int LaserData::GetReadingsCount()
{
	return lastStep - firstStep + 1;
}
