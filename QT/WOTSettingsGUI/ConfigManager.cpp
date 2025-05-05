#include "main.h" // Головний заголовок
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib> // Для getenv
#include <stdexcept> // Для std::runtime_error
#include <iostream> // Для std::cerr (опційно)


// Внутрішня допоміжна функція
fs::path ConfigManager::getGameConfigPathInternal() {
    const char* appDataPath_cstr = getenv("APPDATA");
    if (!appDataPath_cstr) {
        throw std::runtime_error("Не вдалося отримати системний шлях APPDATA.");
    }
    fs::path appDataDir(appDataPath_cstr);
    if (!fs::exists(appDataDir) || !fs::is_directory(appDataDir)) {
        throw std::runtime_error("Шлях APPDATA не існує або не є директорією: " + std::string(appDataPath_cstr));
    }
    return appDataDir / "Wargaming.net" / "WorldOfTanks" / "preferences.xml";
}

// Повертає шлях до поточного конфігу гри
fs::path ConfigManager::getCurrentGameConfigPath() {
    return getGameConfigPathInternal();
}

// Приймає шлях до конфігу користувача, кидає виняток при помилці
void ConfigManager::changeCurrentConfig(const fs::path& sourceConfigPath) {
    // Логіка вибору sourceConfigPath тепер у MainWindow
    // Валідація sourceConfigPath також відбувається в MainWindow перед викликом

    if (!fs::exists(sourceConfigPath)) {
        throw std::runtime_error("Обраний файл конфігурації користувача не знайдено: " + sourceConfigPath.string());
    }
    if (!fs::is_regular_file(sourceConfigPath)) {
        throw std::runtime_error("Вибраний шлях не є файлом: " + sourceConfigPath.string());
    }


    fs::path targetPath = getGameConfigPathInternal();
    fs::path targetDir = targetPath.parent_path();

    // Застосування (копіювання з заміною)
    try {
        if (!fs::exists(targetDir)) {
            fs::create_directories(targetDir);
        }
        fs::copy_file(sourceConfigPath, targetPath, fs::copy_options::overwrite_existing);
        // Якщо копіювання пройшло без винятків - успіх
    } catch (const fs::filesystem_error& e) {
        throw std::runtime_error(std::string("Помилка файлової системи при застосуванні конфігу: ") + e.what());
    } catch (...) {
        throw std::runtime_error("Невідома помилка під час застосування конфігу.");
    }
}

// Читає та повертає вміст поточного конфігу гри
std::string ConfigManager::viewCurrentGameConfigContent() {
    fs::path gameConfigPath = getGameConfigPathInternal();
    if (!fs::exists(gameConfigPath)) {
        throw std::runtime_error("Файл конфігурації гри (preferences.xml) не знайдено.");
    }

    // Валідація перед читанням (використовуємо внутрішній валідатор)
    ValidationResult valResult = m_validator.validateFile(gameConfigPath);
    if (!valResult.isValid()) {
        throw std::runtime_error("Поточний конфіг гри не є коректним XML: " + valResult.wellFormedError);
    }
    // Можна додати логування попереджень з valResult.structureInfo / valResult.valueErrors

    std::ifstream file(gameConfigPath);
    if (!file.is_open()) {
        throw std::runtime_error("Не вдалося відкрити файл конфігурації гри: " + gameConfigPath.string());
    }
    std::stringstream buffer;
    try {
        buffer << file.rdbuf();
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Помилка читання файлу конфігурації гри: ") + e.what());
    }
    return buffer.str();
}

// Amogus
void ConfigManager::uploadConfig() {
    throw std::runtime_error("Функція 'uploadConfig' не реалізована.");
}
