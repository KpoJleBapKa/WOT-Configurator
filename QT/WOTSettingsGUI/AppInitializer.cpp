#include "main.h" // Головний заголовок з оголошеннями класів
#include <vector>
#include <fstream>
#include <stdexcept> // Для std::runtime_error
#include <iostream>
#include "preferences_data.h" // Включаємо дані

void AppInitializer::checkFolders() {
    // Список папок, які мають існувати
    const std::vector<fs::path> folders = {
        "User Data", "Saved Configs", "Restored Configs", "User Configs", "Reference Config"
    };

    try {
        for (const auto& folder : folders) {
            if (!fs::exists(folder)) {
                fs::create_directory(folder);
            }
        }

        /*// Створення reference config, якщо його немає
        fs::path refFolder = "Reference Config";
        fs::path refFile = refFolder / "preferences.xml";
        if (!fs::exists(refFile)) {
            if (!fs::exists(refFolder)) { // Переконуємось, що папка є
                fs::create_directories(refFolder);
            }
            std::ofstream outFile(refFile, std::ios::binary);
            if (outFile.is_open()) {
                outFile.write(reinterpret_cast<const char*>(preferences_xml), preferences_xml_len);
                outFile.close();
            } else {
                throw std::runtime_error("Не вдалося створити еталонний файл конфігурації: " + refFile.string());
            }
        }*/
    } catch (const fs::filesystem_error& e) {
        throw std::runtime_error(std::string("Помилка файлової системи при перевірці/створенні папок: ") + e.what());
    } catch (const std::runtime_error& e) {
        throw;
    }
    catch (...) {
        throw std::runtime_error("Невідома помилка під час перевірки/створення папок.");
    }
}

// Ці методи залишаються як заглушки (лєгєнди)
void AppInitializer::loadInitialSettings() {
    // std::cout << "Loading initial settings..." << std::endl;
}

void AppInitializer::initializeComponents() {
    // std::cout << "Initializing components..." << std::endl;
}
