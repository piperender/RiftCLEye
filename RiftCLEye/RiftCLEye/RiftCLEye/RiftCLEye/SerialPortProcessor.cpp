#include <windows.h>
#include "SerialPortProcessor.h" 

#include <tchar.h>

#define COM_PORT_NUMBER		"\\\\.\\COM3"

CSerialPortProcessor::CSerialPortProcessor(void)
	: myHComPort(NULL), myReceivedData(NULL), myReceivedDataLength(0)
{
	myMutex = CreateMutex(NULL, TRUE, NULL);
}

CSerialPortProcessor::~CSerialPortProcessor(void)
{
	CloseHandle(myMutex);
	if (myReceivedData != NULL){
		delete myReceivedData;
		myReceivedData = NULL;
	}
	myMutex = NULL;
}

void CSerialPortProcessor::Start(LPCSTR comPortName, DCB* portConfig)
{
	LPVOID lpMsgBuf;
	SetLastError(NO_ERROR);		//エラー情報をクリアする

	// 指定ポートを開く 
	myHComPort = CreateFile(
		_T(COM_PORT_NUMBER),
		//(LPCTSTR)comPortName,
		GENERIC_READ | GENERIC_WRITE,
		0, NULL, OPEN_EXISTING, 0, NULL);

	if (myHComPort == INVALID_HANDLE_VALUE)
	{
		DWORD dw = GetLastError();
		FormatMessage(				//エラー表示文字列作成
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf, 0, NULL);

		MessageBox(NULL, (LPCSTR)lpMsgBuf, NULL, MB_OK);	//メッセージ表示

		LocalFree(lpMsgBuf);
		ExitProcess(dw);
	}

	// ポートのボーレート、パリティ等を設定 
	SetCommState(myHComPort, portConfig);

	COMMTIMEOUTS CommTimeouts;
	GetCommTimeouts(myHComPort, &CommTimeouts);

	CommTimeouts.ReadTotalTimeoutConstant = 100;
	CommTimeouts.ReadTotalTimeoutMultiplier = 0;

	SetCommTimeouts(myHComPort, &CommTimeouts);

	myHThread = CreateThread(NULL, 0,
		ThreadFunc, (LPVOID)this,
		CREATE_SUSPENDED, &myThreadId);
}

void CSerialPortProcessor::End()
{
	if (myHComPort != NULL){
		CloseHandle(myHComPort);
	}
}

DWORD WINAPI CSerialPortProcessor::ThreadFunc(LPVOID lpParameter)
{
	return ((CSerialPortProcessor*)lpParameter)->ReceiveData();
}

DWORD WINAPI CSerialPortProcessor::ReceiveData()
{
	BYTE buffer[1024];
	DWORD toReadBytes = 1024;
	DWORD readBytes;
	while (ReadFile(myHComPort, buffer, toReadBytes, &readBytes, NULL)){
		if (readBytes > 0) {
			WaitForSingleObject(myMutex, 0);
			// 受信したデータは、myReceivedData に受信済みで取り出されていない 
			// データに続けて保持しておく 
			BYTE* tmpBuf = new BYTE[myReceivedDataLength + readBytes];
			if (myReceivedData != NULL){
				memcpy(tmpBuf, myReceivedData, myReceivedDataLength);
				delete[] myReceivedData;
			}
			memcpy(tmpBuf, buffer, readBytes);
			myReceivedDataLength += readBytes;
			myReceivedData = tmpBuf;
			ReleaseMutex(myMutex);
		}
	}
	return S_OK;
}

DWORD CSerialPortProcessor::GetReceivedData(LPVOID buffer)
{
	DWORD length;

	WaitForSingleObject(myMutex, 0);

	buffer = new BYTE[myReceivedDataLength];
	memcpy(buffer, myReceivedData, myReceivedDataLength);
	length = this->myReceivedDataLength;

	ReleaseMutex(myMutex);

	return length;
}

DWORD CSerialPortProcessor::SendData(LPVOID buffer, int toWriteBytes)
{
	DWORD writeBytes;
	DWORD index = 0;
	// 指定されたデータを全て書き込む為にループを廻す 
	while (toWriteBytes > 0){
		WriteFile(myHComPort,
			((BYTE*)buffer) + index, toWriteBytes, &writeBytes, NULL);
		index += writeBytes;
		toWriteBytes -= writeBytes;
	}

	return writeBytes;
}