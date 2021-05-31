#pragma once
#include "Tamagochi.cpp"
#include <threads.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <strings.h>

#define MAX_CLIENTS 5

struct ClientData
{
	int clientFd;
	User user;
	Tamagochi tamag;
	bool logged;
};

class Server : public SockConnection
{
public:
	Server();
	
	sockaddr_in CreateConnection(in_port_t& port);
	void StartListening(unsigned maxListeners);
	bool TryLogin(const User& user, const int client_fd, double*& stats);
	void Register(const User& owner, const string tamaName, const TamaTypes tamType, const int client_fd);
	static Server* GetInstance();
	~Server();
private:
	Server(const Server&);
	Server& operator=(Server&);
	pthread_t ActivateUserTama(ClientData&, string&);
	void LoadData();
	void SaveData();
	friend void* StartTamasSimulationThread(void*);
	friend void* HandleServerCmds(void*);
	friend void* HandleClientReqs(void*);
	void SendStats(const ClientData& client) const;
	void HandleRequest(void*);
	ClientData TryFindUser(const User& user, bool& res) const;
	ClientData TryFindUser(const int clientFd, bool& res) const;
	vector<ClientData> _users;
	string _fileName = "./temp/serverdata.txt";
	static Server* _instance;
};
