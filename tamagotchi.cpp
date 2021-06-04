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

Tamagotchi* win;
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
    zucCrBtn = findChild<QPushButton*>("ZukCreateBtn");
    sekCrBtn = findChild<QPushButton*>("SekCreateBtn");
    tamaNameRegister = findChild<QLineEdit*>("TamaName");

    tamagImage = findChild<QLabel*>("TamaImage");
    healthCnt = findChild<QLabel*>("HealthCnt");
    hungerCnt = findChild<QLabel*>("HungerCnt");
    hapinessCnt = findChild<QLabel*>("HapinessCnt");
    pissCnt = findChild<QLabel*>("PissCnt");
    sleepCnt = findChild<QLabel*>("SleepCnt");
    cureBtn = findChild<QPushButton*>("CureBtn");
    playBtn = findChild<QPushButton*>("PlayBtn");
    pissBtn = findChild<QPushButton*>("PissBtn");
    sleepBtn = findChild<QPushButton*>("SleepBtn");

    appleBtn = findChild<QPushButton*>("AppleBtn");
    cucumberBtn = findChild<QPushButton*>("CucumberBtn");
    mushroomBtn = findChild<QPushButton*>("MushroomBtn");
    meatBtn = findChild<QPushButton*>("MeatBtn");
    cheeseBtn = findChild<QPushButton*>("CheeseBtn");
    cakeBtn = findChild<QPushButton*>("CakeBtn");
    fishBtn = findChild<QPushButton*>("FishBtn");
    icecreamBtn = findChild<QPushButton*>("IcecreamBtn");

    tamagName = findChild<QLabel*>("TamagName");
    tamagMsg = findChild<QLabel*>("TamagMsg");

    connect(loginBtn, SIGNAL(released()), this, SLOT(TryLogin()));
    connect(regBtn, SIGNAL(released()), this, SLOT(SetRegisterCache()));

    connect(mamCrBtn, &QPushButton::released, this, [=](){Register(TamaTypes::Cat);} );
    connect(zucCrBtn, &QPushButton::released, this, [=](){Register(TamaTypes::Hedgehog);});
    connect(sekCrBtn, &QPushButton::released, this, [=](){Register(TamaTypes::Penguin);});

    connect(cureBtn, SIGNAL(released()), this, SLOT(TamagCure()));
    connect(playBtn, SIGNAL(released()), this, SLOT(TamagPlay()));
    connect(pissBtn, SIGNAL(released()), this, SLOT(TamagPiss()));
    connect(sleepBtn, SIGNAL(released()), this, SLOT(TamagSleep()));
    connect(appleBtn, &QPushButton::released, this, [=](){TamagFeed(FoodType::Apple);});
    connect(cucumberBtn, &QPushButton::released, this, [=](){TamagFeed(FoodType::Cucumber);});
    connect(mushroomBtn, &QPushButton::released, this, [=](){TamagFeed(FoodType::Mushroom);});
    connect(meatBtn, &QPushButton::released, this, [=](){TamagFeed(FoodType::Meat);});
    connect(cheeseBtn, &QPushButton::released, this, [=](){TamagFeed(FoodType::Cheese);});
    connect(cakeBtn, &QPushButton::released, this, [=](){TamagFeed(FoodType::Cake);});
    connect(fishBtn, &QPushButton::released, this, [=](){TamagFeed(FoodType::Fish);});
    connect(icecreamBtn, &QPushButton::released, this, [=](){TamagFeed(FoodType::Icecream);});

    win = this;
}


Tamagotchi::~Tamagotchi()
{
    _client->NotifyDisconnection();
    delete ui;
}

void Tamagotchi::HandleDisconnection()
{
    delete this;
}

void SetFrameEnable(QFrame* frame, bool status)
{
    frame->setVisible(status);
    frame->setEnabled(status);
}

TamaFeelLevel GetFeelLevel(double health, double hunger, double hapiness, double piss, double sleepness)
{
    if (health <= 0 || hunger <= 0 || hapiness <= 0 || piss <= 0 || sleepness <= 0) return TamaFeelLevel::Died;
    if (health <= 25 || hunger <= 25 || hapiness <= 25 || piss <= 25 || sleepness <= 25) return TamaFeelLevel::Worst;

    double statsCombined = health + hunger + hapiness + piss + sleepness;
    double percent = statsCombined / 500 * 100;
    if (percent <= 25) return TamaFeelLevel::Worst;
    if (percent <= 50) return TamaFeelLevel::Bad;
    if (percent <= 80) return TamaFeelLevel::Ok;
    return TamaFeelLevel::Good;
}

void* SetProcessPicture(void* arg)
{
    QString process = *static_cast<QString*>(arg);
    win->SetActionButtons(false);
    QString path = win->_tamaResourcePath + process;
    QPixmap image(path);
    win->tamagImage->setPixmap(image);
    sleep(3);
    win->tamagImage->setPixmap(win->_currentStatusImage);
    win->SetActionButtons(true);
    pthread_exit(0);
}

QPixmap Tamagotchi::SetTamaPicture(TamaFeelLevel& level)
{
    QString path = "kkk";
    switch (level)
    {
    case TamaFeelLevel::Died:
        path = _tamaResourcePath + "Died.png";
        break;

    case TamaFeelLevel::Worst:
        path = _tamaResourcePath + "Depressed.png";
        break;

    case TamaFeelLevel::Bad:
        path = _tamaResourcePath + "Sad.png";
        break;

    case TamaFeelLevel::Ok:
        path = _tamaResourcePath + "Normal.png";
        break;

    case TamaFeelLevel::Good:
        path = _tamaResourcePath + "Happy.png";
        break;

    default:
        break;
    }

    QPixmap pixmap(path);
    tamagImage->setPixmap(pixmap);
    return pixmap;
}

QPixmap Tamagotchi::SetTamaPicture(QString path, bool usingResourcePath)
{
    QString finalPath;
    if (usingResourcePath) finalPath = _tamaResourcePath + path;
    else finalPath = path;

    QPixmap image(finalPath);
    tamagImage->setPixmap(image);
    return image;
}

void Tamagotchi::UpdateStatLables(double health, double hunger, double hapiness, double piss, double sleepness)
{
    TamaFeelLevel level = GetFeelLevel(health, hunger, hapiness, piss, sleepness);
    if (level == TamaFeelLevel::Died)
    {

    }
    _currentStatusImage = SetTamaPicture(level);

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
            _tamaResourcePath = ":/images/cat/";
            break;
        case TamaTypes::Hedgehog:
            _tamaResourcePath = ":/images/zukutchi/";
            break;
        case TamaTypes::Penguin:
            _tamaResourcePath = ":/images/sekitoritchi/";
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
    string tamaName = tamaNameRegister->text().toStdString();
    if (tamaName == "")
    {
        LogWarning("Имя не может быть пустым!");
        return;
    }
    _client->ServerRegister(registerCache, tamaName, type);
    SetName(tamaName);

    registerCache = {"", ""};
    SetFrameEnable(registerFrame, false);
    SetFrameEnable(playFrame, true);
}

void Tamagotchi::SetActionButtons(bool status)
{
    cureBtn->setVisible(status);
    playBtn->setVisible(status);
    pissBtn->setVisible(status);
    sleepBtn->setVisible(status);

    appleBtn->setVisible(status);
    cheeseBtn->setVisible(status);
    cucumberBtn->setVisible(status);
    cakeBtn->setVisible(status);
    icecreamBtn->setVisible(status);
    meatBtn->setVisible(status);
    mushroomBtn->setVisible(status);
    fishBtn->setVisible(status);
}

void Tamagotchi::HandleTamaDeath()
{
    SetActionButtons(false);
    tamagMsg->setText(QString("Your pet died! You should have taken better care!"));
    sleep(5);
    SetFrameEnable(playFrame, false);
    SetFrameEnable(loginFrame, true);
}

/* Tamag Stat Buttons*/
void Tamagotchi::ChooseFood()
{

}

void Tamagotchi::TamagFeed(FoodType type)
{
    //SetProcessPicture("Eating.png");
    _client->SendEatRequest(type);
}

void Tamagotchi::TamagCure()
{
    //pthread_t thr;
//    QString act = "Cure.png";
//    pthread_create(&thr, 0, SetProcessPicture, (void**)&act);
//    pthread_join(thr, 0);
    _client->SendCureRequest();
}

void Tamagotchi::TamagPlay()
{
    //SetProcessPicture("Walking.png");
    _client->SendPlayRequest();
}

void Tamagotchi::TamagSleep()
{
    //SetProcessPicture("Sleeping.png");
    _client->SendSleepRequest();
}

void Tamagotchi::TamagPiss()
{
    //SetProcessPicture("Pissing.png");
    _client->SendPissRequest();
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
    else
    {
        label->setStyleSheet("QLabel { color: grey; }");
    }
}

void Tamagotchi::SetName(string name)
{
    tamagName->setText(QString::fromStdString(name));
}

void Tamagotchi::SetTamaMsg(string msg)
{
    tamagMsg->setText(QString::fromStdString(msg));
}
