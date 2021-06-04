#include "Includes.h"

void error(string msg)
{
    char m[msg.size()];
    strcpy(m, msg.c_str());
    error(m);
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

SockConnection::SockConnection()
{
    sem_init(&_sendSem, 0, 1);
    sem_init(&_recvSem, 0, 1);
}

SockConnection::~SockConnection()
{
    sem_destroy(&_sendSem);
    sem_destroy(&_recvSem);
}

bool SockConnection::safesend(int sock, void* packet, ssize_t packet_size)
{
    //sem_wait(&_sendSem);
    ssize_t size = send(sock, packet, packet_size, MSG_NOSIGNAL);
    //sem_post(&_sendSem);

    if (size < packet_size) return false;
    return true;
}

bool SockConnection::saferecv(int sock, void* packet, ssize_t max_packet_size, ssize_t min_packet_size)
{
    //sem_wait(&_recvSem);
    ssize_t size = recv(sock, packet, max_packet_size, 0);
    //sem_post(&_recvSem);

    if (size < min_packet_size) return false;

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
//    if (setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
//        error("Server error: error on setsockopt");
    return _sockfd;
}

void SockConnection::TurnOffPipeSig()
{
    // struct sigaction new_actn, old_actn;
    // new_actn.sa_handler = SIG_IGN;
    // sigemptyset(&new_actn.sa_mask);
    // new_actn.sa_flags = 0;
    // sigaction(SIGPIPE, &new_actn, &old_actn);

    // _oldPipeActn = old_actn;
}

void SockConnection::RestorePipeSig()
{
    sigaction(SIGPIPE, &_oldPipeActn, NULL);
}
