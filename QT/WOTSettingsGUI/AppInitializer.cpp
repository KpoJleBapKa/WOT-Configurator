#include "main.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include "preferences_data.h"

// extern unsigned char _binary_preferences_xml_start;
// extern unsigned char _binary_preferences_xml_end;

using namespace std;

void AppInitializer::loadInitialSettings() {
    cout << "Loading initial settings..." << endl;
}

void AppInitializer::checkFolders() {
    if (!filesystem::exists("User Data")) {
        filesystem::create_directory("User Data");
        cout << "Created 'User Data' directory." << endl;
    }
    if (!filesystem::exists("Saved Configs")) {
        filesystem::create_directory("Saved Configs");
        cout << "Created 'Saved Configs' directory." << endl;
    }
    if (!filesystem::exists("Restored Configs")) {
        filesystem::create_directory("Restored Configs");
        cout << "Created 'Restored Configs' directory." << endl;
    }
    if (!filesystem::exists("User Configs")) {
        filesystem::create_directory("User Configs");
        cout << "Created 'User Configs' directory." << endl;
    }
    // if (!filesystem::exists("Backup")) {
    //     filesystem::create_directory("Backup");
    //     cout << "Created 'Backup' directory." << endl;
    // }
    if (!filesystem::exists("Reference Config")) {
        filesystem::create_directory("Reference Config");
        cout << "Created 'Reference Config' directory." << endl;
        ofstream outFile("Reference Config/preferences.xml", ios::binary);
        if (outFile.is_open()) {
            // ЗМІНЕНО: використовуємо масив і довжину з хедера
            outFile.write(reinterpret_cast<const char*>(preferences_xml), preferences_xml_len);
            outFile.close();
            cout << "Extracted 'preferences.xml' to 'Reference Config'." << endl;
        } else { /* ... обробка помилки ... */ }
    }
}

void AppInitializer::initializeComponents() {
    cout << "Initializing components..." << endl;
}
