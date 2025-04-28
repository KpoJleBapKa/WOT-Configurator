#include "main.h" // Головний заголовок (містить FilteredSettingsMap та оголошення ConfigEditor)
#include "pugixml/pugixml.hpp" // Включаємо pugixml ТУТ
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_set> // Потрібен для фільтрів в getFilteredSettings
#include <vector>
#include <map>
#include <utility> // для std::pair
#include <iostream> // Для std::cerr (опційно, для попереджень)

// НЕ використовуємо using namespace std; у .cpp файлах

// Метод для читання всього вмісту файлу як рядка (без змін)
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
        buffer << file.rdbuf(); // Читаємо весь файл в буфер
    } catch (const std::exception& e) {
        // Якщо сталася помилка під час читання
        throw std::runtime_error(std::string("Помилка читання файлу конфігурації: ") + e.what());
    }
    return buffer.str(); // Повертаємо вміст
}


// Метод для отримання відфільтрованих налаштувань (без змін)
FilteredSettingsMap ConfigEditor::getFilteredSettings(const fs::path& configPath) {
    FilteredSettingsMap categorizedSettings; // Мапа для результату: "Категорія" -> { {"Налаштування", "Значення"}, ... }

    // 1. Завантаження та перевірка XML
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(configPath.wstring().c_str()); // Використовуємо wstring().c_str() для шляхів у Windows
    if (!result) {
        throw std::runtime_error("Помилка завантаження XML: " + std::string(result.description()) + " у файлі " + configPath.string());
    }

    pugi::xml_node root = doc.child("root");
    if (!root) {
        throw std::runtime_error("Відсутній кореневий елемент <root> у файлі: " + configPath.string());
    }

    // 2. Визначення фільтрів (які налаштування показувати)
    const std::unordered_set<std::string> soundSettingsFilter = {
        "masterVolume", "volume_micVivox", "volume_vehicles", "volume_music",
        "volume_effects", "volume_ambient", "volume_gui", "volume_voice", "soundMode",
        "bass_boost"
    };
    const std::unordered_set<std::string> controlSettingsFilter = {
        "horzInvert", "vertInvert", "keySensitivity", "scrollSensitivity"
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

    // 3. Обробка секцій та фільтрація
    pugi::xml_node scriptsPreferences = root.child("scriptsPreferences");
    if (scriptsPreferences) {
        // --- Звук ---
        pugi::xml_node soundPrefs = scriptsPreferences.child("soundPrefs");
        if (soundPrefs) {
            std::vector<std::pair<std::string, std::string>> currentCategorySettings;
            for (const auto& setting : soundPrefs.children()) {
                std::string name = setting.name();
                if (soundSettingsFilter.count(name)) {
                    currentCategorySettings.push_back({name, setting.text().as_string()});
                }
            }
            if (!currentCategorySettings.empty()) categorizedSettings["Sound Settings"] = currentCategorySettings;
        }
        // --- Керування ---
        pugi::xml_node controlMode = scriptsPreferences.child("controlMode");
        if (controlMode) {
            std::vector<std::pair<std::string, std::string>> currentCategorySettings;
            for (const auto& mode : controlMode.children()) {
                pugi::xml_node camera = mode.child("camera");
                if (camera) {
                    for (const auto& setting : camera.children()) {
                        std::string name = setting.name();
                        if (controlSettingsFilter.count(name) || name == "sensitivity") {
                            std::string displayName = std::string(mode.name()) + "/" + name;
                            currentCategorySettings.push_back({displayName, setting.text().as_string()});
                        }
                    }
                }
            }
            if (!currentCategorySettings.empty()) categorizedSettings["Control Settings"] = currentCategorySettings;
        }
    }

    // --- Графіка ---
    pugi::xml_node graphicsPreferences = root.child("graphicsPreferences");
    if (graphicsPreferences) {
        std::vector<std::pair<std::string, std::string>> currentCategorySettings;
        for(const std::string& tagName : graphicDirectTagsFilter) {
            pugi::xml_node node = graphicsPreferences.child(tagName.c_str());
            if(node) { currentCategorySettings.push_back({tagName, node.text().as_string()}); }
        }
        for (const auto& entry : graphicsPreferences.children("entry")) {
            std::string label = entry.child_value("label");
            if (!label.empty()) {
                std::string activeOption = entry.child_value("activeOption");
                currentCategorySettings.push_back({label, activeOption});
            }
        }
        if (!currentCategorySettings.empty()) categorizedSettings["Graphics Settings"] = currentCategorySettings;
    }

    // --- Пристрій ---
    pugi::xml_node devicePreferences = root.child("devicePreferences");
    if (devicePreferences) {
        std::vector<std::pair<std::string, std::string>> currentCategorySettings;
        for (const auto& setting : devicePreferences.children()) {
            std::string name = setting.name();
            if (deviceSettingsFilter.count(name)) {
                currentCategorySettings.push_back({name, setting.text().as_string()});
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

    // Ітеруємо по категоріях та налаштуваннях, які ми отримали з діалогу
    for (const auto& categoryPair : settings) {
        const std::string& categoryName = categoryPair.first;
        const auto& settingsInCategory = categoryPair.second;

        // Знаходимо відповідний вузол для категорії
        pugi::xml_node categoryNode; // Вузол для поточної категорії/підкатегорії
        pugi::xml_node scriptsPrefsNode; // Окремо для вкладених
        pugi::xml_node graphicsPrefsNode; // Окремо для вкладених

        // Визначаємо базовий вузол для пошуку/зміни
        if (categoryName == "Sound Settings") {
            scriptsPrefsNode = root.child("scriptsPreferences");
            if (scriptsPrefsNode) categoryNode = scriptsPrefsNode.child("soundPrefs");
        } else if (categoryName == "Graphics Settings") {
            // ВИПРАВЛЕНО: Зберігаємо вузол graphicsPreferences
            graphicsPrefsNode = root.child("graphicsPreferences");
            categoryNode = graphicsPrefsNode; // categoryNode тепер вказує на <graphicsPreferences>
        } else if (categoryName == "Control Settings") {
            scriptsPrefsNode = root.child("scriptsPreferences");
            if (scriptsPrefsNode) categoryNode = scriptsPrefsNode.child("controlMode");
        } else if (categoryName == "Device Settings") {
            categoryNode = root.child("devicePreferences");
        }
        // ... додати інші категорії, якщо потрібно

        if (!categoryNode) {
            std::cerr << "Warning: Category node not found in XML for category during save: " << categoryName << std::endl;
            continue; // Пропускаємо цю категорію
        }

        for (const auto& settingPair : settingsInCategory) {
            const std::string& settingName = settingPair.first;
            const std::string& newValue = settingPair.second;

            pugi::xml_node settingNode; // Вузол, який будемо змінювати

            // Обробка вкладених налаштувань Control
            if (categoryName == "Control Settings" && settingName.find('/') != std::string::npos) {
                size_t slashPos = settingName.find('/');
                std::string modeName = settingName.substr(0, slashPos);
                std::string actualSettingName = settingName.substr(slashPos + 1);
                pugi::xml_node modeNode = categoryNode.child(modeName.c_str()); // categoryNode тут <controlMode>
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
            // Обробка налаштувань Graphics типу <entry>
            // ВИПРАВЛЕНО: Використовуємо categoryNode (який є <graphicsPreferences>) для перевірки прямого дочірнього
            else if (categoryName == "Graphics Settings" && !categoryNode.child(settingName.c_str())) {
                // Це має бути <entry>. Шукаємо <entry> з відповідним <label>.
                bool foundEntry = false;
                for (pugi::xml_node entry : categoryNode.children("entry")) { // Ітеруємо по <entry> всередині <graphicsPreferences>
                    if (entry.child_value("label") == settingName) {
                        settingNode = entry.child("activeOption"); // Знаходимо вузол <activeOption> всередині <entry>
                        foundEntry = true;
                        break;
                    }
                }
                if (!foundEntry) {
                    std::cerr << "Warning: Graphics <entry> node not found for label: " << settingName << " during save." << std::endl;
                    continue; // Пропускаємо, якщо не знайшли
                }
            }
            // Обробка прямих налаштувань (для всіх категорій)
            else {
                settingNode = categoryNode.child(settingName.c_str());
            }

            // Оновлюємо значення знайденого вузла
            if (settingNode) {
                if (!settingNode.text().set(newValue.c_str())) {
                    std::cerr << "Warning: Failed to set text for node: " << settingName << std::endl;
                }
            } else {
                // Вузол не знайдено - логуємо, але не падаємо
                std::cerr << "Warning: Setting node not found in XML for: '" << settingName << "' in category '" << categoryName << "' during save." << std::endl;
            }
        } // кінець for по налаштуваннях
    } // кінець for по категоріях

    // Зберігаємо змінений документ у той самий файл
    if (!doc.save_file(configPath.wstring().c_str())) { // Використовуємо wstring для шляху
        throw std::runtime_error("Не вдалося зберегти зміни у файл: " + configPath.string());
    }
} // кінець saveFilteredSettings
