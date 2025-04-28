#include "main.h"      // Або "ChangeTracker.h"
#include <iostream>
#include <fstream>     // Для запису у файл
#include <string>
#include <chrono>      // Для отримання дати/часу
#include <iomanip>     // Для форматування дати/часу (put_time)
#include <filesystem> // Для перевірки/створення директорії
#include <sstream>     // Для форматування часу

using namespace std;
namespace fs = std::filesystem;

// Метод для запису дії в лог
void ChangeTracker::logAction(const std::string& functionName, bool success, const std::string& details) {
    const fs::path logDir = "User Data";
    const fs::path logFilePath = logDir / "logs.txt";

    try {
        // Переконуємося, що директорія "User Data" існує
        if (!fs::exists(logDir)) {
            cout << "Log directory '" << logDir.string() << "' not found. Creating..." << endl;
            fs::create_directory(logDir);
        }

        // Відкриваємо файл для дозапису (append)
        // ios::app гарантує, що ми будемо писати в кінець файлу
        ofstream logFile(logFilePath, ios::app);

        if (!logFile.is_open()) {
            cerr << "Error: Could not open log file for writing: " << logFilePath.string() << endl;
            return;
        }

        // Отримуємо поточний час
        auto now = chrono::system_clock::now();
        auto now_c = chrono::system_clock::to_time_t(now);
        tm now_tm;
        #ifdef _WIN32
            localtime_s(&now_tm, &now_c); // Безпечна версія для Windows
        #else
            localtime_r(&now_c, &now_tm); // Безпечна версія для POSIX
        #endif

        // Форматуємо лог-повідомлення: YYYY-MM-DD_HH:MM:SS | Function | Result: [OK/NOK] | Details: ...
        logFile << put_time(&now_tm, "%Y-%m-%d_%H:%M:%S") << " | "
                << functionName << " | Result: [" << (success ? "OK" : "NOK") << "]";

        // Додаємо деталі, якщо вони є
        if (!details.empty()) {
            logFile << " | Details: " << details;
        }

        logFile << endl; // Завершуємо рядок логу

        // Файл закриється автоматично при виході з функції (RAII для ofstream)

    } catch (const fs::filesystem_error& e) {
         cerr << "Filesystem error during logging: " << e.what() << endl;
    } catch (const std::exception& e) {
         cerr << "Standard exception during logging: " << e.what() << endl;
    } catch (...) {
         cerr << "Unknown error during logging." << endl;
    }
}

// --- Старі методи (якщо вони були реалізовані) тепер не потрібні для логування дій ---
// void ChangeTracker::logChanges() { cout << "Logging changes..." << endl; }
// void ChangeTracker::rollbackChanges() { cout << "Rolling back changes..." << endl; }
// void ChangeTracker::displayChangeHistory() { cout << "Displaying change history..." << endl; }