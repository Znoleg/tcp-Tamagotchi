#include "Tamagochi.h"

Tamagochi::Tamagochi() {}

Tamagochi::Tamagochi(string name, double startValues = 100.0)
{
    _name = name;
    _statusValue->insert(pair<TamStats, double>(TamStats::Health, startValues));
    _statusValue->insert(pair<TamStats, double>(TamStats::Food, startValues));
    _statusValue->insert(pair<TamStats, double>(TamStats::Hapiness, startValues));
    _statusValue->insert(pair<TamStats, double>(TamStats::Piss, startValues));
    _statusValue->insert(pair<TamStats, double>(TamStats::Sleep, startValues));
}

string Tamagochi::GetName() const
{
    return _name;
}

TamaTypes Tamagochi::GetType() const
{
    return _tamType;
}

double* Tamagochi::GetStats() const
{
    string stats;
    size_t arrSize = _statusValue->size();
    double* array = new double[arrSize];
    int i = 0;
    for (map<TamStats, double>::iterator it = _statusValue->begin(); it != _statusValue->end(); it++, i++)
    {
        array[i] = it->second;
    }
    return array;
}

int Tamagochi::GetStatValue(TamStats stat) const
{
    return _statusValue->find(stat)->second;
}

void Tamagochi::SetStatValue(TamStats stat, double value)
{
    if (value < 0) return;
    _statusValue->find(stat)->second = value;
}

// Returns false if stat <= 0
bool Tamagochi::ChangeStatValue(TamStats stat, double delta)
{
    auto it = _statusValue->find(stat);
    it->second += delta;
    return it->second > 0;
}

bool Tamagochi::ChangeStatValues()
{
    return true;
}

void Tamagochi::FeedAnimal(Food food)
{
    const double sleepDelta = -5;
    const double pissDelta = -10;
    const double foodDelta = 15;
    const double hapinessDelta = 5;
    auto pleasure = _foodPleasure->find(food)->second;
    ChangeStatValue(TamStats::Sleep, sleepDelta);
    ChangeStatValue(TamStats::Piss, pissDelta);

    switch (pleasure)
    {
    case Pleasure::Good:
        ChangeStatValue(TamStats::Food, foodDelta * 2);
        ChangeStatValue(TamStats::Hapiness, hapinessDelta * 2);
        break;
    case Pleasure::OK:
        ChangeStatValue(TamStats::Food, foodDelta);
        ChangeStatValue(TamStats::Hapiness, hapinessDelta);
        break;
    case Pleasure::Bad:
        ChangeStatValue(TamStats::Hapiness, -hapinessDelta * 2);
        ChangeStatValue(TamStats::Health, -15);
        break;

    default:
        break;
    }
}

void Tamagochi::DoLifeIteration()
{
    const double delta = -1;
    ChangeStatValue(TamStats::Health, delta);
    ChangeStatValue(TamStats::Food, delta);
    ChangeStatValue(TamStats::Sleep, delta);
    ChangeStatValue(TamStats::Piss, delta);
    ChangeStatValue(TamStats::Hapiness, delta);
}

void Tamagochi::PlayWithAnimal()
{
    const double hapinessDelta = 20;
    const double pissDelta = -10;
    const double foodDelta = -20;
    const double healthDelta = 5;
    const double sleepDelta = -20;
    ChangeStatValue(TamStats::Hapiness, hapinessDelta);
    ChangeStatValue(TamStats::Food, foodDelta);
    ChangeStatValue(TamStats::Piss, pissDelta);
    ChangeStatValue(TamStats::Health, healthDelta);
    ChangeStatValue(TamStats::Sleep, sleepDelta);
}

void Tamagochi::CureAnimal()
{
    const double pissDelta = -10;
    const double healthDelta = 20;
    const double hapinessDelta = -(rand() % 20 + 15); // -30 -15
    const double foodDelta = -(rand() % 5 + 5); // -10 -5
    const double sleepDelta = -5;
    ChangeStatValue(TamStats::Health, healthDelta);
    ChangeStatValue(TamStats::Hapiness, hapinessDelta);
    ChangeStatValue(TamStats::Food, foodDelta);
    ChangeStatValue(TamStats::Sleep, sleepDelta);
    ChangeStatValue(TamStats::Piss, pissDelta);
}

void Tamagochi::SleepWithAnimal()
{
    const double sleepDelta = 30;
    const double foodDelta = -(rand() % 10 + 20); // -30 -20
    const double pissDelta = -10;
    const double hapinessDelta = -(rand() % 5 + 5); // -10 -5;
    ChangeStatValue(TamStats::Sleep, sleepDelta);
    ChangeStatValue(TamStats::Food, foodDelta);
    ChangeStatValue(TamStats::Piss, pissDelta);
    ChangeStatValue(TamStats::Hapiness, hapinessDelta);
}

void Tamagochi::WalkWithAnimal()
{
    const double pissDelta = 30;
    const double hapinessDelta = 5;
    const double foodDelta = -(rand() % 5 + 5); // -10 - 5
    const double healthDelta = -(rand() % 15 + 10); // -25 -10
    ChangeStatValue(TamStats::Piss, pissDelta);
    ChangeStatValue(TamStats::Hapiness, hapinessDelta);
    ChangeStatValue(TamStats::Food, foodDelta);
    ChangeStatValue(TamStats::Health, healthDelta);
}

ostream& operator<<(ostream& os, const Tamagochi& tama)
{
    os << tama._name  << ' '
        << tama._statusValue->find(TamStats::Health)->second << ' '
        << tama._statusValue->find(TamStats::Food)->second << ' '
        << tama._statusValue->find(TamStats::Hapiness)->second << ' '
        << tama._statusValue->find(TamStats::Piss)->second << ' '
        << tama._statusValue->find(TamStats::Sleep)->second;
    return os;
}

istream& operator>>(istream& is, Tamagochi& tama)
{
    double stats[5];
    is >> tama._name >> stats[0] >> stats[1] >> stats[2]
         >> stats[3] >> stats[4];
    tama._statusValue->find(TamStats::Health)->second = stats[0];
    tama._statusValue->find(TamStats::Food)->second = stats[1];
    tama._statusValue->find(TamStats::Hapiness)->second = stats[2];
    tama._statusValue->find(TamStats::Piss)->second = stats[3];
    tama._statusValue->find(TamStats::Sleep)->second = stats[4];
    return is;
}

void Tamagochi::ConstructFoodPleasureMap(const FoodPleasure pair, ...)
{
}

int Tamagochi::GetStatsCount() const
{
    return _statusValue->size();
}

Penguin::Penguin(std::string name) : Tamagochi(name, 80)
{
    _tamType = TamaTypes::Penguin;
}

void Penguin::FillFoodPleasureMap()
{
    _foodPleasure->insert(pair<Food, Pleasure>(Food::Apple, Pleasure::OK));
    _foodPleasure->insert(pair<Food, Pleasure>(Food::Cucumber, Pleasure::Bad));
    _foodPleasure->insert(pair<Food, Pleasure>(Food::Mushroom, Pleasure::Bad));
    _foodPleasure->insert(pair<Food, Pleasure>(Food::Meat, Pleasure::OK));
    _foodPleasure->insert(pair<Food, Pleasure>(Food::Cheese, Pleasure::Bad));
    _foodPleasure->insert(pair<Food, Pleasure>(Food::Cake, Pleasure::OK));
    _foodPleasure->insert(pair<Food, Pleasure>(Food::Fish, Pleasure::Good));
    _foodPleasure->insert(pair<Food, Pleasure>(Food::Ice, Pleasure::Good));
}

Cat::Cat(std::string name) : Tamagochi(name, 120.0)
{
    _tamType = TamaTypes::Cat;
}

void Cat::FillFoodPleasureMap()
{
    _foodPleasure->insert(pair<Food, Pleasure>(Food::Apple, Pleasure::OK));
    _foodPleasure->insert(pair<Food, Pleasure>(Food::Cucumber, Pleasure::OK));
    _foodPleasure->insert(pair<Food, Pleasure>(Food::Mushroom, Pleasure::OK));
    _foodPleasure->insert(pair<Food, Pleasure>(Food::Meat, Pleasure::Good));
    _foodPleasure->insert(pair<Food, Pleasure>(Food::Cheese, Pleasure::OK));
    _foodPleasure->insert(pair<Food, Pleasure>(Food::Cake, Pleasure::Good));
    _foodPleasure->insert(pair<Food, Pleasure>(Food::Fish, Pleasure::Good));
    _foodPleasure->insert(pair<Food, Pleasure>(Food::Ice, Pleasure::Bad));
}

Hedgehog::Hedgehog(std::string name) : Tamagochi(name, 100.0)
{
    _tamType = TamaTypes::Hedgehog;
}

void Hedgehog::FillFoodPleasureMap()
{
    _foodPleasure->insert(pair<Food, Pleasure>(Food::Apple, Pleasure::Good));
    _foodPleasure->insert(pair<Food, Pleasure>(Food::Cucumber, Pleasure::OK));
    _foodPleasure->insert(pair<Food, Pleasure>(Food::Mushroom, Pleasure::Good));
    _foodPleasure->insert(pair<Food, Pleasure>(Food::Meat, Pleasure::Bad));
    _foodPleasure->insert(pair<Food, Pleasure>(Food::Cheese, Pleasure::OK));
    _foodPleasure->insert(pair<Food, Pleasure>(Food::Cake, Pleasure::OK));
    _foodPleasure->insert(pair<Food, Pleasure>(Food::Fish, Pleasure::Bad));
    _foodPleasure->insert(pair<Food, Pleasure>(Food::Ice, Pleasure::Bad));
}
