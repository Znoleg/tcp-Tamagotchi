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

using namespace std;

#define STD_PORT 2050

void error(char* msg);

void error(string msg);

vector<string> SplitString(const string& str, const string& delim);

enum class TamaTypes
{
	Cat, Hedgehog, Penguin
};

enum class ServerReq
{
    Login, Register, TamagCure, TamagEat, TamagPlay, TamagSleep, TamagPiss
};

struct User
{
	string name;
	string password;

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
	in_port_t _port;
    int _sockfd;
	sockaddr_in _address;
};

