#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include<sstream>
#include <fstream>
#include <algorithm>
#include "..\..\RotationDLL\RotationDLL\rotation.h"
#include <vector>
using namespace std;
// link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

HANDLE threadRecv;

int SendMsgToClient(SOCKET *, const char *);
int ReceiveClientMessage(SOCKET*, string&);
void Write(ofstream*, string);
int ReceiveFile(SOCKET*, string);
int SendFile(SOCKET*, string);
void ColorConsole(int, int);
int SetupServerSocketsAndWaitForClient(SOCKET&, SOCKET&);

DWORD WINAPI Send(void*);
DWORD WINAPI Recv(void*);

bool FileReceiving;
bool FileSending;
bool EndOfCommunication;
bool EndOfClientCommunication;

int main() {
	setlocale(LC_ALL, "");

	//Déclaration des sockets
	SOCKET ClientSocket, ListenSocket = INVALID_SOCKET;

	int iResult = SetupServerSocketsAndWaitForClient(ClientSocket, ListenSocket);
	if (iResult != 0) return 1;

	EndOfCommunication = false;
	EndOfClientCommunication = false;
	FileSending = false;
	FileReceiving = false;

	threadRecv = CreateThread(NULL, 0, Recv, &ClientSocket, 0, 0);
	HANDLE threadSend = CreateThread(NULL, 0, Send, &ClientSocket, 0, 0);
	WaitForSingleObject(threadRecv, INFINITE); 
	CloseHandle(threadRecv);
	WaitForSingleObject(threadSend, INFINITE); 
	CloseHandle(threadSend);

	ColorConsole(5, 0);
	cout << "Connection éteinte" << endl;
	closesocket(ListenSocket);
	WSACleanup();
	return 0;
}

DWORD WINAPI Send(void* pParam)
{
	SOCKET* pClientSocket = reinterpret_cast<SOCKET*>(pParam); 
	
	string msgToClient = "";
	string fileName = "";
	int iResult = 0;

	while (!EndOfCommunication && ! EndOfClientCommunication)
	{
		msgToClient = "";
		ColorConsole(2, 0);
		getline(cin, msgToClient);
		EndOfCommunication = msgToClient == "fin";
		if (msgToClient.find("sfile") != string::npos)
		{
			try
			{
				fileName = msgToClient.substr(6, msgToClient.find("\n") - 6);
				FileSending = true;
				SuspendThread(threadRecv);
				iResult = SendFile(pClientSocket, fileName);
				if (iResult != 0) return 1;
				ResumeThread(threadRecv);
				FileSending = false;
			}
			catch (exception){}
		}
		else if(!FileReceiving && !EndOfClientCommunication)
		{
			EndOfClientCommunication = SendMsgToClient(pClientSocket, (char*)msgToClient.c_str()) != 0;
		}
		
	}
	return 0;
}

DWORD WINAPI Recv(void* pParam)
{
	SOCKET* pClientSocket = reinterpret_cast<SOCKET*>(pParam);

	string clientMsg = "";
	int iResult = 0;

	while (!EndOfCommunication && !EndOfClientCommunication) 
	{
		ColorConsole(9, 0);
		cout << clientMsg << endl;

		ColorConsole(2, 0);
		iResult = ReceiveClientMessage(pClientSocket, clientMsg);
		if (iResult < 0) 
		{
			EndOfCommunication = true;
		}
		if (clientMsg.find("sfile") != string::npos && !FileSending)
		{
			FileReceiving = true;
			string fileName = clientMsg.substr(6, clientMsg.find("\n") - 6);
			ReceiveFile(pClientSocket, fileName);
			clientMsg = "Fichier reçu: " + fileName;
			FileReceiving = false;
		}
		EndOfClientCommunication = clientMsg == "fin";
	}
	return 0;
}



//Sert à envoyer un message au client
int SendMsgToClient(SOCKET* pClientSocket, const char* msgToClient)
{
	//Chiffrement et envoi du message
	chiffrer((char *)msgToClient, 5);
	if (send(*pClientSocket, msgToClient, strlen(msgToClient), 0) == SOCKET_ERROR)
	{
		cout << "Erreur d'envoi du message au client: " << endl;
		if (shutdown(*pClientSocket, SD_SEND) == SOCKET_ERROR)
		{
			cout << "shutdown failed: " << WSAGetLastError() << endl;
		}
		closesocket(*pClientSocket);
		WSACleanup();
		return 1;
	}
	return 0;
}

//Sert à recevoir un message du client
int ReceiveClientMessage(SOCKET* pClientSocket, string & clientMsg)
{
	char recvbuf[512], msg[512];
	//Réception du message
	int iResult = recv(*pClientSocket, recvbuf, 512, 0);
	if (iResult < 0)
	{
		cout << "Erreur de réception ou de connexion avec le client"  << endl;
		return -1;
	}
	memset(&msg, 0, sizeof(msg));
	strncpy(msg, recvbuf, iResult);
	strncpy(recvbuf, "", iResult);
	//Déchiffrement du message
	dechiffrer(msg, 5);
	clientMsg = msg;

	return iResult;
}

//Sert à écrire un string à un ofstream
void Write(ofstream* ofs, string str)
{
	//Nettoyage du string
	replace(str.begin(), str.end(), '\r', ' ');
	str.erase(remove(str.begin(), str.end(), 'Ì'), str.end());
	//Écriture à l'ofs
	*ofs << str;
}

//Sert à recevoir un fichier
int ReceiveFile(SOCKET* pClientSocket, string fileName)
{
	try
	{
		ofstream ofs(fileName);
		string clientMsg = "";
		string paquetReceived = "pr";
		//Confirmer la reception de fichier au client
		if (SendMsgToClient(pClientSocket, (char*)paquetReceived.c_str()) != 0) return 1;
		paquetReceived = "pr";
		if (SendMsgToClient(pClientSocket, (char*)paquetReceived.c_str()) != 0) return 1;
		//if (SendMsgToClient(ClientSocket, (char*)paquetReceived.c_str()) != 0) return 1;
		//Tant que la réception n'est pas terminée
		while (clientMsg != "efile")
		{
			//Écriture du paquet reçu au fichier
			Write(&ofs, clientMsg);
			//Réception du paquet
			ReceiveClientMessage(pClientSocket, clientMsg);
			paquetReceived = "pr";
			//Confirmation de la réception
			if (clientMsg != "efile")
			{
				if (SendMsgToClient(pClientSocket, (char*)paquetReceived.c_str()) != 0) return 1;
			}
			
		}
		ofs.close();
		ColorConsole(9, 0);
	}
	catch (exception)
	{
		cout << "Erreur de réception de fichier" << endl;
	}
	return 0;
}

//Sert à envoyer un fichier
int SendFile(SOCKET* pClientSocket, string fileName)
{
	try
	{
		ifstream ifs(fileName);
		//Si le fichier existe et que le nom n'est pas null
		if (ifs.is_open() && fileName != "")
		{
			//On avertit le client qu'il va recevoir un fichier
			if (SendMsgToClient(pClientSocket, (char*)("sfile " + fileName).c_str()) != 0) return 1;
			string fileContentBuffer = "";
			string clientMsg = "";
			string msgToClient = "";
			//On reçoit la confirmation du client
			while (clientMsg != "pr") 
			{
				ReceiveClientMessage(pClientSocket, clientMsg);
			}
			clientMsg = "";
			while (!ifs.eof())
			{
				//On lit l'entièreté du fichier dans un string (un string est de grandeur suffisante pour le contexte)
				getline(ifs, msgToClient);
				msgToClient += "\n";
				fileContentBuffer += msgToClient;
			}
			fileContentBuffer = fileContentBuffer.substr(0, fileContentBuffer.length() - 1);
			//On prépare les paquets
			vector<string> paquets;
			//On ajoute un premier paquet de taille variable, 
			//pour que ce qui reste à envoyer ait une taille correspondant à un multiple de 512, la taille en bytes des paquets qu'on envoit
			paquets.push_back(fileContentBuffer.substr(0, fileContentBuffer.length() % 512));
			fileContentBuffer = fileContentBuffer.substr(fileContentBuffer.length() % 512, fileContentBuffer.length() - fileContentBuffer.length() % 512);
			//On sudivise ce qui reste à envoyer en plusieurs paquets de taille de 512 bytes
			for (int i = 0; i < fileContentBuffer.length() / 512; i++)
			{
				if (fileContentBuffer.length() >= 512 * (i + 1))
				{
					paquets.push_back(fileContentBuffer.substr((512 * i), 512));
				}
			}
			//On envoit chaque paquet un à un
			for (int i = 0; i < paquets.size(); i++)
			{
				if (SendMsgToClient(pClientSocket, (char*)paquets[i].c_str()) != 0) return 1;
				while (clientMsg != "pr")
				{
					//Pour chaque paquet envoyé, on attend de recevoir la confirmation du client avant de continuer
					ReceiveClientMessage(pClientSocket, clientMsg);
				}
				clientMsg = "";
			}
			paquets.clear();
			//On envoi efile au client pour lui aviser que le fichier a été envoyé
			msgToClient = "efile";
			if (SendMsgToClient(pClientSocket, (char*)msgToClient.c_str()) != 0) return 1;
			ifs.close();
		}
		else
		{
			cout << "Erreur d'ouverture du fichier" << endl;
		}
	}
	catch (exception)
	{
		cout << "Erreur d'envoi du fichier" << endl;
	}
	return 0;
}

void ColorConsole(int couleurDuTexte, int couleurDeFond) 
{ 
	HANDLE H = GetStdHandle(STD_OUTPUT_HANDLE);  
	SetConsoleTextAttribute(H, couleurDeFond * 16 + couleurDuTexte); 
}

int SetupServerSocketsAndWaitForClient(SOCKET& ClientSocket, SOCKET& ListenSocket)
{
	ColorConsole(5, 0);
	//Environnement socket 
	WSADATA wsaData;
	//Structure qui contient des informations utiles au sockets
	SOCKADDR_IN sin;

	//Assignation des variables de la structure sockaddr
	sin.sin_addr.s_addr = inet_addr("127.0.0.1");//adresse ip
	sin.sin_family = AF_INET;//Famille de socket
	sin.sin_port = htons(8080);//port 8080

	//iResult est une variable qui sert à contenir le nombre bytes recus ou envoyes
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	//On vérifie qu'il n'y a pas d'erreur lors du WSAStartup
	if (iResult != 0)
	{
		cout << "WSAStartup failed: " << iResult << endl;
		return 1;
	}

	//Initialisation du socket à l'écoute
	ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//Le socket est invalide
	if (ListenSocket == INVALID_SOCKET)
	{
		cout << "Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return 1;
	}

	//Liaison du socket d'écoute avec une structure sockaddr
	iResult = bind(ListenSocket, (SOCKADDR*)& sin, sizeof(sin));

	//Erreur de liaison du socket
	if (iResult == SOCKET_ERROR)
	{
		cout << "Le bind a échoué" << endl;
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	//On initialise la taille de la file de connexions
	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		cout << "listen failed: " << WSAGetLastError() << endl;
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	//Initialisation du socket client à une valeur nulle
	ClientSocket = INVALID_SOCKET;

	//On attend un client qui est un socket préalablement lié avec un port avec la fonction bind
	cout << "Attendre une connexion d'un client ....." << endl;
	ClientSocket = accept(ListenSocket, NULL, NULL);
	cout << "Un client vient de se connecter ....." << endl;

	//On valide le client
	if (ClientSocket == INVALID_SOCKET)
	{
		cout << "accept failed: " << WSAGetLastError() << endl;
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	return 0;

}

