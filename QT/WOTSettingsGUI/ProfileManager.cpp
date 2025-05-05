#include "main.h" // Головний заголовок
#include <fstream>
#include <filesystem>
#include <stdexcept> // Для std::runtime_error
#include <iostream> // Для std::cerr (опційно)

namespace fs = std::filesystem;

// Визначаємо шлях до файлу даних тут
const fs::path userDataDir = "User Data";
const fs::path userdataFile = userDataDir / "userdata.txt";

// Приймає ім'я, зберігає у файл, кидає виняток при помилці
void ProfileManager::setName(const std::string& name) {
    saveNameToFile(name); // Викликаємо приватний метод
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
                // Успіх - нічого не робимо
        } else {
            // Кидаємо виняток, якщо не вдалося відкрити файл
            throw std::runtime_error("Не вдалося відкрити файл " + userdataFile.string() + " для запису.");
        }
    } catch (const fs::filesystem_error& e) {
        // Перетворюємо помилку файлової системи
        throw std::runtime_error(std::string("Помилка файлової системи при збереженні імені: ") + e.what());
    } catch (const std::runtime_error& e) {
        throw; // Прокидуємо наш власний виняток
    } catch (...) {
        throw std::runtime_error("Невідома помилка під час збереження імені.");
    }
}

// Повертає ім'я або порожній рядок, не використовує std::cout
std::string ProfileManager::loadNameFromFile() {
    if (!fs::exists(userdataFile)) {
        return ""; // Файл не знайдено - це не помилка, імені просто немає
    }
    std::ifstream inFile(userdataFile);
    std::string loadedName = "";
    if (inFile.is_open()) {
        std::getline(inFile, loadedName); // Читаємо весь рядок (ім'я)
        inFile.close();
    } else {
        // Якщо файл є, але не відкрився - це помилка
        throw std::runtime_error("Не вдалося відкрити файл " + userdataFile.string() + " для читання.");
    }
    return loadedName;
}

