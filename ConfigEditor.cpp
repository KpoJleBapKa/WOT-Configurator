#include "main.h"
#include "pugixml/pugixml.hpp"
#include <iostream>
#include <filesystem>
#include <vector>
#include <unordered_set>

using namespace std;
namespace fs = std::filesystem;

void ConfigEditor::readCurrentSettings() {
    string configPath = "User Configs";
    vector<string> configFiles;

    // Check for configuration files in the directory
    for (const auto& entry : fs::directory_iterator(configPath)) {
        if (entry.path().extension() == ".xml") {
            configFiles.push_back(entry.path().string());
        }
    }

    if (configFiles.empty()) {
        cout << "No configuration files available to view." << endl;
        return;
    }

    // Display the list of configuration files for selection
    cout << "Available configuration files:" << endl;
    for (size_t i = 0; i < configFiles.size(); ++i) {
        cout << i + 1 << ". " << configFiles[i] << endl;
    }

    cout << "Enter the number of the file you want to view: ";
    size_t choice;
    cin >> choice;

    if (choice < 1 || choice > configFiles.size()) {
        cout << "Invalid choice. Please try again." << endl;
        return;
    }

    string selectedFile = configFiles[choice - 1];
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(selectedFile.c_str());

    if (!result) {
        cout << "Failed to load file: " << result.description() << endl;
        return;
    }

    // Define required fields
    unordered_set<string> soundSettings = {"masterVolume", "volume_micVivox", "volume_vehicles", 
                                           "volume_music", "volume_effects", "volume_ambient", 
                                           "volume_gui", "volume_voice", "soundMode"};

    unordered_set<string> graphicsSettings = {"COLOR_GRADING_TECHNIQUE", "CUSTOM_AA_MODE", 
                                              "DECOR_LEVEL", "EFFECTS_QUALITY", "FLORA_QUALITY", 
                                              "LIGHTING_QUALITY", "MOTION_BLUR_QUALITY", 
                                              "MSAA_QUALITY", "OBJECT_LOD", 
                                              "POST_PROCESSING_QUALITY", "SHADOWS_QUALITY", 
                                              "WATER_QUALITY", "TERRAIN_QUALITY", 
                                              "TEXTURE_QUALITY", "TRACK_PHYSICS_QUALITY"};

    unordered_set<string> controlSettings = {"horzInvert", "vertInvert", "keySensitivity", 
                                             "sensitivity", "scrollSensitivity"};

    unordered_set<string> deviceSettings = {"windowMode", "windowedWidth", "windowedHeight", 
                                            "fullscreenWidth", "fullscreenHeight", 
                                            "fullscreenRefresh", "aspectRatio"};

    // Output settings
    auto root = doc.child("root");
    if (!root) {
        cout << "Root node <root> not found." << endl;
        return;
    }

    auto scriptsPreferences = root.child("scriptsPreferences");
    auto soundPrefs = scriptsPreferences.child("soundPrefs");

    cout << "Sound Settings:" << endl;
    for (auto& setting : soundPrefs.children()) {
        string name = setting.name();
        if (soundSettings.count(name)) {
            cout << "  " << name << ": " << setting.text().as_string() << endl;
        }
    }

    auto graphicsPreferences = root.child("graphicsPreferences");
    cout << "\nGraphics Settings:" << endl;
    if (graphicsPreferences) {
        for (auto& entry : graphicsPreferences.children("entry")) {
            string label = entry.child("label").text().as_string();
            if (graphicsSettings.count(label)) {
                cout << "  " << label << ": " << entry.child("activeOption").text().as_string() << endl;
            }
        }
    } else {
        cout << "Graphics node not found." << endl;
    }

    auto controlMode = scriptsPreferences.child("controlMode");
    cout << "\nControl Settings:" << endl;
    for (auto& mode : controlMode.children()) {
        for (auto& camera : mode.child("camera").children()) {
            string name = camera.name();
            if (controlSettings.count(name)) {
                cout << "  " << name << ": " << camera.text().as_string() << endl;
            }
        }
    }

    auto devicePreferences = root.child("devicePreferences");
    cout << "\nDevice Settings:" << endl;
    for (auto& setting : devicePreferences.children()) {
        string name = setting.name();
        if (deviceSettings.count(name)) {
            cout << "  " << name << ": " << setting.text().as_string() << endl;
        }
    }
}
