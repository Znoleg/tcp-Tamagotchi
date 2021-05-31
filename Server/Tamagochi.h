#pragma once
#include "../Includes.cpp"
#include <vector>

enum class TamStats
{
	Health, Food, Sleep, Piss, Hapiness
};

enum class Food
{
	Apple, Cucumber, Mushroom, Meat, Cheese, Cake, Fish, Ice
};

enum class Pleasure
{
	Good, OK, Bad 
};

typedef pair<Food, Pleasure> FoodPleasure;

class TamagochiController
{
	friend class Server;
	void virtual SetStatValue(TamStats stat, double value) = 0;
	bool virtual ChangeStatValue(TamStats stat, double delta) = 0;
	bool virtual ChangeStatValues() = 0;
	void virtual DoLifeIteration() = 0;
	void virtual FeedAnimal(Food) = 0;
	void virtual PlayWithAnimal() = 0;
	void virtual CureAnimal() = 0;
	void virtual SleepWithAnimal() = 0;
	void virtual WalkWithAnimal() = 0;
};

class Tamagochi : TamagochiController
{
public:
	Tamagochi(std::string name, double startValues);
	string GetName() const;
	int GetStatValue(TamStats stat) const;
	TamaTypes GetType() const;
	double* GetStats() const;
	void SetStatValue(TamStats stat, double value) override;
	bool ChangeStatValue(TamStats stat, double delta) override;
	bool ChangeStatValues() override;
	void virtual DoLifeIteration() override;
	void virtual FeedAnimal(Food) override;
	void virtual PlayWithAnimal() override;
	void virtual CureAnimal() override;
	void virtual SleepWithAnimal() override;
	void virtual WalkWithAnimal() override;
	int GetStatsCount() const;

	friend ostream& operator<<(ostream& os, const Tamagochi& tama);
	friend istream& operator>>(istream& is, Tamagochi& tama);
protected:
	map<Food, Pleasure>* _foodPleasure = new map<Food, Pleasure>();
	void ConstructFoodPleasureMap(const FoodPleasure pair, ...);
	void virtual FillFoodPleasureMap() {};
	//void virtual _spriteList;
	map<TamStats, double>* _statusValue = new map<TamStats, double>();
	TamaTypes _tamType;
private:
	Tamagochi();
	string _name;	
};

class Penguin final : public Tamagochi
{
public:
    Penguin(std::string name);
protected:
    void FillFoodPleasureMap() override final;
};

class Cat final : public Tamagochi
{
public:
    Cat(std::string name);
protected:
    void FillFoodPleasureMap() override final;
};

class Hedgehog final : public Tamagochi
{
public:
    Hedgehog(std::string name);
protected:
    void FillFoodPleasureMap() override final;
};

