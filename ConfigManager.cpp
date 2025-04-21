// --- Файл ConfigManager.cpp ---
#include "main.h"
#include "pugixml/pugixml.hpp" // <-- Додаємо PugiXML
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <limits>
#include <unordered_set> // <-- Потрібно для списків налаштувань

// Використовуємо простір імен для зручності
using namespace std;
namespace fs = std::filesystem;

// --- Новий метод viewCurrentGameConfig ---
void ConfigManager::viewCurrentGameConfig() {
    cout << "Viewing current game configuration..." << endl;

    // --- Визначаємо шлях до preferences.xml гри ---
    const char* appDataPath = getenv("APPDATA");
    if (!appDataPath) {
        cerr << "Error: Unable to get APPDATA environment variable." << endl;
        return;
    }
    fs::path gameConfigPath = fs::path(appDataPath) / "Wargaming.net" / "WorldOfTanks" / "preferences.xml";

    // --- Перевіряємо, чи файл існує ---
    if (!fs::exists(gameConfigPath)) {
        cerr << "Error: Game configuration file not found at:" << endl;
        cerr << gameConfigPath.string() << endl;
        cerr << "Please ensure the game has been run at least once." << endl;
        return;
    }
     if (!fs::is_regular_file(gameConfigPath)) {
           cerr << "Error: The path found is not a regular file:" << endl;
           cerr << gameConfigPath.string() << endl;
           return;
      }

    // --- Завантаження та парсинг XML (логіка схожа на ConfigEditor) ---
    cout << "\n--- Displaying settings from: " << gameConfigPath.string() << " ---" << endl;
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(gameConfigPath.c_str());

    if (!result) {
        cerr << "Error loading XML file: " << result.description() << endl;
        cerr << "File path: " << gameConfigPath.string() << endl;
        return;
    }

    // Визначення полів (дублюємо з ConfigEditor, або виносимо в спільний header/namespace)
     unordered_set<string> soundSettings = {"masterVolume", "volume_micVivox", "volume_vehicles",
                                           "volume_music", "volume_effects", "volume_ambient",
                                           "volume_gui", "volume_voice", "soundMode"};
    unordered_set<string> controlSettings = {"horzInvert", "vertInvert", "keySensitivity",
                                             "sensitivity", "scrollSensitivity"};
    unordered_set<string> deviceSettings = {"windowMode", "windowedWidth", "windowedHeight",
                                            "fullscreenWidth", "fullscreenHeight",
                                            "fullscreenRefresh", "aspectRatio"};

    // Логіка парсингу та виводу (також дублюється)
    auto root = doc.child("root");
    if (!root) {
        cout << "Root node <root> not found in " << gameConfigPath.filename().string() << "." << endl;
        return;
    }

    // --- Sound Settings ---
    auto scriptsPreferences = root.child("scriptsPreferences");
    auto soundPrefs = scriptsPreferences ? scriptsPreferences.child("soundPrefs") : pugi::xml_node();
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
    } else {
        cout << "  (Sound preferences section not found)" << endl;
    }

    // --- Graphics Settings ---
    auto graphicsPreferences = root.child("graphicsPreferences");
    cout << "\nGraphics Settings:" << endl;
    if (graphicsPreferences) {
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
    } else {
        cout << "  (Graphics preferences section not found)" << endl;
    }

    // --- Control Settings ---
    auto controlMode = scriptsPreferences ? scriptsPreferences.child("controlMode") : pugi::xml_node();
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
     } else {
        cout << "  (Control mode section not found)" << endl;
     }

    // --- Device Settings ---
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
    } else {
        cout << "  (Device preferences section not found)" << endl;
    }
     cout << "--- End of settings for: " << gameConfigPath.filename().string() << " ---" << endl;
}


// --- Існуючі методи ConfigManager ---

void ConfigManager::changeCurrentConfig() {
    // ... (код залишається таким, як був у попередній відповіді, з fs::remove)
    cout << "Changing current game config..." << endl;
    const char* appDataPath = getenv("APPDATA");
    if (!appDataPath) {
        cerr << "Error: Unable to get APPDATA environment variable." << endl;
        return;
    }
    fs::path targetDir = fs::path(appDataPath) / "Wargaming.net" / "WorldOfTanks";
    fs::path targetFile = targetDir / "preferences.xml";
    fs::path sourceDir = "User Configs";
    if (!fs::exists(sourceDir) || !fs::is_directory(sourceDir)) {
        cerr << "Error: Directory '" << sourceDir.string() << "' not found or is not a directory." << endl;
        return;
    }
    vector<fs::path> configFiles;
    cout << "\nAvailable config files in '" << sourceDir.string() << "':" << endl;
    try {
        for (const auto& entry : fs::directory_iterator(sourceDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".xml") {
                configFiles.push_back(entry.path());
            }
        }
    } catch (const fs::filesystem_error& e) {
        cerr << "Error reading directory '" << sourceDir.string() << "': " << e.what() << endl;
        return;
    }
    if (configFiles.empty()) {
        cout << "No config files found in '" << sourceDir.string() << "'." << endl;
        return;
    }
    for (size_t i = 0; i < configFiles.size(); ++i) {
        cout << i + 1 << ". " << configFiles[i].filename().string() << endl;
    }
    cout << "0. Cancel" << endl;
    int choice = -1;
    cout << "Enter the number of the config file to apply (or 0 to cancel): ";
    while (!(cin >> choice) || choice < 0 || choice > static_cast<int>(configFiles.size())) {
        cout << "Invalid input. Please enter a number between 0 and " << configFiles.size() << ": ";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    if (choice == 0) {
        cout << "Operation cancelled." << endl;
        return;
    }
    fs::path selectedSourceFile = configFiles[choice - 1];
    cout << "Applying '" << selectedSourceFile.filename().string() << "' as current game config..." << endl;
    try {
        if (fs::exists(targetFile)) {
            cout << "Attempting to remove existing target file: " << targetFile.string() << endl;
            try {
                fs::remove(targetFile);
                cout << "Existing target file removed successfully." << endl;
            } catch (const fs::filesystem_error& remove_error) {
                cerr << "Error: Could not remove existing target file '" << targetFile.string() << "': " << remove_error.what() << endl;
                cerr << "----> Please ensure the file is not in use (e.g., close World of Tanks and its launcher) and you have necessary permissions." << endl;
                return;
            }
        }
        if (!fs::exists(targetDir)) {
             cout << "Target directory '" << targetDir.string() << "' does not exist. Attempting to create." << endl;
             fs::create_directories(targetDir);
        }
        fs::copy_file(selectedSourceFile, targetFile, fs::copy_options::overwrite_existing);
        cout << "Successfully applied '" << selectedSourceFile.filename().string() << "' as '" << targetFile.filename().string() << "'." << endl;
    } catch (const fs::filesystem_error& e) {
        cerr << "Error applying config file during copy: " << e.what() << endl;
        cerr << "Details: " << e.path1().string() << " -> " << e.path2().string() << endl;
        cerr << "----> Please ensure the file is not in use and you have necessary permissions." << endl;
    }
}

void ConfigManager::uploadConfig() {
    cout << "Upload config functionality is not yet implemented." << endl;
}