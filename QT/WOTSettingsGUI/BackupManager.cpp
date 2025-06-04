#include "main.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <vector>
#include <fstream> // Для копіювання, хоча fs::copy_file краще
#include <cstdlib> // Для getenv
#include <stdexcept> // Для std::runtime_error
#include <iostream> // Тимчасово для cerr (у FileValidator)

// Допоміжна функція для отримання шляху до preferences.xml
fs::path BackupManager::getGameConfigPath() {
    const char* appDataPath_cstr = getenv("APPDATA");
    if (!appDataPath_cstr) {
        throw std::runtime_error("Не вдалося отримати системний шлях APPDATA.");
    }
    // Перевіряємо, чи шлях існує і чи це директорія (хоча б базовий %APPDATA%)
    fs::path appDataDir(appDataPath_cstr);
    if (!fs::exists(appDataDir) || !fs::is_directory(appDataDir)) {
        throw std::runtime_error("Шлях APPDATA не існує або не є директорією: " + std::string(appDataPath_cstr));
    }
    // Повертаємо повний шлях до файлу
    return appDataDir / "Wargaming.net" / "WorldOfTanks" / "preferences.xml";
}

// Повертає шлях до створеного бекапу або кидає виняток
fs::path BackupManager::createBackup() {
    fs::path sourcePath = getGameConfigPath();

    if (!fs::exists(sourcePath)) {
        throw std::runtime_error("Файл конфігурації гри для резервного копіювання не знайдено: " + sourcePath.string());
    }

    if (!m_validator.validateBeforeAction(sourcePath, "Створення резервної копії (перевірка джерела)", false)) {
        throw std::runtime_error("Перевірка файлу-джерела перед резервним копіюванням не пройдена або скасована.");
    }

    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm;
#ifdef _WIN32
    localtime_s(&local_tm, &in_time_t);
#else
    localtime_r(&in_time_t, &local_tm); // POSIX
#endif
    std::stringstream ss;
    ss << "preferences_" << std::put_time(&local_tm, "%Y_%m_%d_%H%M%S") << ".xml";

    // Отримуємо поточний шлях виконуваного файлу для формування відносних шляхів
    fs::path currentExecutablePath = fs::current_path(); // або QCoreApplication::applicationDirPath() якщо використовуєте Qt
    fs::path backupDir = currentExecutablePath / "Restored Configs";
    fs::path backupPath = backupDir / ss.str();

    try {
        if (!fs::exists(backupDir)) {
            fs::create_directories(backupDir);
        }
        fs::copy_file(sourcePath, backupPath, fs::copy_options::overwrite_existing);
        return backupPath;
    } catch (const fs::filesystem_error& e) {
        throw std::runtime_error(std::string("Помилка файлової системи при створенні резервної копії: ") + e.what());
    } catch (...) {
        throw std::runtime_error("Невідома помилка під час створення резервної копії.");
    }
}

// Приймає шлях до файлу бекапу, кидає виняток при помилці
void BackupManager::restoreFromBackup(const fs::path& backupPath) {
    if (!fs::exists(backupPath)) {
        throw std::runtime_error("Обраний файл резервної копії не знайдено: " + backupPath.string());
    }

    if (!m_validator.validateBeforeAction(backupPath, "Відновлення з резервної копії (перевірка джерела)", true)) {
        throw std::runtime_error("Перевірка файлу-джерела перед відновленням не пройдена або скасована.");
    }

    fs::path targetPath = getGameConfigPath();
    fs::path targetDir = targetPath.parent_path();

    try {
        if (!fs::exists(targetDir)) {
            fs::create_directories(targetDir);
        }

        if (fs::exists(targetPath)) {
            try {
                fs::remove(targetPath);
                // std::cerr << "Successfully removed existing preferences.xml before restoration." << std::endl;
            } catch (const fs::filesystem_error& e) {
                // Якщо не вдалося видалити, це вже проблема з дозволами/блокуванням
                throw std::runtime_error(std::string("Помилка файлової системи: не вдалося видалити існуючий файл preferences.xml перед відновленням. ") + e.what());
            }
        }

        // Використовуємо overwrite_existing на випадок, якщо remove не спрацював з невідомих причин
        // або файл був створений між remove та copy_file.
        fs::copy_file(backupPath, targetPath, fs::copy_options::overwrite_existing);
    } catch (const fs::filesystem_error& e) {
        throw std::runtime_error(std::string("Помилка файлової системи при відновленні з копії: ") + e.what());
    } catch (...) {
        throw std::runtime_error("Невідома помилка під час відновлення з копії.");
    }
}

void BackupManager::manageBackupSpace() {
    // Тут може бути логіка для видалення старих бекапів.
}
