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

bool Client::TryServerLogin(const User& user) const
{
    ServerReq req = ServerReq::Login;
    string request = user.name + ' ' + user.password; // creating Login request
    char requestBuf[127];
    char buff[1];
    TamaTypes type;
    strcpy(requestBuf, request.c_str());
    
    send(_sockfd, &req, 4ul, 0);
    send(_sockfd, &requestBuf, 127ul, 0); // sending request
    sleep(2);
    while (recv(_sockfd, buff, 127ul, 0) <= 0);

    if (buff[0] == 'F') return false;

    recv(_sockfd, &type, 4ul, 0);
    tamagWindow->SetTamaType(type);

    pthread_t tamStatChangeThr;
    pthread_create(&tamStatChangeThr, NULL, GetTamagStatChangeThread, (void**)this);
    return true;
}

void Client::ServerRegister(const User& user, const string& tamaName, const TamaTypes type)
{
    ServerReq req = ServerReq::Register;
    string request = user.name + " " + user.password + " " + tamaName; // creating Register request
    char buf[128];
    strcpy(buf, request.c_str());

    send(_sockfd, &req, sizeof(&req), 0);
    send(_sockfd, buf, 127, 0); // sending request
    send(_sockfd, &type, sizeof(&type), 0); // sending tamType
    tamagWindow->SetTamaType(type);
    
    pthread_t tamStatChangeThr;
    pthread_create(&tamStatChangeThr, NULL, GetTamagStatChangeThread, (void**)this);
}

void Client::SendTamRequest(const ServerReq req) const
{
    if (req != ServerReq::TamagCure && req != ServerReq::TamagEat && req != ServerReq::TamagPiss && req != ServerReq::TamagPlay && req != ServerReq::TamagSleep)
        return;

    send(_sockfd, &req, 1, 0);
    
    char reqstr[128];
    recv(_sockfd, reqstr, 127, 0); // getting statst
    vector<double> stats = SplitStringToNumbers(reqstr, " ");
    printf("");
}

void Client::HandleTamStats(const double* stats) const
{
    tamagWindow->UpdateStatLables(stats[0], stats[1], stats[2], stats[3], stats[4]);
}

void* GetTamagStatChangeThread(void* arg)
{
    Client* client = static_cast<Client*>(arg);
    int sock = client->GetSockFd();
    double* stats = new double[STATS_CNT];
    while (true)
    {
        if (recv(sock, stats, 127ul, 0) <= 0) continue;

        client->HandleTamStats(stats);
        memset(stats, 0, 5ul);
        printf("\n");
    }
}

int main(int argc, char* argv[])
{
    int portno;
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
    int sockfd = client->CreateSocket();
    sockaddr_in serv_addr = client->ConnectToServer(portno, server);


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
