#include "main.h" // Головний заголовок (містить FilteredSettingsMap та оголошення ConfigEditor)
#include "pugixml/pugixml.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_set> // Потрібен для фільтрів в getFilteredSettings
#include <vector>
#include <map>
#include <utility> // для std::pair
#include <iostream> // Для std::cerr
#include <string>   // Для std::string
#include <algorithm> // Для std::find_if_not
#include <cctype>   // Для std::isspace


namespace {

std::string ltrim(std::string s) {
    s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), ::isspace));
    return s;
}

std::string rtrim(std::string s) {
    s.erase(std::find_if_not(s.rbegin(), s.rend(), ::isspace).base(), s.end());
    return s;
}

std::string trim(std::string s) {
    return ltrim(rtrim(std::move(s)));
}

}

// Метод для читання всього вмісту файлу як рядка
std::string ConfigEditor::readConfigContent(const fs::path& configPath) {
    if (!fs::exists(configPath)) {
        throw std::runtime_error("Файл конфігурації не знайдено: " + configPath.string());
    }
    if (!fs::is_regular_file(configPath)){
        throw std::runtime_error("Шлях не є файлом: " + configPath.string());
    }

    std::ifstream file(configPath);
    if (!file.is_open()) {
        throw std::runtime_error("Не вдалося відкрити файл конфігурації: " + configPath.string());
    }

    std::stringstream buffer;
    try {
        buffer << file.rdbuf();
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Помилка читання файлу конфігурації: ") + e.what());
    }
    return buffer.str();
}

// Метод для отримання відфільтрованих налаштувань
FilteredSettingsMap ConfigEditor::getFilteredSettings(const fs::path& configPath) {
    FilteredSettingsMap categorizedSettings;

    // Завантаження та перевірка XML
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(configPath.wstring().c_str());
    if (!result) {
        throw std::runtime_error("Помилка завантаження XML: " + std::string(result.description()) + " у файлі " + configPath.string());
    }

    pugi::xml_node root = doc.child("root");
    if (!root) {
        throw std::runtime_error("Відсутній кореневий елемент <root> у файлі: " + configPath.string());
    }

    // Визначення фільтрів
    const std::unordered_set<std::string> soundSettingsFilter = {
        "masterVolume", "volume_micVivox", "volume_vehicles", "volume_music",
        "volume_effects", "volume_ambient", "volume_gui", "volume_voice", "soundMode",
        "bass_boost"
    };
    const std::unordered_set<std::string> controlSettingsFilter = {
        "horzInvert", "vertInvert", "keySensitivity", "sensitivity", "scrollSensitivity"
    };
    const std::unordered_set<std::string> deviceSettingsFilter = {
        "windowMode", "windowedWidth", "windowedHeight", "fullscreenWidth",
        "fullscreenHeight", "fullscreenRefresh", "aspectRatio", "gamma", "tripleBuffering"
    };
    const std::vector<std::string> graphicDirectTagsFilter = {
        "graphicsSettingsVersion", "graphicsSettingsVersionMinor", "graphicsSettingsVersionMaintainance",
        "graphicsSettingsStatus", "ParticlSystemNoRenderGroup", "distributionLevel",
        "colorGradingStrength", "brightnessDeferred", "contrastDeferred", "saturationDeferred"
    };

    pugi::xml_node scriptsPreferences = root.child("scriptsPreferences");
    if (scriptsPreferences) {
        // Звук
        pugi::xml_node soundPrefs = scriptsPreferences.child("soundPrefs");
        if (soundPrefs) {
            std::vector<std::pair<std::string, std::string>> currentCategorySettings;
            for (const auto& setting : soundPrefs.children()) {
                std::string name = trim(setting.name());
                if (soundSettingsFilter.count(name)) {
                    currentCategorySettings.push_back({name, trim(setting.text().as_string())});
                }
            }
            if (!currentCategorySettings.empty()) categorizedSettings["Sound Settings"] = currentCategorySettings;
        }
        // Керування
        pugi::xml_node controlMode = scriptsPreferences.child("controlMode");
        if (controlMode) {
            std::vector<std::pair<std::string, std::string>> currentCategorySettings;
            for (const auto& mode : controlMode.children()) {
                std::string modeName = trim(mode.name());
                pugi::xml_node camera = mode.child("camera");
                if (camera) {
                    for (const auto& setting : camera.children()) {
                        std::string settingNamePart = trim(setting.name());
                        if (controlSettingsFilter.count(settingNamePart)) {
                            std::string fullSettingName = modeName + "/" + settingNamePart; // Формуємо повний ключ
                            currentCategorySettings.push_back({fullSettingName, trim(setting.text().as_string())});
                        }
                    }
                }
            }
            if (!currentCategorySettings.empty()) categorizedSettings["Control Settings"] = currentCategorySettings;
        }
    }

    // Графіка
    pugi::xml_node graphicsPreferences = root.child("graphicsPreferences");
    if (graphicsPreferences) {
        std::vector<std::pair<std::string, std::string>> currentCategorySettings;
        // Обробка прямих тегів
        for(const std::string& tagName : graphicDirectTagsFilter) {
            pugi::xml_node node = graphicsPreferences.child(tagName.c_str());
            if(node) {
                currentCategorySettings.push_back({tagName, trim(node.text().as_string())});
            }
        }
        // Обробка тегів <entry>
        for (const auto& entry : graphicsPreferences.children("entry")) {
            std::string label = entry.child_value("label");
            std::string trimmed_label = trim(label);
            if (!trimmed_label.empty()) {
                std::string activeOption = entry.child_value("activeOption");
                currentCategorySettings.push_back({trimmed_label, trim(activeOption)});
            }
        }
        if (!currentCategorySettings.empty()) categorizedSettings["Graphics Settings"] = currentCategorySettings;
    }

    // Пристрій
    pugi::xml_node devicePreferences = root.child("devicePreferences");
    if (devicePreferences) {
        std::vector<std::pair<std::string, std::string>> currentCategorySettings;
        for (const auto& setting : devicePreferences.children()) {
            std::string name = trim(setting.name());
            if (deviceSettingsFilter.count(name)) {
                currentCategorySettings.push_back({name, trim(setting.text().as_string())});
            }
        }
        if (!currentCategorySettings.empty()) categorizedSettings["Device Settings"] = currentCategorySettings;
    }

    return categorizedSettings;
}

// Метод для збереження змін в XML
void ConfigEditor::saveFilteredSettings(const fs::path& configPath, const FilteredSettingsMap& settings) {
    pugi::xml_document doc;
    pugi::xml_parse_result loadResult = doc.load_file(configPath.wstring().c_str());

    if (!loadResult) {
        throw std::runtime_error("Не вдалося завантажити файл для збереження: " + configPath.string() + " (" + loadResult.description() + ")");
    }

    pugi::xml_node root = doc.child("root");
    if (!root) {
        throw std::runtime_error("Не знайдено <root> елемент у файлі: " + configPath.string());
    }

    for (const auto& categoryPair : settings) {
        const std::string& categoryName = categoryPair.first;
        const auto& settingsInCategory = categoryPair.second;

        pugi::xml_node categoryNode;
        pugi::xml_node scriptsPrefsNode;
        pugi::xml_node graphicsPrefsNode;

        if (categoryName == "Sound Settings") {
            scriptsPrefsNode = root.child("scriptsPreferences");
            if (scriptsPrefsNode) categoryNode = scriptsPrefsNode.child("soundPrefs");
        } else if (categoryName == "Graphics Settings") {
            graphicsPrefsNode = root.child("graphicsPreferences");
            categoryNode = graphicsPrefsNode;
        } else if (categoryName == "Control Settings") {
            scriptsPrefsNode = root.child("scriptsPreferences");
            if (scriptsPrefsNode) categoryNode = scriptsPrefsNode.child("controlMode");
        } else if (categoryName == "Device Settings") {
            categoryNode = root.child("devicePreferences");
        }

        if (!categoryNode) {
            std::cerr << "Warning: Category node not found in XML for category during save: " << categoryName << std::endl;
            continue;
        }

        for (const auto& settingPair : settingsInCategory) {
            const std::string& settingName = settingPair.first;
            const std::string& newValue = settingPair.second;

            pugi::xml_node settingNode;

            if (categoryName == "Control Settings" && settingName.find('/') != std::string::npos) {
                size_t slashPos = settingName.find('/');
                std::string modeName = settingName.substr(0, slashPos);
                std::string actualSettingName = settingName.substr(slashPos + 1);
                pugi::xml_node modeNode = categoryNode.child(modeName.c_str());
                if(modeNode) {
                    pugi::xml_node cameraNode = modeNode.child("camera");
                    if(cameraNode) {
                        settingNode = cameraNode.child(actualSettingName.c_str());
                    } else {
                        std::cerr << "Warning: <camera> node not found for mode '" << modeName << "' during save." << std::endl; continue;
                    }
                } else {
                    std::cerr << "Warning: Control mode node not found: '" << modeName << "' during save." << std::endl; continue;
                }
            }
            else if (categoryName == "Graphics Settings") {
                settingNode = categoryNode.child(settingName.c_str());
                if (!settingNode) {
                    bool foundEntry = false;
                    for (pugi::xml_node entry : categoryNode.children("entry")) {
                        if (trim(entry.child_value("label")) == settingName) {
                            settingNode = entry.child("activeOption");
                            foundEntry = true;
                            break;
                        }
                    }
                    if (!foundEntry) {
                        std::cerr << "Warning: Graphics node (direct or entry/label) not found for key: " << settingName << " during save." << std::endl;
                        continue;
                    }
                }
            }
            // Обробка для інших категорій (Sound, Device)
            else {
                settingNode = categoryNode.child(settingName.c_str());
            }

            if (settingNode) {
                if (!settingNode.text().set(newValue.c_str())) {
                    std::cerr << "Warning: Failed to set text for node: " << settingName << std::endl;
                }
            } else {
                std::cerr << "Warning: Setting node could not be found in XML for key: '" << settingName << "' in category '" << categoryName << "' during save." << std::endl;
            }
        }
    }

    if (!doc.save_file(configPath.wstring().c_str())) {
        throw std::runtime_error("Не вдалося зберегти зміни у файл: " + configPath.string());
    }
}
