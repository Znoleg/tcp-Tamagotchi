#include "Includes.h"

void error(char* msg)
{
    perror(msg);
    exit(1);
}

void error(string msg)
{
    error(msg.c_str());
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
