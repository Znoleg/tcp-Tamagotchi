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
sem_t send_sem, recv_sem;

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

Client::Client()
{
    TurnOffPipeSig();
}

bool Client::ConnectToServer(in_port_t port, hostent* server)
{
    if (_sockfd == 0) error("Client error: create socket before trying connect!"); 
    _port = port;
    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    bcopy((char*)server->h_addr_list[0], (char*)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_family = AF_INET; // TCP
    serv_addr.sin_port = htons(port);

    bool connectStatus = (connect(_sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) >= 0);
    const int failConnectDelay = 10;
    for (int i = 0; !connectStatus && i < 5; i++)
    {
        connectStatus = (connect(_sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) >= 0);
        printf("Error connecting to server! Retrying after %i seconds...\n", failConnectDelay);
        sleep(failConnectDelay);
    }
    return connectStatus;
}

void Client::HandleServerDisconnection() const
{
    write(STDOUT_FILENO, "Lost connection with server!\n", 29);
    sem_destroy(&send_sem);
    sem_destroy(&recv_sem);
    tamagWindow->HandleDisconnection();
}

void Client::NotifyDisconnection()
{
    char err = char(CONNECT_LOST);
    send(_sockfd, &err, sizeof(char), 0);
    close(_sockfd);
}

bool Client::TryServerLogin(const User& user)
{
    ServerReq req = ServerReq::Login;
    string request = user.GetCredinals(); // creating Login request
    char requestBuf[127];
    bool login_result;
    strcpy(requestBuf, request.c_str());
    if (!safesend(_sockfd, &req, sizeof(ServerReq)))
    {
        HandleServerDisconnection();
        return false;
    }
    if (!safesend(_sockfd, requestBuf, strlen(requestBuf) * sizeof(char)))
    {
        HandleServerDisconnection();
        return false;
    }
    if (!saferecv(_sockfd, &login_result, sizeof(bool)))
    {
        HandleServerDisconnection();
        return false;
    }

    if (!login_result) return false;

    TamaTypes type;
    if (!saferecv(_sockfd, &type, sizeof(type)))
    {
        HandleServerDisconnection();
        return false;
    }

    tamagWindow->SetTamaType(type);

    char namebuf[64];
    if (!saferecv(_sockfd, namebuf, 64ul, 1))
    {
        HandleServerDisconnection();
        return false;
    }
    string n(namebuf, 1);
    size_t namesize = stoi(n);
    string name_str(namebuf + 1, namesize);
    tamagWindow->SetName(name_str);

    pthread_t tamStatChangeThr;
    pthread_create(&tamStatChangeThr, NULL, GetTamagStatChangeThread, (void**)this);
    _currentUser = user;
    return true;
}

void Client::ServerRegister(const User& user, const string& tamaName, TamaTypes type)
{
    ServerReq req = ServerReq::Register;
    string request = user.GetCredinals() + ' ' + tamaName; // creating Register request
    char buf[128];
    strcpy(buf, request.c_str());

    if (!safesend(_sockfd, &req, sizeof(ServerReq)))
    {
        HandleServerDisconnection();
        return;
    }
    if (!safesend(_sockfd, buf, strlen(buf) * sizeof(char)))
    {
        HandleServerDisconnection();
        return;
    }
    sleep(1);
    if (!safesend(_sockfd, &type, sizeof(TamaTypes))) // sending tamType
    {
        HandleServerDisconnection();
        return;
    }

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

    if (!safesend(_sockfd, buff, strlen(buff) * sizeof(char)))
    {
        HandleServerDisconnection();
    }
}

void Client::SendCureRequest() const
{
    ServerReq req = ServerReq::TamagCure;
    if (!safesend(_sockfd, &req, sizeof(ServerReq)))
    {
        HandleServerDisconnection();
        return;
    }
    SendUserCredinals();
}

void Client::SendEatRequest(FoodType type) const
{
    ServerReq req = ServerReq::TamagEat;
    if (!safesend(_sockfd, &req, sizeof(ServerReq)))
    {
        HandleServerDisconnection();
        return;
    }
    SendUserCredinals();
    sleep(1);
    if (!safesend(_sockfd, &type, sizeof(FoodType)))
    {
        HandleServerDisconnection();
        return;
    }

    Pleasure pleasure;
    if (!saferecv(_sockfd, &pleasure, sizeof(Pleasure), sizeof(Pleasure)))
    {
        HandleServerDisconnection();
        return;
    }

    string msg;
    switch (pleasure)
    {
    case Pleasure::Bad:
        msg = "Your pet didn't like this! :(";
        break;

    case Pleasure::OK:
        msg = "Your pet is OK with that.";
        break;

    case Pleasure::Good:
        msg = "Your pet liked this! :)";
        break;

    default:
        break;
    }

    tamagWindow->SetTamaMsg(msg);
}

void Client::SendSleepRequest() const
{
    ServerReq req = ServerReq::TamagSleep;
    send(_sockfd, &req, sizeof(ServerReq), 0);
    SendUserCredinals();
}

void Client::SendPlayRequest() const
{
    ServerReq req = ServerReq::TamagPlay;
    send(_sockfd, &req, sizeof(ServerReq), 0);
    SendUserCredinals();
}

void Client::SendPissRequest() const
{
    ServerReq req = ServerReq::TamagPiss;
    send(_sockfd, &req, sizeof(ServerReq), 0);
    SendUserCredinals();
}

void Client::HandleTamStats(const double* stats) const
{
    tamagWindow->UpdateStatLables(stats[1], stats[2], stats[3], stats[4], stats[5]);
}

void* GetTamagStatChangeThread(void* arg)
{
    Client* client = static_cast<Client*>(arg);
    int sock = client->GetSockFd();
    size_t size = (STATS_CNT + 1) * sizeof(double);
    while (true)
    {
        double stats[STATS_CNT + 1];
        if (!saferecv(sock, stats, size, size))
        {
            client->HandleServerDisconnection();
            pthread_exit(0);
        }

        client->HandleTamStats(stats);
        if (stats[0] == 1)
        {
            tamagWindow->HandleTamaDeath();
            pthread_exit(0);
        }
        //sleep(5);
    }
}

bool ConnectByIpStr(Client* client, string adr_str, bool showWindow)
{
    size_t index = adr_str.find(":");
    if (index == string::npos)
    {
        printf("Please provide adress in ip:port format!\n");
        return false;
    }

    string ip_str(adr_str, 0, index);
    string port_str(adr_str.begin() + index + 1, adr_str.end());

    hostent *server = get_host_by_ip(ip_str);
    in_port_t port = stoi(port_str);

    if (showWindow) tamagWindow->show();
    return client->ConnectToServer(port, server);
}

void* CommandsThread(void* arg)
{
    Client* client = static_cast<Client*>(arg);
    while (true)
    {
        char cmd_buf[32];
        scanf("%s", cmd_buf);
        if (strcmp(cmd_buf, "connect") == 0)
        {
            char ip_buf[32];
            scanf("%s", ip_buf);

            if (!ConnectByIpStr(client, ip_buf, true))
            {
                printf("Error connection to server %s", ip_buf);
                continue;
            }
        }
    }
}

int main(int argc, char* argv[])
{
    auto client = new Client();
    client->CreateSocket();
    bool connected = false;

    if (argc >= 2) // Hostname & portno provided as args
    {
        char ip_buf[32];
        string ip = argv[1];
        strcpy(ip_buf, ip.c_str());
        if (!ConnectByIpStr(client, ip_buf, false))
        {
            printf("Error connecting to server %s\n", ip_buf);
        }
        else connected = true;
    }
    else
    {
        char ip_buf[32];
        string ip = "127.0.0.1:2050";
        strcpy(ip_buf, ip.c_str());
        if (!ConnectByIpStr(client, ip_buf, false))
        {
            printf("Error connecting to server %s\n", "127.0.0.1:2050");
        }
        else connected = true;
    }

    sem_init(&recv_sem, 0, 1);
    sem_init(&send_sem, 0, 1);

    pthread_t cmd_thr;
    pthread_create(&cmd_thr, 0, CommandsThread, (void*)&client);

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
    tamagWindow = &w;
    if (connected) w.show();
    return a.exec();
}
