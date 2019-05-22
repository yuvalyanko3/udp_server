#include "pch.h"
#include <iostream>
#include <winsock2.h>
#include <string.h>
#include <time.h>
# include <ctime>
#include <string>
#include <dirent.h>

#pragma comment(lib,"ws2_32.lib")

using namespace std;

void setRequest(char[], char*, char*);
int handleRequest(char[], char[], char[]);
void getFileList(char*);
int getFile(char[], char[]);
bool isFileExists(string);
void readFile(char*, char[]);

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Error: no port provided\n");
		exit(-1);

	}

	int TIME_PORT = atoi(argv[1]);
	int msg_counter = 1;
	WSAData wsaData;

	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		cout << "Time Server: Error at WSAStartup()\n";
	}

	SOCKET m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (INVALID_SOCKET == m_socket)
	{
		cout << "Time Server: Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return(-1);
	}
 
	sockaddr_in serverService;
	
	serverService.sin_family = AF_INET;
	serverService.sin_addr.s_addr = INADDR_ANY;	
	serverService.sin_port = htons(TIME_PORT);

	if (SOCKET_ERROR == bind(m_socket, (SOCKADDR *)&serverService, sizeof(serverService)))
	{
		cout << "Time Server: Error at bind(): " << WSAGetLastError() << endl;
		closesocket(m_socket);
		WSACleanup();
		return(-1);
	}

	sockaddr client_addr;
	int client_addr_len = sizeof(client_addr);
	int bytesSent = 0;
	int bytesRecv = 0;
	char sendBuff[255];
	char recvBuff[255];

	// Get client's requests and answer them.
	// The recvfrom function receives a datagram and stores the source address.
	// The buffer for data to be received and its available size are 
	// returned by recvfrom. The fourth argument is an idicator 
	// specifying the way in which the call is made (0 for default).
	// The two last arguments are optional and will hold the details of the client for further communication. 
	// NOTE: the last argument should always be the actual size of the client's data-structure (i.e. sizeof(sockaddr)).
	cout << "File Server: Wait for clients' requests. at PORT: " << TIME_PORT << " \n";
	while (true)
	{
		bytesRecv = recvfrom(m_socket, recvBuff, 255, 0, &client_addr, &client_addr_len);
		if (SOCKET_ERROR == bytesRecv)
		{
			cout << "File Server: Error at recvfrom(): " << WSAGetLastError() << endl;
			closesocket(m_socket);
			WSACleanup();
			return(-1);
		}

		recvBuff[bytesRecv] = '\0';
		cout << "File Server: Recieved: " << bytesRecv << " bytes of \"" << recvBuff << "\" message.\n";
		char request[4];
		char type[255];
		if (recvBuff != NULL)
		{
			memcpy(request, recvBuff, 3);
			request[3] = '\0';
		}
		else
			strcpy(request, "NAN");
		setRequest(recvBuff, request, type);
		if (type[0] != '\0')
		{
			if (!handleRequest(request, type, sendBuff))
				strcpy(sendBuff, "Error Unknown: 500");
		}
		else
			strcpy(sendBuff, "Error Unknown : 500");
		bytesSent = sendto(m_socket, sendBuff, (int)strlen(sendBuff), 0, (const sockaddr *)&client_addr, client_addr_len);
		if (SOCKET_ERROR == bytesSent)
		{
			cout << "File Server: Error at sendto(): " << WSAGetLastError() << endl;
			closesocket(m_socket);
			WSACleanup();
			return(-1);
		}

		cout << "File Server: Sent: " << bytesSent << "\\" << strlen(sendBuff) << " bytes of \"" << sendBuff << "\" message.\n";
		cout << "File Server: Wait for NEW clients' requests.\n";
	}

	cout << "File Server: Closing Connection.\n";
	closesocket(m_socket);
	WSACleanup();
}

void setRequest(char recBuff[], char* req, char* file) {
	bool flag = true;
	int j = 0;
	int space = 0;
	for (int i = 0; recBuff[i] != '\0'; i++)
	{
		if (recBuff[i] == ' ')
			space++;
		if (space < 2)
		{
			if (recBuff[i] != ' ')
			{
				if (flag)
					req[j++] = recBuff[i];
				else
				{
					file[j++] = recBuff[i];
				}
			}
			else
			{
				j = 0;
				flag = false;
			}
		}
	}
	req[4] = '\0';
	file[j] = '\0';
}

int handleRequest(char req[], char type[], char sendBuff[]) {
	int toReturn = 1;

	if (strcmp(req, "GET") == 0)
	{
		if (strcmp(type, "All") == 0)
			getFileList(sendBuff);
		else {
			if (!getFile(type, sendBuff))
				strcpy(sendBuff, "404: File not Found");
		}
	}
	else if (strcmp(req, "PUT") == 0)
		strcpy(sendBuff, "Service is not available at the moment.");
	else
		toReturn = 0;

	return toReturn;
}

void getFileList(char* files) {
	DIR *dirp;
	char arr[255];
	arr[0] = '\0';
	struct dirent *directory;
	dirp = opendir("./files");
	if (dirp)
	{
		while ((directory = readdir(dirp)) != NULL)
		{
			if (strcmp(directory->d_name, ".") != 0 && strcmp(directory->d_name, "..") != 0)
			{
				char* filename = directory->d_name;
				//string s(directory->d_name);
				//printf("%s\n", directory->d_name);
				strcat(arr, directory->d_name);
				strcat(arr, "\n");
			}
		}

		closedir(dirp);
	}
	int i;
	for (i = 0; arr[i] != '\0'; i++)
	{
		files[i] = arr[i];
	}
	files[i] = '\0';
}

int getFile(char fileName[], char sendBuff[]) {
	int toReturn = 0;
	string path = "./files/";
	string name(fileName);
	path += name;
	if (isFileExists(path))
	{
		toReturn = 1;
		strcpy(fileName, path.c_str());
		readFile(fileName, sendBuff);
	}
	return toReturn;
}

bool isFileExists(string fileName) {
	struct stat buffer;
	return (stat(fileName.c_str(), &buffer) == 0);
}

void readFile(char *fileName, char sendBuff[]) {
	char arr[255] = "";
	FILE *f = fopen(fileName, "rb");
	fseek(f, 0, SEEK_END);
	long fileSize = ftell(f);
	fseek(f, 0, SEEK_SET);
	fread(arr, fileSize, 1, f);
	fclose(f);
	int i;
	for (i = 0; arr[i] != '\0'; i++)
	{
		sendBuff[i] = arr[i];
	}
	sendBuff[i] = '\0';
}