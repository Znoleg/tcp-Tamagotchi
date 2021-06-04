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
	Client();
    bool ConnectToServer(in_port_t port, hostent* server);
    void ServerRegister(const User& user, const string& tamaName, TamaTypes type);
    bool TryServerLogin(const User& user);
    void HandleTamStats(const double*) const;
    void HandleServerDisconnection();
    void NotifyDisconnection();

    void SendUserCredinals();
    void SendCureRequest();
    void SendEatRequest(FoodType type);
    void SendSleepRequest();
    void SendPlayRequest();
    void SendPissRequest();

private:
    friend void* GetTamagStatChangeThread(void*);
    friend void* CommandsThread(void*);
    User _currentUser;
	hostent* _serverConnected;
	static Client* _instance;
};
