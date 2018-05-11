// Alfred Shaker
// windows socket server for web app communication
// 5/3/2018

#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "tserial.h"

using namespace std;

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

// Serial to Arduino global declarations
int arduino_command;
Tserial *arduino_com;
unsigned char MSB = 0;

int __cdecl main(void)
{
	WSADATA WSAData;

	SOCKET server, client;

	SOCKADDR_IN serverAddr, clientAddr;

	// serial to Arduino setup 
	arduino_com = new Tserial();
	if (arduino_com != 0) {
		arduino_com->connect("COM3", 57600, spNONE);
	}
	// serial to Arduino setup 

	WSAStartup(MAKEWORD(2, 0), &WSAData);
	server = socket(AF_INET, SOCK_STREAM, 0);

	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(5555);

	bind(server, (SOCKADDR *)&serverAddr, sizeof(serverAddr));
	listen(server, 0);

	cout << "Listening for incoming connections..." << endl;

	char buffer[1024];
	int clientAddrSize = sizeof(clientAddr);
	while (1)
	{
		if ((client = accept(server, (SOCKADDR *)&clientAddr, &clientAddrSize)) != INVALID_SOCKET)
		{
			cout << "Client connected!" << endl;
			recv(client, buffer, sizeof(buffer), 0);
			cout << "Client says: " << buffer << endl;
			memset(buffer, 0, sizeof(buffer));

			MSB = (1 << 8) & 0xff;
			cout << "CHAR: " << isprint(MSB) << endl;
			arduino_com->sendChar(MSB);

			closesocket(client);
			cout << "Client disconnected." << endl;
		}
	}
	

	return 0;
}
