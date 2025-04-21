#include "main.h"             // Включає оголошення ConfigEditor, FileValidator
#include "pugixml/pugixml.hpp"
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <limits>             // Для numeric_limits
#include <cctype>             // Для tolower (використовується в modifySettings)
#include <sstream>            // Для validateValue

// Переконайся, що FileValidator оголошений у main.h або окремому файлі
// #include "FileValidator.h"

using namespace std;
namespace fs = std::filesystem; // Переконайся, що цей рядок є

// --- Допоміжна функція validateValue (виправлена версія) ---
bool validateValue(const string& settingName, const string& value) {
    try {
        // Спочатку перевірка для windowMode, бо він може бути цілим числом 0, 1, 2
         if (settingName == "windowMode") {
            int intVal;
            std::istringstream iss(value);
            iss >> intVal;
            // Перевіряємо, чи вдалося зчитати, чи немає залишку, і чи значення в діапазоні
            if (!iss.fail() && iss.eof() && intVal >= 0 && intVal <= 2) {
                 return true;
            } else {
                 return false;
            }
        }

        // Для решти спробуємо перетворити на float
        float val = stof(value); // stof може кинути виняток std::invalid_argument або std::out_of_range

        // Повний список тегів для перевірки діапазону 0.0-1.0
        if (settingName == "masterVolume" || settingName == "volume_micVivox" ||
            settingName == "volume_vehicles" || settingName == "volume_music" ||
            settingName == "volume_effects" || settingName == "volume_ambient" ||
            settingName == "volume_gui" || settingName == "volume_voice" ||
            settingName == "colorGradingStrength" || settingName == "brightnessDeferred" ||
            settingName == "contrastDeferred" || settingName == "saturationDeferred") {
            return val >= 0.0f && val <= 1.0f;
        }

        // Чутливість (припускаємо діапазон 0.0-10.0)
        if (settingName.find("sensitivity") != string::npos) {
             return val >= 0.0f && val <= 10.0f;
        }

        // Роздільна здатність
        if (settingName == "windowedWidth" || settingName == "fullscreenWidth") {
            return val >= 800; // Мінімальна ширина
        }
        if (settingName == "windowedHeight" || settingName == "fullscreenHeight") {
            return val >= 600; // Мінімальна висота
        }

        // Частота оновлення
        if (settingName == "fullscreenRefresh") {
            return val >= 50; // Мінімальна частота
        }

    } catch (...) { // Ловимо будь-які винятки (invalid_argument, out_of_range)
         return false; // Якщо сталася помилка конвертації - значення невалідний
    }
    // Якщо перевірка для цього налаштування не визначена, вважаємо валідним
    return true;
}

// --- Метод читання налаштувань (з валідацією та оригінальним виводом) ---
void ConfigEditor::readCurrentSettings() {
    FileValidator fileValidator;
    string configPath = "User Configs";
    vector<fs::path> configFilesPaths; // Шляхи
    vector<string> configFilesNames;   // Імена для виводу користувачу

    cout << "Reading settings from '" << configPath << "'..." << endl;

     if (!fs::exists(configPath) || !fs::is_directory(configPath)) {
         cerr << "Error: Directory '" << configPath << "' not found." << endl;
         return;
     }
    try {
         for (const auto& entry : fs::directory_iterator(configPath)) {
             if (entry.is_regular_file() && entry.path().extension() == ".xml") {
                 configFilesPaths.push_back(entry.path());
                 configFilesNames.push_back(entry.path().filename().string()); // Зберігаємо лише ім'я
             }
         }
    } catch (const fs::filesystem_error& e) {
        cerr << "Error reading directory '" << configPath << "': " << e.what() << endl;
        return;
    }

    if (configFilesNames.empty()) {
        cout << "No configuration files available to view in '" << configPath << "'." << endl;
        return;
    }

    // Відображення списку файлів (імена файлів)
    cout << "Available configuration files:" << endl;
    for (size_t i = 0; i < configFilesNames.size(); ++i) {
        cout << i + 1 << ". " << configFilesNames[i] << endl;
    }
     cout << "0. Cancel" << endl;

    // Отримання вибору користувача
    int choice; // Використовуємо int для узгодженості з іншими функціями
    cout << "Enter the number of the file you want to view: ";
      while (!(cin >> choice) || choice < 0 || choice > static_cast<int>(configFilesNames.size())) {
          cout << "Invalid input. Please enter a number between 0 and " << configFilesNames.size() << ": ";
          cin.clear();
          cin.ignore(numeric_limits<streamsize>::max(), '\n');
     }
     cin.ignore(numeric_limits<streamsize>::max(), '\n');
    if (choice == 0) { cout << "Operation cancelled." << endl; return; }

    fs::path selectedFile = configFilesPaths[choice - 1]; // Отримуємо шлях fs::path

    // --- БЛОК ВАЛІДАЦІЇ ---
    cout << "\nValidating selected file (" << selectedFile.filename().string() << ") before displaying..." << endl;
     if (!fileValidator.isXmlWellFormed(selectedFile)) {
        cerr << "Error: Selected file is not a well-formed XML. Cannot display settings." << endl;
        return;
    }
    if (!fileValidator.hasExpectedStructure(selectedFile)) {
        cerr << "Warning: Selected file might be missing expected WoT structure sections. Displaying available data..." << endl;
    }
     vector<string> valueErrors = fileValidator.findInvalidSimpleValues(selectedFile);
    if (!valueErrors.empty()) {
        cout << "\n--- Validation Warnings for " << selectedFile.filename().string() << " ---" << endl;
        for(const auto& err : valueErrors) cout << "  - " << err << endl;
        cout << "------------------------------------" << endl;
    } else {
        // Можна прибрати це повідомлення
        // cout << "Selected file validation passed." << endl;
    }
    cout << "Proceeding to display settings..." << endl;
    // --- КІНЕЦЬ БЛОКУ ВАЛІДАЦІЇ ---

    // --- Завантаження та виведення (як у твоєму робочому коді) ---
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(selectedFile.string().c_str()); // Використовуємо рядок

    if (!result) {
        cout << "Failed to load file: " << result.description() << endl;
        return;
    }

    cout << "\n--- Displaying settings from: " << selectedFile.filename().string() << " ---" << endl;

    // Визначення полів (як у твоєму коді)
    unordered_set<string> soundSettings = {"masterVolume", "volume_micVivox", "volume_vehicles",
                                           "volume_music", "volume_effects", "volume_ambient",
                                           "volume_gui", "volume_voice", "soundMode"};
    unordered_set<string> controlSettings = {"horzInvert", "vertInvert", "keySensitivity",
                                             "sensitivity", "scrollSensitivity"};
    unordered_set<string> deviceSettings = {"windowMode", "windowedWidth", "windowedHeight",
                                            "fullscreenWidth", "fullscreenHeight",
                                            "fullscreenRefresh", "aspectRatio"};

    auto root = doc.child("root"); if (!root) { /*...*/ return; }
    auto scriptsPreferences = root.child("scriptsPreferences");

    // Sound
    cout << "\nSound Settings:" << endl;
    if (scriptsPreferences) {
        auto soundPrefs = scriptsPreferences.child("soundPrefs");
        if (soundPrefs) {
            bool found = false;
            for (auto& setting : soundPrefs.children()) {
                string name = setting.name();
                if (soundSettings.count(name)) {
                    cout << "  " << name << ": " << setting.text().as_string() << endl;
                    found = true;
                }
            }
             if (!found) cout << "  (No matching settings found)" << endl;
        } else { cout << "  (Section <soundPrefs> not found)" << endl; }
    } else { cout << "  (Section <scriptsPreferences> not found)" << endl; }


    // Graphics
    auto graphicsPreferences = root.child("graphicsPreferences");
    cout << "\nGraphics Settings:" << endl;
    if (graphicsPreferences) {
         cout << "  " << "graphicsSettingsVersion: " << graphicsPreferences.child("graphicsSettingsVersion").text().as_string() << endl;
         cout << "  " << "graphicsSettingsVersionMinor: " << graphicsPreferences.child("graphicsSettingsVersionMinor").text().as_string() << endl;
         cout << "  " << "graphicsSettingsVersionMaintainance: " << graphicsPreferences.child("graphicsSettingsVersionMaintainance").text().as_string() << endl;
         cout << "  " << "graphicsSettingsStatus: " << graphicsPreferences.child("graphicsSettingsStatus").text().as_string() << endl;
         for (auto& entry : graphicsPreferences.children("entry")) {
             string label = entry.child("label").text().as_string();
             string activeOption = entry.child("activeOption").text().as_string();
             cout << "  " << label << ": " << activeOption << endl;
         }
         cout << "  " << "ParticlSystemNoRenderGroup: " << graphicsPreferences.child("ParticlSystemNoRenderGroup").text().as_string() << endl;
         cout << "  " << "distributionLevel: " << graphicsPreferences.child("distributionLevel").text().as_string() << endl;
         cout << "  " << "colorGradingStrength: " << graphicsPreferences.child("colorGradingStrength").text().as_string() << endl;
         cout << "  " << "brightnessDeferred: " << graphicsPreferences.child("brightnessDeferred").text().as_string() << endl;
         cout << "  " << "contrastDeferred: " << graphicsPreferences.child("contrastDeferred").text().as_string() << endl;
         cout << "  " << "saturationDeferred: " << graphicsPreferences.child("saturationDeferred").text().as_string() << endl;
    } else { cout << "  (Graphics preferences section not found)" << endl; }

    // Control
    cout << "\nControl Settings:" << endl;
     if (scriptsPreferences) {
         auto controlMode = scriptsPreferences.child("controlMode");
         if (controlMode) {
              bool found = false;
              for (auto& mode : controlMode.children()) {
                  auto camera = mode.child("camera");
                  if (camera) {
                     for (auto& setting : camera.children()) {
                           string name = setting.name();
                           if (controlSettings.count(name)) {
                               cout << "  " << mode.name() << "/" << name << ": " << setting.text().as_string() << endl; // Виправлено тут
                               found = true;
                           }
                     }
                  }
              }
               if (!found) cout << "  (No matching control settings found)" << endl;
         } else { cout << "  (Control mode section not found)" << endl; }
    } // Немає else, бо вже перевірили scriptsPreferences

    // Device
    auto devicePreferences = root.child("devicePreferences");
    cout << "\nDevice Settings:" << endl;
    if (devicePreferences) {
        bool found = false;
        for (auto& setting : devicePreferences.children()) {
            string name = setting.name();
            if (deviceSettings.count(name)) {
                cout << "  " << name << ": " << setting.text().as_string() << endl;
                found = true;
            }
        }
         if (!found) cout << "  (No matching device settings found)" << endl;
    } else { cout << "  (Device settings section not found)" << endl; }

     cout << "\n--- End of settings ---" << endl;
}


// --- Метод редагування налаштувань (з валідацією) ---
// ТУТ ПОВИНЕН БУТИ ЛИШЕ ОДИН МЕТОД MODIFYSETTINGS
void ConfigEditor::modifySettings() {
    FileValidator fileValidator;
    string configPath = "User Configs";
    vector<fs::path> configFilesPaths; // Шляхи
    vector<string> configFilesNames;   // Імена

    cout << "Editing settings in '" << configPath << "'..." << endl;

     if (!fs::exists(configPath) || !fs::is_directory(configPath)) { /* ... */ return; }
    // ... (пошук файлів та заповнення векторів) ...
      try {
           for (const auto& entry : fs::directory_iterator(configPath)) {
               if (entry.is_regular_file() && entry.path().extension() == ".xml") {
                    configFilesPaths.push_back(entry.path());
                    configFilesNames.push_back(entry.path().filename().string());
               }
           }
      } catch (const fs::filesystem_error& e) { /* ... */ return; }
    if (configFilesNames.empty()) { /* ... */ return; }

    // Вибір файлу (як у твоєму коді)
    cout << "Available configuration files:" << endl;
    for (size_t i = 0; i < configFilesNames.size(); ++i) {
         cout << i + 1 << ". " << configFilesNames[i] << endl; // Виводимо імена
    }
    cout << "0. Exit." << endl;

    int choice; // Змінив size_t на int
    cout << "Enter the number of the file you want to modify: ";
    while (!(cin >> choice) || choice < 0 || choice > static_cast<int>(configFilesNames.size())) {
         cout << "Invalid input. Please enter a number between 0 and " << configFilesNames.size() << ": ";
         cin.clear();
         cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Очистка буфера

    if (choice == 0) {
        cout << "Exiting configuration editor." << endl;
        return;
    }

    // **ВИПРАВЛЕННЯ:** Оголошуємо ПЕРЕД блоком валідації
    fs::path selectedFile = configFilesPaths[choice - 1]; // Шлях для валідації
    string selectedFileStr = selectedFile.string();      // Рядок для pugixml

    // --- ВАЛІДАЦІЯ ПЕРЕД РЕДАГУВАННЯМ ---
      cout << "\nValidating file before editing (" << selectedFile.filename().string() << ")..." << endl;
      if (!fileValidator.isXmlWellFormed(selectedFile)) { // Використовуємо selectedFile (fs::path)
        cerr << "Error: Selected file is not a well-formed XML. Cannot edit." << endl;
        return;
    }
    if (!fileValidator.hasExpectedStructure(selectedFile)) { // Використовуємо selectedFile (fs::path)
        cerr << "Error: Selected file is missing expected WoT structure. Cannot edit reliably." << endl;
        return;
    }
     vector<string> valueErrors = fileValidator.findInvalidSimpleValues(selectedFile); // Використовуємо selectedFile (fs::path)
     if (!valueErrors.empty()) {
         cout << "\n--- Validation Warnings for file before editing ---" << endl;
          for(const auto& err : valueErrors) cout << "  - " << err << endl;
         cout << "-------------------------------------------------" << endl;
         // Продовжуємо, дозволяючи користувачу виправити помилки
     } else {
          cout << "File validation passed." << endl;
     }
     // --- КІНЕЦЬ ВАЛІДАЦІЇ ---


    // --- Логіка редагування (як у твоєму коді, використовуємо selectedFileStr) ---
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(selectedFileStr.c_str());
    if (!result) {
         cout << "Failed to load file for editing: " << result.description() << endl;
         return;
    }

    unordered_map<string, pugi::xml_node> categories;
    auto root = doc.child("root");
    if (!root) { /* ... */ return; }

    categories["Sound Settings"] = root.child("scriptsPreferences").child("soundPrefs");
    categories["Graphics Settings"] = root.child("graphicsPreferences");
    categories["Control Settings"] = root.child("scriptsPreferences").child("controlMode");
    categories["Device Settings"] = root.child("devicePreferences");

    unordered_set<string> hiddenSettings = { /* ... твій набір ... */ };

    // Вибір категорії (з виправленням categoryNames)
    vector<string> categoryNames;
    cout << "\nAvailable categories:" << endl; // Додав перенос рядка
    size_t catIndex = 1; // Використовуємо size_t для індексу
    const vector<string> orderedCategoryKeys = {"Sound Settings", "Graphics Settings", "Control Settings", "Device Settings"};
    for (const auto& key : orderedCategoryKeys) {
        if (categories.count(key) && categories[key]) {
             cout << catIndex++ << ". " << key << endl;
             categoryNames.push_back(key);
        }
    }
    cout << "0. Exit." << endl;

    int categoryChoice; // Використовуємо int
    cout << "Enter the number of the category you want to modify: ";
     while (!(cin >> categoryChoice) || categoryChoice < 0 || categoryChoice > static_cast<int>(categoryNames.size())) { /* ... */ }
     cin.ignore(numeric_limits<streamsize>::max(), '\n');
    if (categoryChoice == 0) { /* ... */ return; }

    string selectedCategory = categoryNames[categoryChoice - 1];
    auto selectedNode = categories[selectedCategory];

    // Вибір налаштування
    vector<pair<string, pugi::xml_node>> settingsList;
    cout << "\n" << selectedCategory << ":" << endl; // Додав перенос рядка

    // Заповнення settingsList (як у твоєму коді)
    size_t settingIndex = 1; // Індекс для виведення користувачу
    if (selectedCategory == "Control Settings") {
        for (auto& mode : selectedNode.children()) {
            string modeName = mode.name();
            auto cameraNode = mode.child("camera");
            if (cameraNode) {
                for (auto& setting : cameraNode.children()) { // Ітеруємо по налаштуваннях камери
                    string settingName = setting.name();
                    if (settingName == "sensitivity" && hiddenSettings.find("sensitivity") == hiddenSettings.end()) { // Показуємо лише sensitivity
                         string displayName = modeName + " sensitivity";
                         cout << "  " << settingIndex++ << ". " << displayName << ": " << setting.text().as_string() << endl;
                         settingsList.emplace_back(displayName, setting); // Зберігаємо сам вузол налаштування
                    }
                    // Додай сюди інші controlSettings, якщо потрібно редагувати
                }
            }
        }
    } else {
        for (auto& setting : selectedNode.children()) {
            string name = setting.name();
            if (hiddenSettings.find(name) == hiddenSettings.end()) {
                 cout << "  " << settingIndex++ << ". " << name << ": " << setting.text().as_string() << endl;
                 settingsList.emplace_back(name, setting);
            }
        }
    }

     if (settingsList.empty()) {
          cout << "No modifiable settings found in this category." << endl;
          return;
     }
    cout << "0. Exit." << endl;

    int settingChoice; // Використовуємо int
    cout << "Enter the number of the setting you want to modify: ";
      while (!(cin >> settingChoice) || settingChoice < 0 || settingChoice > static_cast<int>(settingsList.size())) { /* ... */ }
      cin.ignore(numeric_limits<streamsize>::max(), '\n');
    if (settingChoice == 0) { /* ... */ return; }

    // Зверни увагу: тут індекс для вектора на 1 менший за вибір користувача
    auto& [selectedSettingName, selectedNodeSetting] = settingsList[settingChoice - 1];

    // Введення нового значення
    cout << "Enter new value for " << selectedSettingName << " (current: " << selectedNodeSetting.text().as_string() << "): ";
    string newValue;
    getline(cin, newValue); // Читаємо весь рядок

    // Перевірка нового значення через validateValue
    if (!validateValue(selectedSettingName, newValue)) {
        cout << "Invalid value entered for " << selectedSettingName << ". Please ensure it is within the allowed range/format." << endl;
        return;
    }

    // Збереження
    selectedNodeSetting.text().set(newValue.c_str());
    if (doc.save_file(selectedFileStr.c_str())) {
        cout << "Settings updated successfully." << endl;
    } else {
        cout << "Failed to save changes." << endl;
    }
}