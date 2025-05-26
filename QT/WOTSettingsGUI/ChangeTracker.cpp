#include "main.h" // Головний заголовок
#include <fstream>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include <sstream>
#include <iostream> // Залишаємо для std::cerr

namespace fs = std::filesystem;

// Метод для запису дії в лог
void ChangeTracker::logAction(const std::string& functionName, bool success, const std::string& details) {
    const fs::path logDir = "User Data";
    const fs::path logFilePath = logDir / "logs.txt";

    try {
        if (!fs::exists(logDir)) {
            try {
                fs::create_directory(logDir);
            } catch(const fs::filesystem_error& e) {
                std::cerr << "CRITICAL ERROR: Could not create log directory '" << logDir.string() << "'. Error: " << e.what() << std::endl;
                return;
            }
        }

        std::ofstream logFile(logFilePath, std::ios::app);

        if (!logFile.is_open()) {
            std::cerr << "ERROR: Could not open log file for writing: " << logFilePath.string() << std::endl;
            return;
        }

        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm;
#ifdef _WIN32
        localtime_s(&now_tm, &now_c);
#else
        localtime_r(&now_c, &now_tm);
#endif

        logFile << std::put_time(&now_tm, "%Y-%m-%d_%H:%M:%S") << " | "
                << functionName << " | Result: [" << (success ? "OK" : "NOK") << "]";

        if (!details.empty()) {
            logFile << " | Details: " << details;
        }

        logFile << std::endl;

    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error during logging: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Standard exception during logging: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error during logging." << std::endl;
    }
}
