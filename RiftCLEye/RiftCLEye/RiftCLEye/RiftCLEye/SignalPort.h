#pragma once

#define MAX_NUMBER_OF_RECV_STRING	256
#define MAX_NUMBER_OF_SEND_STRING	6

#include <windows.h>

class SignalPort
{
public:
	SignalPort(void);
	~SignalPort(void);
	int Connect(void);
	int Recv(unsigned char str[MAX_NUMBER_OF_RECV_STRING]);
	int Send(unsigned char str[MAX_NUMBER_OF_RECV_STRING]);
	unsigned char ReadByte();

private:
	HANDLE m_hComPort;

public:
	int Disconnect(void);
};
