#include <iostream>
#include <Windows.h>
#include <conio.h>
#include <ctime>
#include <cstdio>

struct Bayum
{
	long long health = 2200000;
	int resist = 44;
	int damage = 73843;
	int specialDamage = 150000;
	int attackCooldown = 5;
	int specialCooldown = 10;

	bool alive = true;
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

	long long totalDamage = 0;
	bool alive = true;
};

Player* players[10];
int pCount;
Bayum* boss;
bool stoppedEarly = false;
long long pauseTime = 0;

HANDLE hMutex;
HANDLE hStartEvent;
HANDLE hPauseEvent;
HANDLE hStopEvent;
HANDLE hBossAttackEvent;
HANDLE hBossSpecialEvent;

int bossTarget = -1;
int specialHandled = 0;
int specialAliveCount = 0;

DWORD specialSetTime = 0;


DWORD WINAPI BossThread(LPVOID)
{
	WaitForSingleObject(hStartEvent, INFINITE);

	DWORD startTick = GetTickCount();
	long long nextAttack = 0;
	long long nextSpecial = 0;

	while (boss->alive)
	{
		if (WaitForSingleObject(hStopEvent, 0) == WAIT_OBJECT_0) break;

		if (WaitForSingleObject(hPauseEvent, 0) != WAIT_OBJECT_0)
		{
			HANDLE h[2] = { hPauseEvent, hStopEvent };
			DWORD w = WaitForMultipleObjects(2, h, FALSE, INFINITE);
			if (w == WAIT_OBJECT_0 + 1) break;
		}

		if (WaitForSingleObject(hBossSpecialEvent, 0) == WAIT_OBJECT_0)
		{
			if (GetTickCount() - specialSetTime > 200)
			{
				ResetEvent(hBossSpecialEvent);
				specialHandled = 0;
			}
		}

		DWORD waitCheck = WaitForSingleObject(hMutex, 100);
		if (waitCheck == WAIT_OBJECT_0)
		{
			int aliveNow = 0;
			for (int i = 0; i < pCount; i++)
			{
				if (players[i]->alive) aliveNow++;
			}
			ReleaseMutex(hMutex);

			if (aliveNow == 0)
			{
				std::cout << GetTickCount() << " ms | Все игроки погибли\n";
				SetEvent(hStopEvent);
				break;
			}
		}
		else if (WaitForSingleObject(hStopEvent, 0) == WAIT_OBJECT_0)
		{
			break;
		}

		DWORD now = GetTickCount() - startTick - (DWORD)pauseTime;

		if (now >= nextAttack)
		{
			DWORD wait = WaitForSingleObject(hMutex, 100);
			if (wait == WAIT_OBJECT_0)
			{
				if (WaitForSingleObject(hStopEvent, 0) == WAIT_OBJECT_0)
				{
					ReleaseMutex(hMutex);
					break;
				}

				int aliveCount = 0;
				for (int i = 0; i < pCount; i++)
				{
					if (players[i]->alive) aliveCount++;
				}

				if (aliveCount > 0)
				{
					int idx = -1;
					while (idx == -1)
					{
						int r = rand() % pCount;
						if (players[r]->alive)
						{
							idx = r;
						}
					}

					bossTarget = idx;
					ReleaseMutex(hMutex);
					SetEvent(hBossAttackEvent);
				}
				else
				{
					ReleaseMutex(hMutex);
				}

				nextAttack = now + boss->attackCooldown * 1000;
			}
			else if (WaitForSingleObject(hStopEvent, 0) == WAIT_OBJECT_0)
			{
				break;
			}
		}

		if (now >= nextSpecial)
		{
			DWORD wait = WaitForSingleObject(hMutex, 100);
			if (wait == WAIT_OBJECT_0)
			{
				if (WaitForSingleObject(hStopEvent, 0) == WAIT_OBJECT_0)
				{
					ReleaseMutex(hMutex);
					break;
				}

				int aliveCount = 0;
				for (int i = 0; i < pCount; i++)
				{
					if (players[i]->alive)
					{
						aliveCount++;
					}
				}

				if (aliveCount > 0)
				{
					double eff = boss->specialDamage * (1 - 0.05 * (aliveCount - 1));

					std::cout << GetTickCount() << " ms | Босс нанес особую атаку по " << aliveCount << " игрокам\n";

					for (int i = 0; i < pCount; i++)
					{
						if (!players[i]->alive) continue;

						int chance = rand() % 100;

						if (chance < players[i]->dodgeChance)
						{
							std::cout << GetTickCount() << " ms | " << players[i]->name <<
								" увернулся от особой атаки\n";
						}
						else
						{
							long long dmg = (long long)(eff * (100 - players[i]->defense) / 100);
							players[i]->health -= dmg;
							if (players[i]->health <= 0)
							{
								players[i]->health = 0;
								players[i]->alive = false;
							}

							std::cout << GetTickCount() << " ms | " << players[i]->name << " получил " <<
								dmg << " особого урона | HP " << players[i]->health << std::endl;
						}
					}

					specialAliveCount = aliveCount;
					specialHandled = 0;
					specialSetTime = GetTickCount();
					ReleaseMutex(hMutex);
					SetEvent(hBossSpecialEvent);
				}
				else
				{
					ReleaseMutex(hMutex);
				}

				nextSpecial = now + boss->specialCooldown * 1000;
			}
			else if (WaitForSingleObject(hStopEvent, 0) == WAIT_OBJECT_0)
			{
				break;
			}
		}

		Sleep(20);
	}

	return 0;
}

DWORD WINAPI PlayerThread(LPVOID param)
{	
	int i = (int)(INT_PTR)param;
	WaitForSingleObject(hStartEvent, INFINITE);

	Player* self = players[i];

	DWORD startTick = GetTickCount();
	long long nextAttack = 0;
	long long nextSpecial = 0;

	while (self->alive && boss->alive)
	{
		if (WaitForSingleObject(hStopEvent, 0) == WAIT_OBJECT_0) break;

		if (WaitForSingleObject(hPauseEvent, 0) != WAIT_OBJECT_0)
		{
			HANDLE h[2] = { hPauseEvent, hStopEvent };
			DWORD w = WaitForMultipleObjects(2, h, FALSE, INFINITE);
			if (w == WAIT_OBJECT_0 + 1) break;
		}

		DWORD now = GetTickCount() - startTick - (DWORD)pauseTime;

		if (now >= nextAttack)
		{
			DWORD wait = WaitForSingleObject(hMutex, 100);
			if (wait == WAIT_OBJECT_0)
			{
				if (WaitForSingleObject(hStopEvent, 0) == WAIT_OBJECT_0)
				{
					ReleaseMutex(hMutex);
					break;
				}

				if (self->alive && boss->alive)
				{
					long long dmg = (long long)self->damage * (100 - boss->resist) / 100;

					boss->health -= dmg;
					self->totalDamage += dmg;
					if (boss->health <= 0)
					{
						boss->health = 0;
						boss->alive = false;
						std::cout << GetTickCount() << " ms | Босс погиб\n";

						ReleaseMutex(hMutex);
						SetEvent(hStopEvent);
						break;
					}

					std::cout << GetTickCount() << " ms | " << self->name << " нанес " <<
						dmg << " урона | HP босса " << boss->health << std::endl;
				}

				ReleaseMutex(hMutex);
			}
			else if (WaitForSingleObject(hStopEvent, 0) == WAIT_OBJECT_0)
			{
				break;
			}
			nextAttack = now + self->attackCooldown * 1000;
		}

		if (now >= nextSpecial)
		{
			DWORD wait = WaitForSingleObject(hMutex, 100);
			if (wait == WAIT_OBJECT_0)
			{
				if (WaitForSingleObject(hStopEvent, 0) == WAIT_OBJECT_0)
				{
					ReleaseMutex(hMutex);
					break;
				}

				if (self->alive && boss->alive)
				{
					long long dmg = (long long)self->specialDamage * (100 - boss->resist) / 100;

					boss->health -= dmg;
					self->totalDamage += dmg;
					if (boss->health <= 0)
					{
						boss->health = 0;
						boss->alive = false;
						std::cout << GetTickCount() << " ms | Босс погиб\n";

						ReleaseMutex(hMutex);
						SetEvent(hStopEvent);
						break;
					}

					std::cout << GetTickCount() << " ms | " << self->name <<
						" нанес особый урон " << dmg << " | HP Босса " << boss->health << std::endl;
				}

				ReleaseMutex(hMutex);
			}
			else if (WaitForSingleObject(hStopEvent, 0) == WAIT_OBJECT_0)
			{
				break;
			}
			nextSpecial = now + self->specialCooldown * 1000;
		}

		if (WaitForSingleObject(hBossAttackEvent, 0) == WAIT_OBJECT_0)
		{
			DWORD wait = WaitForSingleObject(hMutex, 100);
			if (wait == WAIT_OBJECT_0)
			{
				if (bossTarget == i && self->alive && boss->alive)
				{
					int chance = rand() % 100;
					if (chance < self->dodgeChance)
					{
						std::cout << GetTickCount() << " ms | " << self->name <<
							" увернулся от атаки босса\n";
					}
					else
					{
						long long dmg = (long long)boss->damage * (100 - self->defense) / 100;

						self->health -= dmg;
						if (self->health <= 0)
						{
							self->health = 0;
							self->alive = false;
						}

						std::cout << GetTickCount() << " ms | " << self->name << " получил " <<
							dmg << " урона |  HP " << self->health << std::endl;
					}

					bossTarget = -1;
				}
				ReleaseMutex(hMutex);
			}
		}

		if (WaitForSingleObject(hBossSpecialEvent, 0) == WAIT_OBJECT_0)
		{
			DWORD wait = WaitForSingleObject(hMutex, 100);
			if (wait == WAIT_OBJECT_0)
			{
				if (self->alive && boss->alive)
				{
					std::cout << GetTickCount() << " ms | " << self->name <<
						" получил уведомление об особой атаке\n";
				}
				specialHandled++;
				if (specialHandled >= specialAliveCount)
				{
					ResetEvent(hBossSpecialEvent);
					specialSetTime = 0;
				}
				ReleaseMutex(hMutex);
			}
		}

		Sleep(20);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "Russian");
	srand((unsigned)time(NULL));

	if (argc < 2)
	{
		std::cout << "Ошибка: не переданы аргументы\n";
		return -1;
	}	

	int countPlayers = atoi(argv[1]);
	pCount = countPlayers;

	boss = new Bayum();

	for (int i = 0; i < pCount; i++)
	{
		players[i] = new Player();
		sprintf_s(players[i]->name, "Игрок_%d", i + 1);
	}

	hMutex = CreateMutex(NULL, FALSE, NULL);
	if (hMutex == NULL)
	{
		std::cout << "Ошибка создания мюьтекса\n";
		return GetLastError();
	}

	hStartEvent = CreateEventA(NULL, TRUE, FALSE, "PR16_Start");
	if (hStartEvent == NULL)
	{
		std::cout << "Ошибка создания события старта\n";
		return GetLastError();
	}

	hPauseEvent = CreateEventA(NULL, TRUE, TRUE, "PR16_Pause");
	if (hPauseEvent == NULL)
	{
		std::cout << "Ошибка создания события паузы\n";
		return GetLastError();
	}

	hStopEvent = CreateEventA(NULL, TRUE, FALSE, "PR16_Stop");
	if (hStopEvent == NULL)
	{
		std::cout << "Ошибка создания события стопа\n";
		return GetLastError();
	}

	hBossAttackEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hBossAttackEvent == NULL)
	{
		std::cout << "Ошибка создания события атаки босса\n";
		return GetLastError();
	}

	hBossSpecialEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (hBossSpecialEvent == NULL)
	{
		std::cout << "Ошибка создания события особой атаки\n";
		return GetLastError();
	}

	HANDLE hThreadBoss;
	DWORD IDThread;

	hThreadBoss = CreateThread(NULL, 0, BossThread, NULL, 0, &IDThread);
	if (hThreadBoss == NULL)
	{
		std::cout << "Ошибка создания потока босса\n";
		return GetLastError();
	}	

	HANDLE threadsPLayers[10];

	for (int i = 0; i < countPlayers; i++)
	{
		threadsPLayers[i] = CreateThread(NULL, 0, PlayerThread, (LPVOID)(INT_PTR)i, 0, NULL);
	}	

	DWORD battleStart = GetTickCount();

	std::cout << "Сервер ожидает команды старт\n";
	WaitForSingleObject(hStartEvent, INFINITE);
	std::cout << "Сражение началось\n";

	while (true)
	{
		if (WaitForSingleObject(hStopEvent, 0) == WAIT_OBJECT_0) break;

		if (WaitForSingleObject(hPauseEvent, 0) != WAIT_OBJECT_0)
		{
			DWORD ps = GetTickCount();
			HANDLE h[2] = { hPauseEvent, hStopEvent };
			DWORD w = WaitForMultipleObjects(2, h, FALSE, INFINITE);
			if (w == WAIT_OBJECT_0 + 1) break;
			pauseTime += GetTickCount() - ps;
		}

		Sleep(20);
	}

	bool anyAlive = false;
	for (int i = 0; i < pCount; i++)
	{
		if (players[i]->alive) anyAlive = true;
	}

	if (boss->alive && anyAlive) stoppedEarly = true;

	WaitForSingleObject(hThreadBoss, 5000);
	WaitForMultipleObjects(pCount, threadsPLayers, TRUE, 5000);

	DWORD total = (GetTickCount() - battleStart - (DWORD)pauseTime) / 1000;

	std::cout << "\n";
	if (stoppedEarly)
	{
		std::cout << "Сражение прервано досрочно\n";
	}
	else if (boss->alive)
	{
		std::cout << "Победитель: Босс\n";
	}
	else
	{
		std::cout << "Победитель: Игроки\n";
	}

	std::cout << "Продолжительность боя: " << total << " сек.\n";
	std::cout << "HP босса: " << boss->health << "\n";

	int alivePlayers = 0;
	for (int i = 0; i < pCount; i++)
	{
		if (players[i]->alive) alivePlayers++;
	}

	std::cout << "Выжило игроков: " << alivePlayers << " из " << pCount << std::endl;

	std::cout << "\nТоп-3 по суммарному урону:\n";

	int best1 = 0;
	for (int i = 1; i < pCount; i++)
	{
		if (players[i]->totalDamage > players[best1]->totalDamage)
		{
			best1 = i;
		}
	}

	std::cout << "Топ-1. " << players[best1]->name << " - " << players[best1]->totalDamage << std::endl;

	if (pCount > 1)
	{
		int best2 = -1;
		for (int i = 0; i < pCount; i++)
		{
			if (i == best1) { continue; }
			if (best2 == -1 || players[i]->totalDamage > players[best2]->totalDamage) 
			{ 
				best2 = i;
			}
		}
		std::cout << "Топ 2. " << players[best2]->name << " - " << players[best2]->totalDamage << std::endl;

		if (pCount > 2)
		{
			int best3 = -1;
			for (int i = 0; i < pCount; i++)
			{
				if (i == best1 || i == best2) { continue; }
				if (best3 == -1 || players[i]->totalDamage > players[best3]->totalDamage) 
				{
					best3 = i;
				}
			}
			std::cout << "Топ 3. " << players[best3]->name << " - " << players[best3]->totalDamage << std::endl;
		}
	}

	CloseHandle(hThreadBoss);

	for (int i = 0; i < pCount; i++)
	{
		CloseHandle(threadsPLayers[i]);
	}

	CloseHandle(hMutex);
	CloseHandle(hStartEvent);
	CloseHandle(hPauseEvent);
	CloseHandle(hStopEvent);
	CloseHandle(hBossAttackEvent);
	CloseHandle(hBossSpecialEvent);

	for (int i = 0; i < pCount; i++)
	{
		delete players[i];
	}
	delete boss;

	std::cout << "\nНажмите Enter...\n";
	std::cin.get();
	return 0;
}