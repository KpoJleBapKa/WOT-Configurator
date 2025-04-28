#include "main.h" // Головний заголовок
#include <fstream>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include <sstream>
#include <iostream> // Залишаємо для std::cerr

namespace fs = std::filesystem; // Використовуємо простір імен явно

// Метод для запису дії в лог
void ChangeTracker::logAction(const std::string& functionName, bool success, const std::string& details) {
    const fs::path logDir = "User Data";
    const fs::path logFilePath = logDir / "logs.txt";

    try {
        // Переконуємося, що директорія "User Data" існує
        if (!fs::exists(logDir)) {
            // Спроба створити директорію
            try {
                fs::create_directory(logDir);
            } catch(const fs::filesystem_error& e) {
                // Якщо не вдалося створити - критична помилка
                std::cerr << "CRITICAL ERROR: Could not create log directory '" << logDir.string() << "'. Error: " << e.what() << std::endl;
                return; // Не можемо логувати
            }
        }

        // Відкриваємо файл для дозапису (append)
        std::ofstream logFile(logFilePath, std::ios::app);

        if (!logFile.is_open()) {
            // Якщо не можемо відкрити файл логів - це проблема
            std::cerr << "ERROR: Could not open log file for writing: " << logFilePath.string() << std::endl;
            // Можливо, варто кинути виняток або повернути статус помилки?
            // Поки що просто виходимо.
            return;
        }

        // Отримуємо поточний час
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm;
#ifdef _WIN32
        localtime_s(&now_tm, &now_c); // Безпечна версія для Windows
#else
        localtime_r(&now_c, &now_tm); // Безпечна версія для POSIX
#endif

        // Форматуємо лог-повідомлення
        logFile << std::put_time(&now_tm, "%Y-%m-%d_%H:%M:%S") << " | "
                << functionName << " | Result: [" << (success ? "OK" : "NOK") << "]";

        // Додаємо деталі, якщо вони є
        if (!details.empty()) {
            logFile << " | Details: " << details;
        }

        logFile << std::endl; // Завершуємо рядок логу

        // Файл закриється автоматично (RAII)

    } catch (const fs::filesystem_error& e) {
        // Ловимо помилки файлової системи під час роботи (окрім створення папки/відкриття файлу)
        std::cerr << "Filesystem error during logging: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        // Ловимо інші стандартні винятки
        std::cerr << "Standard exception during logging: " << e.what() << std::endl;
    } catch (...) {
        // Ловимо щось зовсім непередбачуване
        std::cerr << "Unknown error during logging." << std::endl;
    }
}
