#include "Client.h"
#include "../tamagotchi.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

#include <cstring>
#include <stdio.h>
#include <string>
#include <iostream>

void* GetTamagStatChangeThread(void* arg);

Tamagotchi* tamagWindow;

#define STATS_CNT 5

vector<double> SplitStringToNumbers(const string& str, const string& delim)
{
    vector<double> dValues;
    vector<string> doubleStrings = SplitString(str, delim);
    for (auto dStr : doubleStrings)
    {
        dValues.push_back(stod(dStr));
    }
    return dValues;
}

hostent* get_host_by_ip(string ipstr = "127.0.0.1")
{
    in_addr ip;
    hostent *hp;
    if (!inet_aton(ipstr.c_str(), &ip)) error("Can't parse IP address!");
    if ((hp = gethostbyaddr((const void*)&ip, sizeof(ip), AF_INET)) == NULL) error("No server associated with " + ipstr);
    return hp;
}

sockaddr_in Client::ConnectToServer(in_port_t port, hostent* server)
{
    if (_sockfd == 0) error("Client error: create socket before trying connect!"); 
    _port = port;
    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    bcopy((char*)server->h_addr_list[0], (char*)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_family = AF_INET; // TCP
    serv_addr.sin_port = htons(port);

    const int failConnectDelay = 10;
    while (connect(_sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Error connecting to server! Retrying after %i seconds...\n", failConnectDelay);
        sleep(failConnectDelay);
    }
    return serv_addr;
}

bool Client::TryServerLogin(const User& user)
{
    ServerReq req = ServerReq::Login;
    string request = user.GetCredinals(); // creating Login request
    char requestBuf[127];
    bool login_result;
    strcpy(requestBuf, request.c_str());
    
    send(_sockfd, &req, sizeof(ServerReq), 0);
    send(_sockfd, requestBuf, strlen(requestBuf) * sizeof(char), 0); // sending request
    recv(_sockfd, &login_result, sizeof(bool), 0);

    if (!login_result) return false;

    TamaTypes type;
    recv(_sockfd, &type, sizeof(type), 0);
    tamagWindow->SetTamaType(type);

    pthread_t tamStatChangeThr;
    pthread_create(&tamStatChangeThr, NULL, GetTamagStatChangeThread, (void**)this);
    _currentUser = user;
    return true;
}

void Client::ServerRegister(const User& user, const string& tamaName, const TamaTypes type)
{
    ServerReq req = ServerReq::Register;
    string request = user.GetCredinals() + ' ' + tamaName; // creating Register request
    char buf[128];
    strcpy(buf, request.c_str());

    send(_sockfd, &req, sizeof(ServerReq), 0);
    send(_sockfd, buf, strlen(buf) * sizeof(char), 0); // sending request
    sleep(1);
    send(_sockfd, &type, sizeof(TamaTypes), 0); // sending tamType
    tamagWindow->SetTamaType(type);
    
    pthread_t tamStatChangeThr;
    pthread_create(&tamStatChangeThr, NULL, GetTamagStatChangeThread, (void**)this);
    _currentUser = user;
}

void Client::SendUserCredinals() const
{
    string credinals = _currentUser.GetCredinals();
    char buff[credinals.size()];
    strcpy(buff, credinals.c_str());

    send(_sockfd, buff, strlen(buff) * sizeof(char), 0);
}

void Client::SendCureRequest() const
{
    ServerReq req = ServerReq::TamagCure;
    send(_sockfd, &req, sizeof(ServerReq), 0);
    SendUserCredinals();
}

void Client::SendEatRequest(FoodType type) const
{
    ServerReq req = ServerReq::TamagEat;
    send(_sockfd, &req, sizeof(ServerReq), 0);
    SendUserCredinals();
    send(_sockfd, &type, sizeof(FoodType), 0);
}

void Client::SendSleepRequest() const
{
    ServerReq req = ServerReq::TamagSleep;
    send(_sockfd, &req, sizeof(ServerReq), 0);
    SendUserCredinals();
}

void Client::SendPlayRequest() const
{
    send(_sockfd, (void*)ServerReq::TamagPlay, sizeof(ServerReq), 0);
    SendUserCredinals();
}

void Client::SendPissRequest() const
{
    send(_sockfd, (void*)ServerReq::TamagPiss, sizeof(ServerReq), 0);
    SendUserCredinals();
}

void Client::HandleTamStats(const double* stats) const
{
    tamagWindow->UpdateStatLables(stats[0], stats[1], stats[2], stats[3], stats[4]);
}

void* GetTamagStatChangeThread(void* arg)
{
    Client* client = static_cast<Client*>(arg);
    int sock = client->GetSockFd();
    size_t size = STATS_CNT * sizeof(double);
    while (true)
    {
        double stats[STATS_CNT];
        if (recv(sock, stats, size, 0) <= 0) continue;

        client->HandleTamStats(stats);
    }
}

int main(int argc, char* argv[])
{
    in_port_t portno;
    hostent *server;
    if (argc >= 3) // Hostname & portno provided as args
    {
//        string hostName = argv[1];
//        server = GetHostByName(hostName);
//        if (server == NULL) error("Console argument error: no " + hostName + " host!");
//        portno = atoi(argv[2]);
    }
    else // Not provided - using standart
    {
        server = get_host_by_ip();
        portno = STD_PORT;
    }

    auto client = new Client();
    client->CreateSocket();
    client->ConnectToServer(portno, server);


    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "Tamagochi_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    Tamagotchi w(client);
    w.show();
    tamagWindow = &w;
    return a.exec();
}
