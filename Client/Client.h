#pragma once
#include "../Includes.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

struct User;

class Client : public SockConnection
{
public:
	Client() {};
	sockaddr_in ConnectToServer(in_port_t port, hostent* server);
	void ServerRegister(const User& user, const string& tamaName, const TamaTypes type);
    bool TryServerLogin(const User& user);
    void HandleTamStats(const double*) const;
    void HandleServerDisconnection();
    void NotifyDisconnection();

    void SendUserCredinals() const;
	void SendCureRequest() const;	
	void SendEatRequest(FoodType type) const;
	void SendSleepRequest() const;
    void SendPlayRequest() const;
	void SendPissRequest() const;

private:
    friend void* GetTamagStatChangeThread(void*);
    friend void* CommandsThread(void*);
    User _currentUser;
	hostent* _serverConnected;
	static Client* _instance;
};
