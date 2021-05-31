#include "Server.h"
#include <threads.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <sstream>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <termios.h>

Server* Server::_instance = nullptr;

Tamagochi* GetTamagochiUsingType(const string& name, const TamaTypes type)
{
	Tamagochi* tam;
	switch (type)
	{
		case TamaTypes::Cat:
			tam = new Cat(name);
			break;
		
		case TamaTypes::Hedgehog:
			tam = new Hedgehog(name);
			break;

		case TamaTypes::Penguin:
			tam = new Penguin(name);
		
		default:
			break;
	}
	return tam;
}

Server::Server()
{
	if (_instance != nullptr) throw new exception(); 
	
	Server::_instance = this;

	LoadData();
}

Server::~Server()
{
	SaveData();
}

sockaddr_in Server::CreateConnection(in_port_t& port)
{
	if (_sockfd == 0) error("Server error: create socket before creating connetion!");
	_port = port;
	sockaddr_in serv_addr;
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_family = AF_INET; // TCP
    serv_addr.sin_port = htons(_port);

	const int maxTries = 5;
	int tryNumber = 0;
    while (bind(_sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
	{
		if (tryNumber == maxTries) error("Can't bind server after " + to_string(maxTries) + " tries\n");

		printf("Error on binding with %i port, changing...\n", port);
		port += 100;
		tryNumber++;
	}
	printf("Listening on %i port...\n", port);

	return serv_addr;
}

void Server::StartListening(unsigned maxListeners)
{
	if (_sockfd == 0) error("Server error: create socket before listening start!\n"); 
	if (listen(_sockfd, maxListeners) < 0) error("Server error: error on listening!\n");
}

Server* Server::GetInstance()
{
	return _instance;
}


void Server::Register(const User& owner, const string tamaName, const TamaTypes tamType, const int client_fd)
{
	Tamagochi* tamag = GetTamagochiUsingType(tamaName, tamType);
	
	ClientData data = {client_fd, owner, *tamag, 1};
	_users.push_back(data);
	SaveData();
	
	SendStats(data);
}

bool Server::TryLogin(const User& user, const int client_fd, double*& stats)
{
	bool res;
	ClientData toLogin = TryFindUser(user, res);
	if (res == false) return false;
	if (toLogin.logged == true) return false;
	toLogin.clientFd = client_fd;

    TamaTypes type = toLogin.tamag.GetType();
    send(client_fd, &type, 4ul, 0);
	stats = toLogin.tamag.GetStats();
	toLogin.logged = true;
	return true;
}

void Server::SendStats(const ClientData& client) const
{
	double* stats = client.tamag.GetStats();
	send(client.clientFd, stats, 127ul, 0);
	printf("Sended stats to %i client \n", client.clientFd);
}

void* StartTamasSimulationThread(void*)
{
	Server* server = Server::GetInstance();
	while (true)
	{
		for (ClientData clientData : server->_users)
		{
			clientData.tamag.DoLifeIteration();
			if (clientData.logged) 
				server->SendStats(clientData);
		}
		sleep(10);
	}
}

// Activates and returns tamagochi life simulation thread
pthread_t Server::ActivateUserTama(ClientData& owner, string& dieStatus)
{
	//if (!TryFindUser(owner, NULL)) throw new invalid_argument("User " + owner.name + "doesn't exist.");
	auto userTama = owner.tamag;
	pthread_t simulation;
	pthread_create(&simulation, NULL, StartTamasSimulationThread, (void*)&userTama);
	pthread_join(simulation, (void**)&dieStatus);
	return simulation;
}

void Server::SaveData()
{
	ofstream file;
	file.open(_fileName, ofstream::out | ofstream::trunc);
	if (!file.is_open()) throw new invalid_argument("Can't open file " + _fileName + " for saving.");
	int writeCnt = 0;
	for (auto userData : _users)
	{
		file << userData.user << " | " << (int)userData.tamag.GetType() << " " << userData.tamag << endl;
		writeCnt++;
	}
	printf("Data successfully saved: saved %i data\n", writeCnt);
}

void Server::LoadData()
{
	fstream file;
	file.open(_fileName);
	printf("%s\n", _fileName.c_str());
	if (!file.is_open()) error("Error opening server log file\n");
	char trash;
	while (!file.eof())
	{
		User user;
		Tamagochi* tama;
		int tamTypeInt;
		TamaTypes tamType;
		file >> user >> trash >> tamTypeInt;
		if (user.name == "") return;
		tamType = static_cast<TamaTypes>(tamTypeInt);

		tama = GetTamagochiUsingType("", tamType);
		
		file >> *tama;
		
		_users.push_back({0, user, *tama, 0});
	}
}

ClientData Server::TryFindUser(const User& user, bool& res) const
{
    res = false;
	for (auto userData : _users)
	{
		if (userData.user == user) 
		{
			res = true;
			return userData;
		}
	}
}

ClientData Server::TryFindUser(const int clientFd, bool& res) const
{
    res = false;
	for (auto userData : _users)
	{
		if (userData.clientFd == clientFd) 
		{
			res = true;
			return userData;
		}
	}
}


void* HandleClientReqs(void* clientFdPtr)
{
    auto server = Server::GetInstance();
    int clientFd = *static_cast<int*>(clientFdPtr);
	ServerReq request;
    while (true) // waiting for client requests
	{
		int ret = recv(clientFd, &request, sizeof(&request), 0);
		if (ret > 0)
		{
			char requestStr[128];
			while (recv(clientFd, requestStr, 127, 0) <= 0);
			printf("Request %s\n", requestStr);
			vector<string> splitedReq = SplitString(requestStr, " ");
			User user{splitedReq[0], splitedReq[1]};
			
			if (request == ServerReq::Login)
			{
				double* stats = new double[5];
				bool result = server->TryLogin(user, clientFd, stats); 
                if (!result) 
				{
					char r = 'F';
					send(clientFd, &r, 127ul, 0);
				}
				else 
				{
					char r = 'T';
					send(clientFd, &r, 127ul, 0);
					bool res;
 					ClientData client = server->TryFindUser(user, res);
					auto type = client.tamag.GetType();
					send(clientFd, &type, 4ul, 0);
					server->SendStats(client);
				}
			}
			else if (request == ServerReq::Register)
			{
				string tamaName = splitedReq[2]; // splitting request
				TamaTypes type;
				recv(clientFd, &type, sizeof(type), 0); // Recieving type

				server->Register(user, tamaName, type, clientFd);
			}
			else
			{
				bool res;
				ClientData data = server->TryFindUser(user, res);
				if (res == false) break;
				if (request == ServerReq::TamagCure)
				{
					data.tamag.CureAnimal();
				}
				else if (request == ServerReq::TamagEat)
				{
					data.tamag.FeedAnimal(Food::Apple); // for example;
				}
				else if (request == ServerReq::TamagPlay)
				{
					data.tamag.PlayWithAnimal();
				}
				else if (request == ServerReq::TamagPiss)
				{
					data.tamag.WalkWithAnimal();
				}
				else if (request == ServerReq::TamagSleep)
				{
					data.tamag.SleepWithAnimal();
				}
				
				server->SendStats(data);
			}
			
		}
	}
    return NULL;
}

void* HandleServerCmds(void*)
{
	while (true)
	{
		int ch = getchar();
		if (ch == 'x')
		{
			Server::GetInstance()->SaveData();
		}
	}
}

int main(int argc, char* argv[])
{
	/* Port definition */
   in_port_t portno = STD_PORT;
   if (argc == 2)
   {
       portno = atoi(argv[1]);
   }
	/* */
    
	auto server = new Server();
	int sockfd = server->CreateSocket();
	sockaddr_in serv_addr = server->CreateConnection(portno);
	server->StartListening(MAX_CLIENTS);

	int newsockfd;
	sockaddr_in cli_addr;
	socklen_t clilen;
   clilen = sizeof(cli_addr);

   char buf[256];
	pthread_t srvCmdThrd;
	pthread_create(&srvCmdThrd, NULL, HandleServerCmds, NULL);
	pthread_t simulation;
	pthread_create(&simulation, NULL, StartTamasSimulationThread, NULL);
	//pthread_join(srvCmdThrd, NULL);
   int clientId = 0;
   while (clientId < MAX_CLIENTS)
   {
       newsockfd = accept(sockfd, (sockaddr*)&cli_addr, &clilen);
       if (newsockfd < 0) error("Error on accept\n");
		printf("Client connected! ID: %i, FD: %i\n", clientId, newsockfd);
       pthread_t threadId;
       if (pthread_create(&threadId, NULL, HandleClientReqs, (void**)&newsockfd) < 0) error("error creating thread\n");
       pthread_join(threadId, NULL);
       //close(newsockfd);
       clientId++;
   }
}
