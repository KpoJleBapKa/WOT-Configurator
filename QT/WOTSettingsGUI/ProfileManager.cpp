#include "main.h" // Головний заголовок
#include <fstream>
#include <filesystem>
#include <stdexcept> // Для std::runtime_error
#include <iostream> // Для std::cerr (опційно)

namespace fs = std::filesystem;

const fs::path userDataDir = "User Data";
const fs::path userdataFile = userDataDir / "userdata.txt";

// Приймає ім'я, зберігає у файл, кидає виняток при помилці
void ProfileManager::setName(const std::string& name) {
    saveNameToFile(name);
}

void ProfileManager::saveNameToFile(const std::string& name) {
    try {
        if (!fs::exists(userDataDir)) {
            fs::create_directories(userDataDir);
        }
        std::ofstream outFile(userdataFile);
        if (outFile.is_open()) {
            outFile << name; // Записуємо ім'я
            outFile.close();
        } else {
            throw std::runtime_error("Не вдалося відкрити файл " + userdataFile.string() + " для запису.");
        }
    } catch (const fs::filesystem_error& e) {

        throw std::runtime_error(std::string("Помилка файлової системи при збереженні імені: ") + e.what());
    } catch (const std::runtime_error& e) {
        throw;
    } catch (...) {
        throw std::runtime_error("Невідома помилка під час збереження імені.");
    }
}

std::string ProfileManager::loadNameFromFile() {
    if (!fs::exists(userdataFile)) {
        return "";
    }
    std::ifstream inFile(userdataFile);
    std::string loadedName = "";
    if (inFile.is_open()) {
        std::getline(inFile, loadedName);
        inFile.close();
    } else {
        throw std::runtime_error("Не вдалося відкрити файл " + userdataFile.string() + " для читання.");
    }
    return loadedName;
}

