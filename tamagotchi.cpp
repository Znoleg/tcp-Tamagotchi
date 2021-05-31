#include "tamagotchi.h"
#include "ui_tamagotchi.h"

#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QLineEdit>
#include <QFrame>
#include <QSignalMapper>
#include <QPixmap>

User registerCache;

void SetFrameEnable(QFrame* frame, bool status);

Tamagotchi::Tamagotchi(Client* client, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Tamagotchi)
{
    ui->setupUi(this);
    _client = client;

    loginFrame = findChild<QFrame*>("LoginWindow");
    registerFrame = findChild<QFrame*>("RegisterWindow");
    playFrame = findChild<QFrame*>("PlayWindow");
    SetFrameEnable(registerFrame, false);
    SetFrameEnable(playFrame, false);

    logWarningLabel = findChild<QLabel*>("WarningLabel");
    loginBtn = findChild<QPushButton*>("LoginBtn");
    regBtn = findChild<QPushButton*>("RegisterBtn");
    loginField = findChild<QLineEdit*>("LoginField");
    passField = findChild<QLineEdit*>("PassField");

    mamCrBtn = findChild<QPushButton*>("MamCreateBtn");
    zucCrBtn = findChild<QPushButton*>("ZucCreateBtn");
    sekCrBtn = findChild<QPushButton*>("SekCreateBtn");
    tamaNameField = findChild<QLineEdit*>("TamaName");

    tamagImage = findChild<QLabel*>("TamaImage");
    healthCnt = findChild<QLabel*>("HealthCnt");
    hungerCnt = findChild<QLabel*>("HungerCnt");
    hapinessCnt = findChild<QLabel*>("HapinessCnt");
    pissCnt = findChild<QLabel*>("PissCnt");
    sleepCnt = findChild<QLabel*>("SleepCnt");
    cureBtn = findChild<QPushButton*>("CureBtn");
    feedBtn = findChild<QPushButton*>("FeedBtn");
    walkBtn = findChild<QPushButton*>("WalkBtn");
    pissBtn = findChild<QPushButton*>("PissBtn");
    sleepBtn = findChild<QPushButton*>("SleepBtn");

    connect(loginBtn, SIGNAL(released()), this, SLOT(TryLogin()));
    connect(regBtn, SIGNAL(released()), this, SLOT(SetRegisterCache()));

    connect(mamCrBtn, &QPushButton::released, this, [=](){Register(TamaTypes::Cat);} );
    connect(zucCrBtn, &QPushButton::released, this, [=](){Register(TamaTypes::Hedgehog);});
    connect(sekCrBtn, &QPushButton::released, this, [=](){Register(TamaTypes::Penguin);});
}

Tamagotchi::~Tamagotchi()
{
    delete ui;
}

void SetFrameEnable(QFrame* frame, bool status)
{
    frame->setVisible(status);
    frame->setEnabled(status);
}

TamaFeelLevel GetFeelLevel(double health, double hunger, double hapiness, double piss, double sleepness)
{
    if (health <= 0 || hunger <= 0 || hapiness <= 0 || piss <= 0 || sleepness <= 0) return TamaFeelLevel::Died;
    if (health <= 20 || hunger <= 20 || hapiness <= 20 || piss <= 20 || sleepness <= 20) return TamaFeelLevel::Worst;

    double statsCombined = health + hunger + hapiness + piss + sleepness;
    double percent = statsCombined / 500 * 100;
    if (percent <= 25) return TamaFeelLevel::Worst;
    if (percent <= 50) return TamaFeelLevel::Bad;
    if (percent <= 75) return TamaFeelLevel::Ok;
    return TamaFeelLevel::Good;
}

void Tamagotchi::SetTamaPicture(TamaFeelLevel& level)
{
    QString path = "kkk";
    switch (level)
    {
    case TamaFeelLevel::Died:
        path = *_tamaResourcePath + "Died.png";
        break;

    case TamaFeelLevel::Worst:
        path = *_tamaResourcePath + "Depressed.png";
        break;

    case TamaFeelLevel::Bad:
        path = *_tamaResourcePath + "Sad.png";
        break;

    case TamaFeelLevel::Ok:
        path = *_tamaResourcePath + "Normal.png";
        break;

    case TamaFeelLevel::Good:
        path = *_tamaResourcePath + "Happy.png";
        break;

    default:
        break;
    }

    QPixmap pixmap(":/images/cat/");
    tamagImage->setPixmap(pixmap);
}

void Tamagotchi::SetTamaPicture(QString path, bool usingResourcePath)
{
    QString finalPath;
    if (usingResourcePath) finalPath = *_tamaResourcePath + path;
    else finalPath = path;

    QPixmap image(finalPath);
    tamagImage->setPixmap(image);
}

void Tamagotchi::UpdateStatLables(double health, double hunger, double hapiness, double piss, double sleepness)
{
    TamaFeelLevel level = GetFeelLevel(health, hunger, hapiness, piss, sleepness);
    if (level == TamaFeelLevel::Died)
    {

    }
    SetTamaPicture(":/images/cat/Happy.png", false);

    SetStat(health, healthCnt);
    SetStat(hunger, hungerCnt);
    SetStat(hapiness, hapinessCnt);
    SetStat(piss, pissCnt);
    SetStat(sleepness, sleepCnt);
}

void Tamagotchi::SetTamaType(const TamaTypes type)
{
    switch (type)
    {
        case TamaTypes::Cat:
            _tamaResourcePath = new QString(":/images/cat/");
            break;
        case TamaTypes::Hedgehog:
            _tamaResourcePath = new QString(":/images/zukutchi/");
            break;
        case TamaTypes::Penguin:
            _tamaResourcePath = new QString(":/images/sekitoritchi/");
            break;

        default: break;
    }
}

void Tamagotchi::TryLogin()
{
    string login = loginField->text().toStdString();
    string password = passField->text().toStdString();
    if (!_client->TryServerLogin({login, password}))
    {
        LogWarning("Ошибка входа! Проверьте данные!");
        return;
    }
    SetFrameEnable(loginFrame, false);
    SetFrameEnable(playFrame, true);
}

void Tamagotchi::SetRegisterCache()
{
    string login = loginField->text().toStdString();
    string password = passField->text().toStdString();
    if (login == "" || password == "")
    {
        LogWarning("Логин и пароль не могут быть пустыми!");
        return;
    }
    registerCache = {login, password};
    SetFrameEnable(loginFrame, false);
    SetFrameEnable(registerFrame, true);
}

void Tamagotchi::Register(TamaTypes type)
{
    string tamaName = tamaNameField->text().toStdString();
    if (tamaName == "")
    {
        LogWarning("Имя не может быть пустым!");
        return;
    }
    _client->ServerRegister(registerCache, tamaName, type);
    registerCache = {"", ""};
    SetFrameEnable(registerFrame, false);
    SetFrameEnable(playFrame, true);
}

void Tamagotchi::LogWarning(const QString& text)
{
    logWarningLabel->setText(text);
}

void Tamagotchi::SetStat(const double statValue, QLabel* label)
{
    label->setText(QString::number(statValue));
    if (statValue <= 25)
    {
        label->setStyleSheet("QLabel { color: red; }");
    }
}
