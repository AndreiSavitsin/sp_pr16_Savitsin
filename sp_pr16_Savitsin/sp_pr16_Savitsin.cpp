#include <iostream>
#include <Windows.h>
#include <string>

void PrintMenu()
{
	std::cout << "\nМеню управления\n";
	std::cout << "1. Старт сражения\n";
	std::cout << "2. Пауза\n";
	std::cout << "3. Продолжение\n";
	std::cout << "4. Досрочное завершение сражения\n";
	std::cout << "5. Выход\n";
}

int main()
{
	setlocale(LC_ALL, "Russian");

	int countPlayers = 0;

	while (countPlayers > 10 || countPlayers < 1)
	{
		std::cout << "Введите количество игроков: ";
		std::cin >> countPlayers;

		if (countPlayers < 1 || countPlayers > 10)
		{
			std::cout << "Количество игроков должно быть от 1 до 10\n";
		}
	}

	HANDLE hStart = CreateEventA(NULL, TRUE, FALSE, "PR16_Start");
	if (hStart == NULL)
	{
		std::cout << "Ошибка создания события старта\n";
		return GetLastError();
	}

	HANDLE hPause = CreateEventA(NULL, TRUE, TRUE, "PR16_Pause");
	if (hPause == NULL)
	{
		std::cout << "Ошибка создания события паузы\n";
		return GetLastError();
	}

	HANDLE hStop = CreateEventA(NULL, TRUE, FALSE, "PR16_Stop");
	if (hStop == NULL)
	{
		std::cout << "Ошибка создания события стопа\n";
		return GetLastError();
	}

	std::string targetExe = "C:\\Program Files (x86)\\projects\\sp_pr16_Savitsin\\x64\\Debug\\GameServer.exe";

	std::string cmdLine = "\"" + targetExe + "\" " + std::to_string(countPlayers);

	char* cmd = new char[cmdLine.length() + 1];
	strcpy_s(cmd, cmdLine.length() + 1, cmdLine.c_str());

	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	
	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);

	if (!CreateProcessA(targetExe.c_str(), cmd, NULL, NULL, 
		FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
	{
		std::cout << "Ошибка создания процесса" << std::endl;
		return GetLastError();
	}	

	bool exit = false;

	while (exit == false)
	{
		PrintMenu();

		std::cout << "Выберите пункт меню: ";
		int choice;
		std::cin >> choice;

		switch (choice)
		{
		case 1:
			SetEvent(hStart);
			std::cout << "Сражение началось\n";
			break;
		case 2:
			ResetEvent(hPause);
			std::cout << "Сражение приостановлено\n";
			break;
		case 3:
			SetEvent(hPause);
			std::cout << "Сражение продолжается\n";
			break;
		case 4:
			SetEvent(hStop);
			std::cout << "Сражениe завершилось досрочно\n";
			break;
		case 5:
			exit = true;
			break;
		}
	}

	SetEvent(hStop);
	WaitForSingleObject(pi.hProcess, 5000);

	CloseHandle(hStart);
	CloseHandle(hPause);
	CloseHandle(hStop);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	delete[] cmd;

	return 0;
}