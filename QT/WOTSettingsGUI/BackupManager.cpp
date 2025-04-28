#include "main.h" // Головний заголовок
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

// ЗМІНЕНО: Повертає шлях до створеного бекапу або кидає виняток
fs::path BackupManager::createBackup() {
    fs::path sourcePath = getGameConfigPath();

    if (!fs::exists(sourcePath)) {
        throw std::runtime_error("Файл конфігурації гри для резервного копіювання не знайдено: " + sourcePath.string());
    }

    // Валідація джерела (використовуємо валідатор, що є членом класу)
    // validateBeforeAction сам покаже повідомлення або викине виняток, якщо треба
    // false - не показувати вікно успіху валідації
    // Припускаємо, що якщо validateBeforeAction повернув false, він вже повідомив причину.
    if (!m_validator.validateBeforeAction(sourcePath, "Створення резервної копії (перевірка джерела)", false)) {
        throw std::runtime_error("Перевірка файлу-джерела перед резервним копіюванням не пройдена або скасована.");
    }
    // Якщо дійшли сюди, файл валідний (можливо, з попередженнями, які користувач проігнорував)

    // Створення імені файлу бекапу
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

    fs::path backupDir = "Restored Configs";
    fs::path backupPath = backupDir / ss.str();

    // Копіювання файлу
    try {
        if (!fs::exists(backupDir)) {
            fs::create_directories(backupDir);
        }
        fs::copy_file(sourcePath, backupPath, fs::copy_options::overwrite_existing);
        return backupPath; // Повертаємо шлях до успішно створеного бекапу
    } catch (const fs::filesystem_error& e) {
        throw std::runtime_error(std::string("Помилка файлової системи при створенні резервної копії: ") + e.what());
    } catch (...) {
        throw std::runtime_error("Невідома помилка під час створення резервної копії.");
    }
}

// ЗМІНЕНО: Приймає шлях до файлу бекапу, кидає виняток при помилці
void BackupManager::restoreFromBackup(const fs::path& backupPath) {
    // Логіка вибору файлу тепер у MainWindow
    // Валідація файлу backupPath також відбувається в MainWindow перед викликом

    if (!fs::exists(backupPath)) {
        throw std::runtime_error("Обраний файл резервної копії не знайдено: " + backupPath.string());
    }

    fs::path targetPath = getGameConfigPath(); // Отримуємо шлях до preferences.xml
    fs::path targetDir = targetPath.parent_path();

    // Відновлення (копіювання з заміною)
    try {
        if (!fs::exists(targetDir)) {
            fs::create_directories(targetDir);
        }
        // Заміна файлу
        fs::copy_file(backupPath, targetPath, fs::copy_options::overwrite_existing);
        // Якщо копіювання пройшло без винятків - відновлення успішне
    } catch (const fs::filesystem_error& e) {
        throw std::runtime_error(std::string("Помилка файлової системи при відновленні з копії: ") + e.what());
    } catch (...) {
        throw std::runtime_error("Невідома помилка під час відновлення з копії.");
    }
}

// Цей метод поки не використовується в GUI
void BackupManager::manageBackupSpace() {
    // Тут може бути логіка для видалення старих бекапів
    // Наприклад, залишити тільки N останніх файлів
    // std::cerr << "Warning: manageBackupSpace() is not implemented." << std::endl;
    throw std::runtime_error("Функція manageBackupSpace не реалізована.");
}
