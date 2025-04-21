#include "main.h"             // Включає ConfigManager, FileValidator
#include "pugixml/pugixml.hpp" // Потрібен для viewCurrentGameConfig
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <cstdlib>             // Для getenv
#include <limits>              // Для numeric_limits
#include <unordered_set>     // Потрібен для viewCurrentGameConfig
#include <cctype>              // Для tolower

using namespace std;
namespace fs = std::filesystem;

// --- Метод зміни поточного конфігу (з валідацією) ---
void ConfigManager::changeCurrentConfig() {
    cout << "Changing current game config..." << endl;
    FileValidator fileValidator; // Валідатор

    // --- Шляхи ---
    const char* appDataPath = getenv("APPDATA");
    if (!appDataPath) {
        cerr << "Error: Unable to get APPDATA environment variable." << endl;
        return;
    }
    fs::path targetDir = fs::path(appDataPath) / "Wargaming.net" / "WorldOfTanks";
    fs::path targetFile = targetDir / "preferences.xml";
    fs::path sourceDir = "User Configs";

    // --- Вибір файлу з User Configs ---
    vector<fs::path> configFilesPaths; // Шляхи
    vector<string> configFilesNames;  // Імена

    if (!fs::exists(sourceDir) || !fs::is_directory(sourceDir)) {
        cerr << "Error: Directory '" << sourceDir.string() << "' not found." << endl;
        return;
    }
    try {
        for (const auto& entry : fs::directory_iterator(sourceDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".xml") {
                configFilesPaths.push_back(entry.path());
                configFilesNames.push_back(entry.path().filename().string());
            }
        }
    } catch (const fs::filesystem_error& e) {
        cerr << "Error reading directory '" << sourceDir.string() << "': " << e.what() << endl;
        return;
    }

    if (configFilesNames.empty()) {
        cout << "No config files found in '" << sourceDir.string() << "'." << endl;
        return;
    }

    cout << "\nAvailable config files in '" << sourceDir.string() << "':" << endl;
    for (size_t i = 0; i < configFilesNames.size(); ++i) {
        cout << i + 1 << ". " << configFilesNames[i] << endl;
    }
    cout << "0. Cancel" << endl;

    int choice = -1;
    cout << "Enter the number of the config file to apply: ";
    while (!(cin >> choice) || choice < 0 || choice > static_cast<int>(configFilesNames.size())) {
        cout << "Invalid input. Please enter a number between 0 and " << configFilesNames.size() << ": ";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    if (choice == 0) {
        cout << "Operation cancelled." << endl;
        return;
    }
    fs::path selectedSourceFile = configFilesPaths[choice - 1];

    // --- ВАЛІДАЦІЯ ВИБРАНОГО ФАЙЛУ ---
    cout << "Validating selected config file before applying..." << endl;
     if (!fileValidator.isXmlWellFormed(selectedSourceFile)) {
        cerr << "Error: Selected file is not a well-formed XML. Operation aborted." << endl;
        return;
    }
    if (!fileValidator.hasExpectedStructure(selectedSourceFile)) {
        cerr << "Error: Selected file is missing expected WoT structure. Operation aborted." << endl;
        return;
    }
    vector<string> valueErrors = fileValidator.findInvalidSimpleValues(selectedSourceFile);
    if (!valueErrors.empty()) {
         cout << "\n--- Validation Warnings for selected file ---" << endl;
         for(const auto& err : valueErrors) cout << "  - " << err << endl;
         cout << "-------------------------------------------" << endl;
        cout << "Do you want to continue applying this config despite the warnings? (y/n): ";
        char confirm = 'n';
        cin >> confirm;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        if (tolower(confirm) != 'y') {
            cout << "Operation cancelled by user." << endl;
            return;
        }
          cout << "Continuing operation despite warnings..." << endl;
    } else {
         cout << "Selected file validation passed." << endl;
    }
    // --- КІНЕЦЬ ВАЛІДАЦІЇ ---

    // --- Логіка застосування (з видаленням) ---
    cout << "Applying '" << selectedSourceFile.filename().string() << "' as current game config..." << endl;
    try {
         if (!fs::exists(targetDir)) { fs::create_directories(targetDir); }
         if (fs::exists(targetFile)) {
             cout << "Removing existing game config file: " << targetFile.string() << endl;
             fs::remove(targetFile);
         }
        cout << "Copying " << selectedSourceFile.filename().string() << " to " << targetFile.string() << endl;
        fs::copy_file(selectedSourceFile, targetFile, fs::copy_options::overwrite_existing);
        cout << "Successfully applied '" << selectedSourceFile.filename().string() << "' as '" << targetFile.filename().string() << "'." << endl;
    } catch (const fs::filesystem_error& e) {
        cerr << "Error applying config file: " << e.what() << endl;
        cerr << "Path1: " << e.path1().string() << endl;
        cerr << "Path2: " << e.path2().string() << endl;
    }
}

// --- Метод перегляду поточного конфігу гри (з валідацією) ---
void ConfigManager::viewCurrentGameConfig() {
     cout << "Viewing current game configuration..." << endl;
     FileValidator fileValidator; // Валідатор

     // --- Шлях до файлу гри ---
     const char* appDataPath = getenv("APPDATA");
     if (!appDataPath) { /* ... */ return; }
     fs::path gameConfigPath = fs::path(appDataPath) / "Wargaming.net" / "WorldOfTanks" / "preferences.xml";

     // --- ВАЛІДАЦІЯ ФАЙЛУ ГРИ ---
     cout << "Validating game config file..." << endl;
      if (!fs::exists(gameConfigPath)) {
           cerr << "Error: Game configuration file not found at: " << gameConfigPath.string() << endl;
           return;
      }
       if (!fs::is_regular_file(gameConfigPath)) { // Додав перевірку, що це файл
           cerr << "Error: The path found is not a regular file: " << gameConfigPath.string() << endl;
           return;
      }
      if (!fileValidator.isXmlWellFormed(gameConfigPath)) {
         cerr << "Error: Current game config file is not a well-formed XML. Cannot display settings." << endl;
         return;
     }
     if (!fileValidator.hasExpectedStructure(gameConfigPath)) {
         cerr << "Warning: Current game config file might be missing expected WoT structure sections. Displaying available data..." << endl;
     }
     vector<string> valueErrors = fileValidator.findInvalidSimpleValues(gameConfigPath);
     if (!valueErrors.empty()) {
         cout << "\n--- Validation Warnings for current game config ---" << endl;
         for(const auto& err : valueErrors) cout << "  - " << err << endl;
         cout << "-------------------------------------------------" << endl;
     } else {
          cout << "Game config validation passed." << endl;
     }
     // --- КІНЕЦЬ ВАЛІДАЦІЇ ---

     // --- Логіка відображення налаштувань (як у твоєму оригінальному ConfigEditor::readCurrentSettings) ---
     cout << "\n--- Displaying settings from: " << gameConfigPath.string() << " ---" << endl;
     pugi::xml_document doc;
     pugi::xml_parse_result result = doc.load_file(gameConfigPath.string().c_str()); // Використовуємо шлях
      if (!result) {
         cerr << "Error loading XML file again for display: " << result.description() << endl; // Помилка навіть після валідації? Дивно, але перевіримо.
         return;
      }

     // Визначення полів (як у твоєму ConfigEditor)
     unordered_set<string> soundSettings = {"masterVolume", "volume_micVivox", "volume_vehicles",
                                           "volume_music", "volume_effects", "volume_ambient",
                                           "volume_gui", "volume_voice", "soundMode"};
     unordered_set<string> controlSettings = {"horzInvert", "vertInvert", "keySensitivity",
                                             "sensitivity", "scrollSensitivity"};
     unordered_set<string> deviceSettings = {"windowMode", "windowedWidth", "windowedHeight",
                                            "fullscreenWidth", "fullscreenHeight",
                                            "fullscreenRefresh", "aspectRatio"};

     // --- Вивід ---
     auto root = doc.child("root");
     if (!root) { cout << "Root node <root> not found." << endl; return; }

     auto scriptsPreferences = root.child("scriptsPreferences");
     if (scriptsPreferences) { // Додав перевірку
         auto soundPrefs = scriptsPreferences.child("soundPrefs");
         cout << "\nSound Settings:" << endl;
         if (soundPrefs) {
             bool found = false;
             for (auto& setting : soundPrefs.children()) {
                 string name = setting.name();
                 if (soundSettings.count(name)) {
                     cout << "  " << name << ": " << setting.text().as_string() << endl;
                     found = true;
                 }
             }
             if (!found) cout << "  (No relevant sound settings found or section empty)" << endl;
         } else { cout << "  (Sound preferences section not found)" << endl; }

         auto controlMode = scriptsPreferences.child("controlMode");
         cout << "\nControl Settings:" << endl;
         if (controlMode) {
              bool found = false;
              for (auto& mode : controlMode.children()) {
                  auto camera = mode.child("camera");
                  if (camera) {
                     for (auto& setting : camera.children()) {
                           string name = setting.name();
                           if (controlSettings.count(name)) {
                               cout << "  " << mode.name() << "/" << name << ": " << setting.text().as_string() << endl;
                               found = true;
                           }
                     }
                  }
              }
              if (!found) cout << "  (No relevant control settings found or section empty)" << endl;
         } else { cout << "  (Control mode section not found)" << endl; }
     } else { cout << "Section <scriptsPreferences> not found." << endl;}


     auto graphicsPreferences = root.child("graphicsPreferences");
     cout << "\nGraphics Settings:" << endl;
     if (graphicsPreferences) {
         // ... (Вивід версій, <entry>, інших параметрів - ЯК У ТВОЄМУ КОДІ ConfigEditor::readCurrentSettings) ...
         cout << "  " << "graphicsSettingsVersion: " << graphicsPreferences.child("graphicsSettingsVersion").text().as_string("N/A") << endl;
         cout << "  " << "graphicsSettingsVersionMinor: " << graphicsPreferences.child("graphicsSettingsVersionMinor").text().as_string("N/A") << endl;
         cout << "  " << "graphicsSettingsVersionMaintainance: " << graphicsPreferences.child("graphicsSettingsVersionMaintainance").text().as_string("N/A") << endl;
         cout << "  " << "graphicsSettingsStatus: " << graphicsPreferences.child("graphicsSettingsStatus").text().as_string("N/A") << endl;
         for (auto& entry : graphicsPreferences.children("entry")) {
             string label = entry.child("label").text().as_string("N/A");
             string activeOption = entry.child("activeOption").text().as_string("N/A");
             cout << "  " << label << ": " << activeOption << endl;
         }
         cout << "  " << "ParticlSystemNoRenderGroup: " << graphicsPreferences.child("ParticlSystemNoRenderGroup").text().as_string("N/A") << endl;
         cout << "  " << "distributionLevel: " << graphicsPreferences.child("distributionLevel").text().as_string("N/A") << endl;
         cout << "  " << "colorGradingStrength: " << graphicsPreferences.child("colorGradingStrength").text().as_string("N/A") << endl;
         cout << "  " << "brightnessDeferred: " << graphicsPreferences.child("brightnessDeferred").text().as_string("N/A") << endl;
         cout << "  " << "contrastDeferred: " << graphicsPreferences.child("contrastDeferred").text().as_string("N/A") << endl;
         cout << "  " << "saturationDeferred: " << graphicsPreferences.child("saturationDeferred").text().as_string("N/A") << endl;
     } else { cout << "  (Graphics preferences section not found)" << endl; }

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
         if (!found) cout << "  (No relevant device settings found or section empty)" << endl;
     } else { cout << "  (Device preferences section not found)" << endl; }

     cout << "--- End of settings ---" << endl;
}

// --- Метод завантаження конфігу (заглушка) ---
void ConfigManager::uploadConfig() {
    cout << "Upload config functionality is not yet implemented." << endl;
}