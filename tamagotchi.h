#ifndef TAMAGOTCHI_H
#define TAMAGOTCHI_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include "Client/Client.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Tamagotchi; }
QT_END_NAMESPACE

enum class TamaFeelLevel
{
    Died, Worst, Bad, Ok, Good
};

class Tamagotchi : public QMainWindow
{
    Q_OBJECT

public:
    Tamagotchi(Client* client, QWidget *parent = nullptr);
    ~Tamagotchi();
    void UpdateStatLables(double health, double hunger, double hapiness, double piss, double sleepness);
    void SetTamaType(const TamaTypes type);
    void SetName(string name);
    void HandleDisconnection();
    void HandleTamaDeath();
    void SetTamaMsg(string msg);

private:
    Ui::Tamagotchi *ui;
    Client* _client;
    QString _tamaResourcePath;

    QFrame* loginFrame, *registerFrame, *playFrame;
    QLabel* logWarningLabel, *healthCnt, *hungerCnt, *hapinessCnt, *pissCnt, *sleepCnt, *tamagImage, *tamagName, *tamagMsg;
    QPushButton* loginBtn, *regBtn, *mamCrBtn, *zucCrBtn, *sekCrBtn, *cureBtn, *playBtn, *pissBtn, *sleepBtn;
    QPushButton* appleBtn, *cucumberBtn, *mushroomBtn, *meatBtn, *cheeseBtn, *cakeBtn, *fishBtn, *icecreamBtn;
    QLineEdit* loginField, *passField, *tamaNameRegister;
    QPixmap _currentStatusImage;

    void LogWarning(const QString& text);
    QPixmap SetTamaPicture(TamaFeelLevel& level);
    QPixmap SetTamaPicture(QString path, bool usingResourcePath = true);
    void SetActionButtons(bool status);
    friend void* SetProcessPicture(void* arg);

private slots:
    void TryLogin();
    void SetRegisterCache();
    void Register(TamaTypes type);
    void SetStat(const double statValue, QLabel* label);

    void ChooseFood();
    void TamagFeed(FoodType type);
    void TamagCure();
    void TamagSleep();
    void TamagPlay();
    void TamagPiss();
};
#endif // TAMAGOTCHI_H
