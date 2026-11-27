#include <iostream>
#include <Windows.h>
#include <conio.h>

struct Bayum
{
	long long health = 2200000;
	int resist = 44;
	int damage = 73843;
	int specialDamage = 150000;
	int attackCooldown = 5;
	int specialCooldown = 10;
};

struct Player
{
	long long health = 500000;
	int damage = 12000;
	int specialDamage = 30000;
	int attackCooldown = 2;
	int specialCooldown = 5;
	int defense = 20;
	int dodgeChance = 15;
	char name[64];
};

DWORD WINAPI BossThread(LPVOID)
{
	DWORD wait = WaitForSingleObject(hMutex, 2000);

	int idPlayer = rand() % 10;

	int chance = rand() % 101;

	if (chance < 15)
	{
		players[idPlayer]->health -= boss->damage;
		std::cout << "Босс нанес урон игроку номер: " << idPlayer << std::endl;
	}
	else
	{
		std::cout << "Игрок увернулся от удара Босса\n";
	}

	double effectiveDamage = boss->specialDamage * (1 - 0.05 * (pCount - 1));

	for (int i = 0; i < pCount; i++)
	{
		chance = rand() % 1;

		if (chance == 0)
		{
			players[i]->health -= effectiveDamage;
		}
		else
		{
			std::cout << "Игрок увернулся от особой атаки Босса\n";
		}
	}
	std::cout << "Босс нанес особый урон всем игрокам, которые не смогли увернуться\n";

	ReleaseMutex(hMutex);

	return 0;
}

DWORD WINAPI PlayerThread(LPVOID)
{	
	DWORD wait = WaitForSingleObject(hMutex, 2000);

	double effectiveDamage = players[0]->damage * (100 - boss->resist) / 100;

	boss->health -= effectiveDamage;
	std::cout << "Игрок нанес урон\n";

	effectiveDamage = players[0]->specialDamage * (100 - boss->resist) / 100;

	boss->health -= effectiveDamage;		
	std::cout << "Игрок нанес особый урон\n";

	ReleaseMutex(hMutex);

	return 0;
}

Player* players[10];
int pCount;
Bayum* boss;

HANDLE hMutex;
HANDLE hEvent;

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "Russian");
	srand(time(NULL));

	if (argc < 3)
	{
		std::cout << "Ошибка: не переданы аргументы\n";
		return -1;
	}	

	int countPlayers = atoi(argv[1]);
	pCount = countPlayers;

	hMutex = CreateMutex(NULL, FALSE, NULL);
	if (hMutex == NULL)
	{
		std::cout << "Ошибка создания мюьтекса\n";
		return GetLastError();
	}

	hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hEvent == NULL)
	{
		std::cout << "Ошибка создания события\n";
		return GetLastError();
	}

	HANDLE hThreadBoss;
	DWORD IDThread;

	hThreadBoss = CreateThread(NULL, 0, BossThread, NULL, 0, &IDThread);
	if (hThreadBoss == NULL)
	{
		std::cout << "Ошибка создания потока\n";
		return GetLastError();
	}

	WaitForSingleObject(hThreadBoss, INFINITE);

	HANDLE threadsPLayers[10];

	for (int i = 0; i < countPlayers; i++)
	{
		threadsPLayers[i] = CreateThread(NULL, 0, PlayerThread, NULL, 0, NULL);
	}

	WaitForMultipleObjects(countPlayers, threadsPLayers, TRUE, INFINITE);

	DWORD timeGame = GetTickCount();



	CloseHandle(hMutex);
	CloseHandle(hEvent);
	CloseHandle(hThreadBoss);

	for (int i = 0; i < countPlayers; i++)
	{
		CloseHandle(threadsPLayers[i]);
	}	

	return 0;
}