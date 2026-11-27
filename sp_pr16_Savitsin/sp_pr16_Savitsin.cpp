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

	std::string exeName = "\\GameServer.exe";

	std::string targetExe = "C:\\Users\\st312-07\\Desktop\\Savitsin\\sp_pr16_Savitsin\\x64\\Debug" + exeName;

	std::string cmdLine = "\"" + targetExe + "\" " + std::to_string(countPlayers);

	char* cmd = new char[cmdLine.length() + 1];
	strcpy_s(cmd, cmdLine.length() + 1, cmdLine.c_str());

	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	
	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);

	if (!CreateProcessA(targetExe.c_str(), cmd, NULL, NULL, 
		FALSE, CREATE_NEW_CONSOLE | CREATE_SUSPENDED, NULL, NULL, &si, &pi))
	{
		std::cout << "Ошибка создания процесса" << std::endl;
		return GetLastError();
	}	

	int exit = false;

	while (exit == false)
	{
		PrintMenu();

		std::cout << "Выберите пункт меню: ";
		int choice;
		std::cin >> choice;

		switch (choice)
		{
		case 1:
			ResumeThread(pi.hThread);
			std::cout << "Сражение началось\n";
			break;
		case 2:
			SuspendThread(pi.hThread);
			std::cout << "Сражение приостановлено\n";
			break;
		case 3:
			ResumeThread(pi.hThread);
			std::cout << "Сражение продолжается\n";
			break;
		case 4:
			TerminateProcess(pi.hProcess, 1);
			std::cout << "Сражениe завершилось досрочно\n";
			break;
		case 5:
			exit = true;
			break;
		}
	}

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	delete[] cmd;

	return 0;
}