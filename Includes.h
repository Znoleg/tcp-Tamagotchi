#pragma once
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <map>
#include <ostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <semaphore.h>

#define STD_PORT 2050
#define CONNECT_LOST -1
using namespace std;

void error(string msg);

extern sem_t send_sem, recv_sem;

vector<string> SplitString(const string& str, const string& delim);
bool safesend(int sock, void* packet, ssize_t packet_size);
bool saferecv(int sock, void* packet, ssize_t max_packet_size, ssize_t min_packet_size = 1);

enum class TamaTypes
{
	Cat, Hedgehog, Penguin
};

enum class ServerReq
{
    Login, Register, TamagCure, TamagEat, TamagPlay, TamagSleep, TamagPiss
};

enum class FoodType
{
	Apple, Cucumber, Mushroom, Meat, Cheese, Cake, Fish, Icecream
};

enum class Pleasure
{
	Good, OK, Bad 
};

struct User
{
	string name;
	string password;

	string GetCredinals() const
	{
		return name + ' ' + password;
	}

	friend bool operator==(const User& user1, const User& user2)
	{
		if (user1.name == user2.name && user1.password == user2.password) return true;
		return false;
	}	
	friend ostream& operator<<(ostream& os, const User& user)
	{
		os << user.name << ' ' << user.password;
		return os;
	}
	friend istream& operator>>(istream& is, User& user)
	{
		is >> user.name >> user.password;
		return is;
	}
	friend bool operator<(const User& user1, const User& user2)
	{
		return user1.name < user2.name;
	}
};

class SockConnection
{
public:
    int GetSockFd() const;
	int CreateSocket();
protected: 
	void TurnOffPipeSig();
	void RestorePipeSig();
	in_port_t _port;
    int _sockfd;
	sockaddr_in _address;
	struct sigaction _oldPipeActn;
};

