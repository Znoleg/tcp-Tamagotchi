#include "Includes.h"

void error(string msg)
{
    error(msg.c_str());
    exit(1);
}

vector<string> SplitString(const string& str, const string& delim)
{
    vector<string> tokens;
    size_t prev = 0, pos = 0;
    do
    {
        pos = str.find(delim, prev);
        if (pos == string::npos) pos = str.length();
        string token = str.substr(prev, pos-prev);
        if (!token.empty()) tokens.push_back(token);
        prev = pos + delim.length();
    } while (pos < str.length() && prev < str.length());
    return tokens;
}

bool safesend(int sock, void* packet, size_t packet_size)
{
    if (send(sock, packet, packet_size, 0) <= packet_size) return false;
    return true;
}

bool saferecv(int sock, void* packet, size_t max_packet_size, size_t min_packet_size)
{
    if (recv(sock, packet, max_packet_size, 0) < min_packet_size) return false;

    char* p = static_cast<char*>(packet);
    if (p[0] == char(CONNECT_LOST)) return false;
    return true;
}

int SockConnection::GetSockFd() const
{
    return _sockfd;
}

int SockConnection::CreateSocket()
{
    _sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (_sockfd < 0) error("Server error: error opening socket!");
    int reuse = 1;
    if (setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
        error("Server error: error on setsockopt");
    return _sockfd;
}
