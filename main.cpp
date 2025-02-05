#include <iostream>

#include "AppInitializer.h"
#include "ConfigManager.h"
#include "ConfigEditor.h"

using namespace std;

int main() {
    AppInitializer initializer;
    initializer.loadInitialSettings();
    initializer.checkFiles();
    initializer.initializeComponents();

    ConfigManager configManager;
    configManager.coordinate();
    configManager.readSettings();
    configManager.writeSettings();

    ConfigEditor configEditor;
    configEditor.readCurrentSettings("Saved Configs/preferences.xml");
    configEditor.makeChanges("Saved Configs/preferences.xml");
    configEditor.saveUpdatedSettings("Saved Configs/preferences.xml");

    std::cout << "Press Enter to exit...";
    std::cin.get();
    return 0;
}
