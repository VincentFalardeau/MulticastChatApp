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

const int MAX_CLIENTS = 5;

HANDLE hSemaphore;

struct Client
{
	int id;
	SOCKET socket;
};

int GetAvailableId();
void ShutDownClient(SOCKET* pClientSocket, int id, string msgToClient, string msgToServer);
int SendMsgToClient(SOCKET* pClientSocket, const char* msgToClient);
int ReceiveClientMessage(SOCKET* pClientSocket, string& clientMsg);
void Write(ofstream*, string);
int ReceiveFile(Client*, string);
DWORD WINAPI ClientThread(void* pParam);
void ColorConsole(int couleurDuTexte, int couleurDeFond);

vector<Client> clients(MAX_CLIENTS);

int main()
{
	setlocale(LC_ALL, "");
	SOCKET listenSocket = INVALID_SOCKET;

	WSADATA wsaData;
	SOCKADDR_IN sin;

	sin.sin_addr.s_addr = inet_addr("127.0.0.1");
	sin.sin_family = AF_INET;
	sin.sin_port = htons(80);

	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (iResult != 0)
	{
		cout << "WSAStartup failed: " << iResult << endl;
		return 1;
	}

	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (listenSocket == INVALID_SOCKET)
	{
		cout << "Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return 1;
	}

	iResult = bind(listenSocket, (SOCKADDR*)& sin, sizeof(sin));

	if (iResult == SOCKET_ERROR)
	{
		cout << "Le bind a échoué" << endl;
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	if (listen(listenSocket, 2) == SOCKET_ERROR)
	{
		cout << "listen failed: " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	int numClient = 0;
	bool endOfCommunication = false;
	HANDLE hClientThreads[MAX_CLIENTS];

	Client client;
	//Initialiser le vecteur clients
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		client.id = i;
		client.socket = INVALID_SOCKET;
		clients[i] = client;
	}
	hSemaphore = CreateSemaphore(NULL, 2 /* valeur de depart */, 2 /* nb max d'access */, NULL);
	while (!endOfCommunication)
	{
		SOCKET newSocket = INVALID_SOCKET;
		//Accepter une connexion
		newSocket = accept(listenSocket, NULL, NULL);
			
		if (newSocket != INVALID_SOCKET)
		{
			//Obtenir le numéro du nouveau client (selon disponibilité)
			numClient = GetAvailableId();
			//Si un numéro est disponible
			if (numClient != -1) 
			{
				//Accepter le nouveau client dans la file
				ColorConsole(6, 0);
				clients[numClient].socket = newSocket;
				cout << "Le client #" << numClient << " est accepté" << endl;
				//Envoyer la confirmation de l'acceptation au client
				iResult = SendMsgToClient(&newSocket, (char*)to_string(numClient).c_str());
				if (iResult == 0)
				{
					//Débuter un nouveau thread pour le nouveau client
					hClientThreads[numClient] = CreateThread(NULL, 0, ClientThread, &clients[numClient], 0, 0);
				}
				else 
				{
					ShutDownClient(&newSocket, numClient, "", (string)("Client #" + to_string(client.id) + " déconnecté"));
				}
			}
			else
			{
				ShutDownClient(&newSocket, numClient, "Le serveur est plein", "");
			}
		}
	} 
	closesocket(listenSocket);
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		CloseHandle(hClientThreads[i]);
		closesocket(clients[i].socket);
	}
	WSACleanup();
	return 0;
}

//Sert à fermer la connexion avec un client
//On a l'option d'écrire un message au client avant de fermer la connexion ou un message dans la console serveur
void ShutDownClient(SOCKET* pClientSocket, int id, string msgToClient, string msgToServer)
{
	ColorConsole(6, 0);
	if (msgToClient != "")
	{
		SendMsgToClient(pClientSocket, (char*)msgToClient.c_str());
	}
	if (msgToServer != "")
	{
		cout << msgToServer << endl;
	}
	closesocket(*pClientSocket);
	if (id != -1) 
	{
		closesocket(clients[id].socket);
		clients[id].socket = INVALID_SOCKET;
	}
	
}

//Obtenir une place vide dans la file d'attente, le numéro de la place
int GetAvailableId() 
{
	int id = -1;
	for (int i = 0; i < clients.size(); i++) 
	{
		if(clients[i].socket == INVALID_SOCKET)
		{
			id = clients[i].id;
			break;
		}
	}
	return id;
}

DWORD WINAPI ClientThread(void* pParam)
{
	Client* client = reinterpret_cast<Client*>(pParam);
	string msg;
	//Recevoir la confirmation du client, pour valider que la communication fonctionne
	int iResult = ReceiveClientMessage(&client->socket, msg);
	if (msg != "ok")
	{
		ShutDownClient(&client->socket, client->id, "", (string)("Client #" + to_string(client->id) + " déconnecté"));
		return 0;
	}
	//Attendre le token du sémaphore
	WaitForSingleObject(hSemaphore, INFINITE);
	//Indiquer au client qu'il est accepté, pour qu'il puisse commencer à écrire
	iResult = SendMsgToClient(&client->socket, (char*)((string)"ok").c_str());
	if (iResult != 0)
	{
		ReleaseSemaphore(hSemaphore, 1, NULL);
		ShutDownClient(&client->socket, client->id, "", (string)("Client #" + to_string(client->id) + " déconnecté"));
		return 0;
	}
	
	bool endOfCommunication = false;
	//Tant que la communication n'est pas terminée
	while (!endOfCommunication)
	{
		if(client->socket != 0)
		{
			//Recevoir un message du client
			iResult = ReceiveClientMessage(&client->socket, msg);
			//Recevoir un simple message à afficher
			if (iResult > 0 && msg != "fin" && msg.find("sfile") == string::npos)
			{
				ColorConsole(client->id + 1, 0);
				cout << "Client #" << client->id << ": " << msg << endl;
			}
			//Recevoir un fichier
			else if (iResult > 0 && msg != "fin" && msg.find("sfile") != string::npos) {
				string fileName = msg.substr(6, msg.find("\n") - 6);
				
				ReceiveFile(client, fileName);
				ColorConsole(client->id + 1, 0);
				cout << "Client #" << client->id << ": " << "Fichier reçu: " << fileName << endl;
			}
			else
			{
				//Fermer le client
				ShutDownClient(&client->socket, client->id, "", (string)("Client #" + to_string(client->id) + " déconnecté"));
				endOfCommunication = true;
			}
		}
	}
	//Libérer un token du sémaphore
	ReleaseSemaphore(hSemaphore, 1, NULL); 
	return 0;
}

//Sert à recevoir un fichier
int ReceiveFile(Client* client, string fileName)
{
	try
	{
		SOCKET* pClientSocket = &client->socket;
		ofstream ofs("Client" + to_string(client->id) + "-" + fileName);
		string clientMsg = "";
		string pr = "pr";
		//Confirmer la reception de fichier au client
		if (SendMsgToClient(pClientSocket, (char*)pr.c_str()) != 0) return 1;
		//Tant que la réception n'est pas terminée
		while (clientMsg != "efile")
		{
			//Écriture du paquet reçu au fichier
			Write(&ofs, clientMsg);
			//Réception du paquet
			ReceiveClientMessage(pClientSocket, clientMsg);
			pr = "pr";
			//Confirmation de la réception
			if (clientMsg != "efile")
			{
				if (SendMsgToClient(pClientSocket, (char*)pr.c_str()) != 0) return 1;
			}
			
		}
		ofs.close();
	}
	catch (exception)
	{
		cout << "Erreur de réception de fichier" << endl;
	}
	return 0;
}

//Sert à écrire à un osftream
void Write(ofstream* ofs, string str)
{
	//Nettoyage du string
	str.erase(remove(str.begin(), str.end(), '\r'), str.end());
	str.erase(remove(str.begin(), str.end(), 'Ì'), str.end());
	//Écriture à l'ofs
	*ofs << str;
}

int SendMsgToClient(SOCKET* pClientSocket, const char* msgToClient)
{
	//Chiffrement et envoi du message
	chiffrer((char*)msgToClient, 5);
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
int ReceiveClientMessage(SOCKET* pClientSocket, string& clientMsg)
{
	char recvbuf[512], msg[512];
	//Réception du message
	int iResult = recv(*pClientSocket, recvbuf, 512, 0);
	if (iResult < 0)
	{
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

void ColorConsole(int couleurDuTexte, int couleurDeFond) 
{ 
	HANDLE H = GetStdHandle(STD_OUTPUT_HANDLE);  
	SetConsoleTextAttribute(H, couleurDeFond * 16 + couleurDuTexte); 
}





