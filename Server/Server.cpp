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

	//TurnOffPipeSig();
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

void Server::AddClientHandler(pair<int, pthread_t> pthreadHandler)
{
	_clientHandlers.insert(pthreadHandler);
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
	
	ClientData data = {client_fd, owner, tamag, 1};
	_users.push_back(data);
	SaveData();
	
	SendStats(data);
}

bool Server::TryLogin(const User& user, const int client_fd, ClientData& client)
{
	bool res;
	int index = TryFindUser(user, res);
	if (res == false) return false;
	if (_users[index].logged == true) return false;
	_users[index].clientFd = client_fd;

	_users[index].logged = true;
	client = _users[index];
	return true;
}

bool Server::HandleClientDisconnect(int clientSock)
{
	auto it = _clientHandlers.find(clientSock);
	if (it == _clientHandlers.end()) return false;
	pthread_cancel(it->second);
	close(it->first);

	bool res;
	int index = TryFindUser(clientSock, res);
	_users[index].logged = false;

	_clientHandlers.erase(it);

	printf("Client %i disconnected!\n", clientSock);
	fflush(stdout);	
	return true;
}

void NotifyAndExit(int signum)
{
	write(STDOUT_FILENO, "Server is about disconnection!\n", 31);

	auto server = Server::GetInstance();
	auto clientHandlers = server->_clientHandlers;
	for (auto it = clientHandlers.begin(); it != clientHandlers.end(); it++)
	{
		char code = (char)CONNECT_LOST;
		send(it->first, &code, sizeof(char), 0);
	}
	close(server->_sockfd);
	exit(0);
}

void Server::SendStats(const ClientData& client)
{
	bool died = false;
	double* stats = client.tamag->GetStats();
	int fd = client.clientFd;
	size_t size = (client.tamag->GetStatsCount() + 1) * sizeof(double);
	if (!safesend(fd, stats, size))
		HandleClientDisconnect(fd);
	else printf("Sended stats to %i client \n", fd);

	if (stats[0] == 0)
	{
		printf("Player %i died! Erasing him...\n", fd);
		bool res;
		int index = TryFindUser(client.user, res);
		_users.erase(_users.begin() + index);
	}
}

void* StartTamasSimulationThread(void*)
{
	Server* server = Server::GetInstance();
	while (true)
	{
		for (ClientData clientData : server->_users)
		{
			bool logged = clientData.logged;
			clientData.tamag->DoLifeIteration(logged, server->_tamagMultiplier);
			if (logged) 
				server->SendStats(clientData);
		}
		sleep(10);
	}
}

void Server::SaveData()
{
	ofstream file;
	file.open(_fileName, ofstream::out | ofstream::trunc);
	if (!file.is_open()) throw new invalid_argument("Can't open file " + _fileName + " for saving.");
	int writeCnt = 0;
	for (auto userData : _users)
	{
		file << userData.user << " | " << (int)userData.tamag->GetType() << " " << *userData.tamag << endl;
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
		
		_users.push_back({0, user, tama, 0});
	}
}

int Server::TryFindUser(const User& user, bool& res) const
{
    res = false;
	for (int i = 0; i < _users.size(); i++)
	{
		if (_users[i].user == user) 
		{
			res = true;
			return i;
		}
	}
	return 0;
}

int Server::TryFindUser(const int clientFd, bool& res) const
{
    res = false;
	for (int i = 0; i < _users.size(); i++)
	{
		if (_users[i].clientFd == clientFd) 
		{
			res = true;
			return i;
		}
	}
	return 0;
}


void* HandleClientReqs(void* clientFdPtr)
{
    auto server = Server::GetInstance();
    int clientFd = *static_cast<int*>(clientFdPtr);
	ServerReq request;
    while (true) // waiting for client requests
	{
		if (!saferecv(clientFd, &request, sizeof(ServerReq), sizeof(ServerReq)))
		{
			server->HandleClientDisconnect(clientFd);
			pthread_exit(0);
		}
		if (true)
		{
			char requestStr[128];
			if (!saferecv(clientFd, requestStr, 127, 1))
			{
				server->HandleClientDisconnect(clientFd);
				pthread_exit(0);
			}
			printf("Got request from %i user: %s\n", clientFd, requestStr);
			vector<string> splitedReq = SplitString(requestStr, " ");
			User user{splitedReq[0], splitedReq[1]};
			
			if (request == ServerReq::Login)
			{
				double* stats = new double[5];
				ClientData logged_client;
				bool result = server->TryLogin(user, clientFd, logged_client); 
				if (!safesend(clientFd, &result, sizeof(bool)))
				{
					server->HandleClientDisconnect(clientFd);
					pthread_exit(0);
				}
				
				if (result) 
				{
					TamaTypes tamaType = logged_client.tamag->GetType();
					string name = logged_client.tamag->GetName();
					char name_buf[name.size() + 1];
					int size = name.size();
					sprintf(name_buf, "%i", size);
					strcpy(name_buf + 1, name.c_str());
					
					if (!safesend(clientFd, &tamaType, sizeof(tamaType)))
					{
						server->HandleClientDisconnect(clientFd);
						pthread_exit(0);
					}
					if (!safesend(clientFd, name_buf, strlen(name_buf) * sizeof(char)))
					{
						server->HandleClientDisconnect(clientFd);
						pthread_exit(0);
					}
					server->SendStats(logged_client);
				}
			}
			else if (request == ServerReq::Register)
			{
				string tamaName = splitedReq[2]; // splitting request
				TamaTypes type;
				if (!saferecv(clientFd, &type, sizeof(TamaTypes), sizeof(TamaTypes)))
				{
					server->HandleClientDisconnect(clientFd);
					pthread_exit(0);
				}

				server->Register(user, tamaName, type, clientFd);
			}
			else
			{
				bool res;
				int index = server->TryFindUser(user, res);
				if (res == false) break;
				if (request == ServerReq::TamagCure)
				{
					server->_users[index].tamag->CureAnimal();
				}
				else if (request == ServerReq::TamagEat)
				{
					FoodType type;
					if (!saferecv(clientFd, &type, sizeof(FoodType), sizeof(FoodType)))
					{
						server->HandleClientDisconnect(clientFd);
						pthread_exit(0);
					}
					
					Pleasure pleasure = server->_users[index].tamag->FeedAnimal(type);
					if (!safesend(clientFd, &pleasure, sizeof(Pleasure)))
					{
						server->HandleClientDisconnect(clientFd);
						pthread_exit(0);
					}
				}
				else if (request == ServerReq::TamagPlay)
				{
					server->_users[index].tamag->PlayWithAnimal();
				}
				else if (request == ServerReq::TamagPiss)
				{
					server->_users[index].tamag->WalkWithAnimal();
				}
				else if (request == ServerReq::TamagSleep)
				{
					server->_users[index].tamag->SleepWithAnimal();
				}
				
				server->SendStats(server->_users[index]);
			}
			
		}
	}
    return NULL;
}

size_t Server::GetConnectedCount() const
{
	return _clientHandlers.size();
}

void* HandleServerCmds(void*)
{
	auto server = Server::GetInstance();
	while (true)
	{
		char cmd[32];
		scanf("%s", cmd);
		if (strcmp(cmd, "kick") == 0)
		{
			int kick_sock;
			scanf("%i", &kick_sock);
			if (!server->HandleClientDisconnect(kick_sock))
			{
				printf("Client not found!\n");
				continue;
			}
		}
		else if (strcmp(cmd, "setmultiplier") == 0)
		{
			const double minMultip = 0.1;
			double multip;
			scanf("%lf", &multip);
			if (multip < minMultip)
			{
				printf("Multiplier can't be less then %lf\n", minMultip);
				continue;
			}
			server->_tamagMultiplier = multip;
			scanf("Multiplier changed to %lf", &multip);
		}
		else if (strcmp(cmd, "exit") == 0)
		{
			NotifyAndExit(0);
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

	struct sigaction new_actn, old_actn;
    new_actn.sa_handler = SIG_IGN;
    sigemptyset(&new_actn.sa_mask);
    new_actn.sa_flags = 0;
    sigaction(SIGPIPE, &new_actn, &old_actn);
    
	auto server = new Server();
	int sockfd = server->CreateSocket();
	sockaddr_in serv_addr = server->CreateConnection(portno);
	server->StartListening(MAX_CLIENTS);

	struct sigaction sa;
	sa.sa_handler = NotifyAndExit;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	int newsockfd;
	sockaddr_in cli_addr;
	socklen_t clilen;
   	clilen = sizeof(cli_addr);

   	char buf[256];
	pthread_t srvCmdThrd;
	pthread_create(&srvCmdThrd, NULL, HandleServerCmds, NULL);
	pthread_t simulation;
	pthread_create(&simulation, NULL, StartTamasSimulationThread, NULL);
   	int clientId = 0;
   	while (clientId < MAX_CLIENTS)
   	{
    	newsockfd = accept(sockfd, (sockaddr*)&cli_addr, &clilen);
    	if (newsockfd < 0) error("Error on accept\n");
		printf("Client connected! ID: %i, FD: %i\n", clientId, newsockfd);
    	pthread_t threadId;
    	if (pthread_create(&threadId, NULL, HandleClientReqs, (void**)&newsockfd) < 0) error("error creating thread\n");
		server->AddClientHandler(pair<int, pthread_t>(newsockfd, threadId));

    	//pthread_join(threadId, NULL);
    	clientId++;
   	}
}
