#include <Windows.h>
#include <conio.h>
#include <iostream>
#include <string>

using namespace std;

int functionSource() {
	HANDLE hWrite = CreateFile("COM2", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (!hWrite) {
		cout << "Failed to open port!" << endl;
		return 0;
	}

	//Сигнал окончания чтения
	HANDLE readyToWrite = CreateEvent(NULL, TRUE, TRUE, "readyToWrite");
	if (!readyToWrite) {
		cout << "Failed to create readyToWrite Event!" << endl;
		CloseHandle(hWrite);
		return 0;
	}

	//Overlapped для чтения из порта
	HANDLE readyToRead = CreateEvent(NULL, TRUE, FALSE, "readyToRead");
	if (!readyToRead) {
		cout << "Failed to create readyToRead Event!" << endl;
		CloseHandle(hWrite);
		CloseHandle(readyToWrite);
		return 0;
	}
	OVERLAPPED asynchWrite = { 0 };
	asynchWrite.hEvent = readyToRead;

	//Новый процесс
	//STARTUPINFO si;																			//Структура определяющая свойства нового процесса	
	//ZeroMemory(&si, sizeof(STARTUPINFO));													//Обнуление структуры si
	//si.cb = sizeof(STARTUPINFO);															//Инициализация поля cb структуры si размером структры

	//PROCESS_INFORMATION client;																//Создание структуры PROCESS_INFORMATION для нового процесса
	//ZeroMemory(&client, sizeof(PROCESS_INFORMATION));										//Обнуление структуры pi

	//if (!CreateProcess("C:\\Users\\Brunoswine\\source\\repos\\APK\\LAB7\\Client\\Debug\\client.exe",																//Создание нового процесса
	//	NULL,
	//	NULL,
	//	NULL,
	//	TRUE,
	//	CREATE_NEW_CONSOLE,
	//	NULL,
	//	NULL,
	//	&si,
	//	&client)) {
	//	cout << "CreateProcess failed\n";
	//	return 0;
	//}

	/*

		DCB ComDCM;
		memset(&ComDCM, 0, sizeof(ComDCM));
		ComDCM.DCBlength = sizeof(DCB);
		ComDCM.BaudRate = DWORD(baudrate);
		ComDCM.ByteSize = 8; // размер байта
		ComDCM.Parity = NOPARITY; // паритет
		ComDCM.StopBits = ONESTOPBIT; // количество стоп бит
		ComDCM.fAbortOnError = TRUE;
		ComDCM.fDtrControl = DTR_CONTROL_DISABLE; // сброс DTR бита

		ComDCM.fRtsControl = RTS_CONTROL_TOGGLE; // автоустановка RTS бита
		ComDCM.fBinary = TRUE; //бинарный режим всегда
		ComDCM.fParity = FALSE; //паритет
		ComDCM.fInX = ComDCM.fOutX = FALSE;
		ComDCM.XonChar = 0;
		ComDCM.XoffChar = uint8_t(0xff);
		ComDCM.fErrorChar = FALSE;
		ComDCM.fNull = FALSE;
		ComDCM.fOutxCtsFlow = FALSE;
		ComDCM.fOutxDsrFlow = FALSE;
		ComDCM.XonLim = 128;
		ComDCM.XoffLim = 128;

		*/

	string str;
	int i;
	char buf = '\n';
	do {
		i = 0;
		rewind(stdin);
		//Ввод строки для передачи
		cout << "Input message or press 'Enter' to exit" << endl;
		getline(cin, str);

		if (!str.size()) break;

		//Посимвольная передача строки
		while (i < str.size()) {
			//Ожидание окончания чтения
			WaitForSingleObject(readyToWrite, INFINITE);
			ResetEvent(readyToWrite);
			WriteFile(hWrite, &(str[i]), 1, NULL, &asynchWrite);
			i++;
		}

		//Запись '\n'
		//Ожидание окончания чтения
		WaitForSingleObject(readyToWrite, INFINITE);
		ResetEvent(readyToWrite);
		WriteFile(hWrite, &buf, 1, NULL, &asynchWrite);

	} while (1);

	//Запись 0 символа - конец работы
	buf = 0;
	WaitForSingleObject(readyToWrite, INFINITE);
	WriteFile(hWrite, &buf, 1, NULL, &asynchWrite);

	CloseHandle(hWrite);
	CloseHandle(readyToRead);
	CloseHandle(readyToWrite);

}

int functionRecieve() {

	char buf;		//Буфер для записи
	int f = 1;


	HANDLE hRead = CreateFile("COM2", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (!hRead) {
		cout << "Failed to open port!" << endl;
		return 0;
	}

	//Сигнал окочания асинхронного чтения
	HANDLE finishedReading = CreateEvent(NULL, TRUE, FALSE, "finishedReading");
	if (!finishedReading) {
		cout << "Failed to create finishedReading Event!" << endl;
		CloseHandle(hRead);
		return 0;
	}
	//Overlapped для чтения из порта
	OVERLAPPED asynchRead = { 0 };
	asynchRead.hEvent = finishedReading;

	//Сигнал окончания чтения
	HANDLE readyToWrite = OpenEvent(EVENT_ALL_ACCESS, FALSE, "readyToWrite");
	if (!readyToWrite) {
		cout << "Failed to open readyToWrite Event!" << endl;
		CloseHandle(hRead);
		CloseHandle(finishedReading);
		return 0;
	}

	//Сигнал окончания записи
	HANDLE readyToRead = OpenEvent(EVENT_ALL_ACCESS, FALSE, "readyToRead");
	if (!readyToRead) {
		cout << "Failed to open readyToRead Event!" << endl;
		CloseHandle(hRead);
		CloseHandle(finishedReading);
		CloseHandle(readyToWrite);
		return 0;
	}


	do {


		//Ожидание окончания записи
		WaitForSingleObject(readyToRead, INFINITE);
		ResetEvent(readyToRead);

		ReadFile(hRead, &buf, 1, NULL, &asynchRead);
		//Ожидание окончания чтения
		WaitForSingleObject(finishedReading, INFINITE);
		SetEvent(readyToWrite);

		//Вывод символа
		if (!buf) break;

		if (f) {
			cout << "New message: ";
			f = 0;
		}

		cout << buf;

		if (buf == '\n')
			f = 1;

	} while (1);

	//Закрытие handl'ов
	CloseHandle(hRead);
	CloseHandle(readyToRead);
	CloseHandle(readyToWrite);
	CloseHandle(finishedReading);
}

int main() {

	HANDLE serverWrites = OpenEvent(EVENT_ALL_ACCESS, FALSE, "serverWrites");
	if (!serverWrites) {
		cout << "Failed to open serverWrites Event!" << endl;
		return 0;
	}

	if ((WaitForSingleObject(serverWrites, 0) == WAIT_OBJECT_0)) {
		cout << "This is Reciever" << endl;
		functionRecieve();
	}
	else {
		cout << "This is Source" << endl;
		functionSource();
	}
	CloseHandle(serverWrites);
	return 0;
}