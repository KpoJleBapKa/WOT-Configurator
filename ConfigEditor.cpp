#include "main.h"
#include "pugixml/pugixml.hpp"
#include <iostream>
#include <filesystem>
#include <vector>
#include <unordered_set>
#include <unordered_map>

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
        // Display version settings
        cout << " " << "graphicsSettingsVersion: " << graphicsPreferences.child("graphicsSettingsVersion").text().as_string() << endl;
        cout << " " << "graphicsSettingsVersionMinor: " << graphicsPreferences.child("graphicsSettingsVersionMinor").text().as_string() << endl;
        cout << " " << "graphicsSettingsVersionMaintainance: " << graphicsPreferences.child("graphicsSettingsVersionMaintainance").text().as_string() << endl;
        cout << " " << "graphicsSettingsStatus: " << graphicsPreferences.child("graphicsSettingsStatus").text().as_string() << endl;

        // Display each entry
        for (auto& entry : graphicsPreferences.children("entry")) {
            string label = entry.child("label").text().as_string();
            string activeOption = entry.child("activeOption").text().as_string();
            cout <<  " " << label << ": " << activeOption << endl;
        }

        // Display additional parameters
        cout << " " << "ParticlSystemNoRenderGroup: " << graphicsPreferences.child("ParticlSystemNoRenderGroup").text().as_string() << endl;
        cout << " " << "distributionLevel: " << graphicsPreferences.child("distributionLevel").text().as_string() << endl;
        cout << " " << "colorGradingStrength: " << graphicsPreferences.child("colorGradingStrength").text().as_string() << endl;
        cout << " " << "brightnessDeferred: " << graphicsPreferences.child("brightnessDeferred").text().as_string() << endl;
        cout << " " << "contrastDeferred: " << graphicsPreferences.child("contrastDeferred").text().as_string() << endl;
        cout << " " << "saturationDeferred: " << graphicsPreferences.child("saturationDeferred").text().as_string() << endl;
    } else {
        cout << "Graphics preferences node not found." << endl;
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

bool validateValue(const string& setting, const string& value) {
    try {
        float val = stof(value);

        if (setting == "masterVolume" || setting == "volume_micVivox" || 
            setting == "volume_vehicles" || setting == "volume_music" || 
            setting == "volume_effects" || setting == "volume_ambient" || 
            setting == "volume_gui" || setting == "volume_voice" || 
            setting == "colorGradingStrength" || setting == "brightnessDeferred" || 
            setting == "contrastDeferred" || setting == "saturationDeferred") {
            return val >= 0.0f && val <= 1.0f;
        }

        if (setting.find("sensitivity") != string::npos) {
            return val >= 0.0f && val <= 10.0f;
        }

        if (setting == "windowMode") {
            return val >= 1 && val <= 2;
        }

        if (setting == "windowedWidth" || setting == "fullscreenWidth") {
            return val >= 800;
        }

        if (setting == "windowedHeight" || setting == "fullscreenHeight") {
            return val >= 600;
        }

        if (setting == "fullscreenRefresh") {
            return val >= 50;
        }

    } catch (...) {
        return false;
    }
    return true;
}

void ConfigEditor::modifySettings() {
    string configPath = "User Configs";
    vector<string> configFiles;

    for (const auto& entry : fs::directory_iterator(configPath)) {
        if (entry.path().extension() == ".xml") {
            configFiles.push_back(entry.path().string());
        }
    }

    if (configFiles.empty()) {
        cout << "No configuration files available to modify." << endl;
        return;
    }

    cout << "Available configuration files:" << endl;
    for (size_t i = 0; i < configFiles.size(); ++i) {
        cout << i + 1 << ". " << configFiles[i] << endl;
    }
    cout << "Press 0 to exit." << endl;

    size_t choice;
    cout << "Enter the number of the file you want to modify: ";
    cin >> choice;

    if (choice == 0) {
        cout << "Exiting configuration editor." << endl;
        return;
    }

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

    unordered_map<string, pugi::xml_node> categories;
    auto root = doc.child("root");
    if (!root) {
        cout << "Root node <root> not found." << endl;
        return;
    }

    categories["Sound Settings"] = root.child("scriptsPreferences").child("soundPrefs");
    categories["Graphics Settings"] = root.child("graphicsPreferences");
    categories["Control Settings"] = root.child("scriptsPreferences").child("controlMode");
    categories["Device Settings"] = root.child("devicePreferences");

    unordered_set<string> hiddenSettings = {
        "soundMode", "enable", "volume_music_hangar", "volume_voice", "volume_ev_ambient",
        "volume_ev_effects", "volume_ev_gui", "volume_ev_music", "volume_ev_vehicles",
        "volume_ev_voice", "detectedSpeakerConfig",

        "graphicsSettingsVersion", "graphicsSettingsVersionMinor", "graphicsSettingsVersionMaintainance",
        "graphicsSettingsStatus", "COLOR_GRADING_TECHNIQUE", "CUSTOM_AA_MODE", "DECOR_LEVEL",
        "DRR_AUTOSCALER_ENABLED", "EFFECTS_QUALITY", "FAR_PLANE", "FLORA_QUALITY", "HAVOK_ENABLED",
        "HAVOK_QUALITY", "LIGHTING_QUALITY", "MOTION_BLUR_QUALITY", "MSAA_QUALITY", "OBJECT_LOD",
        "POST_PROCESSING_QUALITY", "RENDER_PIPELINE", "SEMITRANSPARENT_LEAVES_ENABLED", "SHADER_DEBUG",
        "SHADOWS_QUALITY", "SNIPER_MODE_EFFECTS_QUALITY", "SNIPER_MODE_GRASS_ENABLED",
        "SNIPER_MODE_SWINGING_ENABLED", "SNIPER_MODE_TERRAIN_TESSELLATION_ENABLED", "SPEEDTREE_QUALITY",
        "TERRAIN_QUALITY", "TERRAIN_TESSELLATION_ENABLED", "TEXTURE_QUALITY", "TRACK_PHYSICS_QUALITY",
        "VEHICLE_DUST_ENABLED", "VEHICLE_TRACES_ENABLED", "WATER_QUALITY", "ParticlSystemNoRenderGroup",
        "distributionLevel", "detector", "deviceGUID", "deviceGUIDGraphicsAPI", "deviceGUIDAdapterOutput",
        "deviceChanges", "currentPreset", "entry",

        "horzInvert", "vertInvert", "keySensitivity", "scrollSensitivity", "aspectRatio",

        "positionX", "positionY", "isWindowMaximized", "borderlessBehaviour", "borderlessPositionX",
        "borderlessPositionY", "borderlessWidth", "borderlessHeight", "borderlessMonitorIndex",
        "toggleBorderless", "fullscreenMonitorIndex", "waitVSync", "tripleBuffering",
        "aspectRatio_override", "gammaDeferred", "gammaForward", "drrScale", "useAlternateFrameTimer",
        "textureQualityMemoryBlock"
    };

    cout << "Available categories:" << endl;
    size_t index = 1;
    vector<string> categoryNames;
    for (const auto& category : categories) {
        cout << index++ << ". " << category.first << endl;
        categoryNames.push_back(category.first);
    }
    cout << "Press 0 to exit." << endl;

    size_t categoryChoice;
    cout << "Enter the number of the category you want to modify: ";
    cin >> categoryChoice;

    if (categoryChoice == 0) {
        cout << "Exiting configuration editor." << endl;
        return;
    }

    if (categoryChoice < 1 || categoryChoice > categoryNames.size()) {
        cout << "Invalid choice. Please try again." << endl;
        return;
    }

    string selectedCategory = categoryNames[categoryChoice - 1];
    auto selectedNode = categories[selectedCategory];

    vector<pair<string, pugi::xml_node>> settingsList;
    cout << selectedCategory << ":" << endl;

    if (selectedCategory == "Control Settings") {
        for (auto& mode : selectedNode.children()) {
            string modeName = mode.name();
            auto cameraNode = mode.child("camera");
            if (cameraNode) {
                auto sensitivityNode = cameraNode.child("sensitivity");
                if (sensitivityNode && hiddenSettings.find("sensitivity") == hiddenSettings.end()) {
                    string displayName = modeName + " sensitivity";
                    cout << "  " << displayName << ": " << sensitivityNode.text().as_string() << endl;
                    settingsList.emplace_back(displayName, sensitivityNode);
                }
            }
        }
    } else {
        for (auto& setting : selectedNode.children()) {
            string name = setting.name();
            if (hiddenSettings.find(name) == hiddenSettings.end()) {
                cout << "  " << name << ": " << setting.text().as_string() << endl;
                settingsList.emplace_back(name, setting);
            }
        }
    }
    cout << "Press 0 to exit." << endl;

    size_t settingChoice;
    cout << "Enter the number of the setting you want to modify: ";
    cin >> settingChoice;

    if (settingChoice == 0) {
        cout << "Exiting configuration editor." << endl;
        return;
    }

    if (settingChoice < 1 || settingChoice > settingsList.size()) {
        cout << "Invalid choice. Please try again." << endl;
        return;
    }

    auto [selectedSetting, selectedNodeSetting] = settingsList[settingChoice - 1];
    cout << "Enter new value for " << selectedSetting << ": ";
    string newValue;
    cin >> newValue;

    if (!validateValue(selectedSetting, newValue)) {
        cout << "Invalid value for " << selectedSetting << ". Please ensure it is within the allowed range." << endl;
        return;
    }

    selectedNodeSetting.text().set(newValue.c_str());

    if (doc.save_file(selectedFile.c_str())) {
        cout << "Settings updated successfully." << endl;
    } else {
        cout << "Failed to save changes." << endl;
    }
}
