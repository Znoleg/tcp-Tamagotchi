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

void Server::HandleClientDisconnect(int clientSock)
{
	printf("Client %i disconnected!\n", clientSock);
	fflush(stdout);
	
	auto it = _clientHandlers.find(clientSock);
	//pthread_cancel(it->second);
	bool res;
	int index = TryFindUser(clientSock, res);
	_users[index].logged = false;
}

void Server::SendStats(const ClientData& client) const
{
	double* stats = client.tamag->GetStats();
	size_t size = client.tamag->GetStatsCount() * sizeof(double);
	send(client.clientFd, stats, size, 0);
	printf("Sended stats to %i client \n", client.clientFd);
}

void* StartTamasSimulationThread(void*)
{
	Server* server = Server::GetInstance();
	while (true)
	{
		for (ClientData clientData : server->_users)
		{
			bool logged = clientData.logged;
			clientData.tamag->DoLifeIteration(logged);
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
				send(clientFd, &result, sizeof(bool), 0);
				
				if (result) 
				{
					TamaTypes tamaType = logged_client.tamag->GetType();
					string name = logged_client.tamag->GetName();
					char name_buf[name.size() + 1];
					int size = name.size();
					sprintf(name_buf, "%i", size);
					strcpy(name_buf + 1, name.c_str());
					
					send(clientFd, &tamaType, sizeof(tamaType), 0);
					send(clientFd, name_buf, strlen(name_buf) * sizeof(char), 0);
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
					server->_users[index].tamag->FeedAnimal(type);
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
		server->AddClientHandler(pair<int, pthread_t>(newsockfd, threadId));

    	pthread_join(threadId, NULL);
    	clientId++;
   }
}
