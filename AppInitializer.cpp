#include "AppInitializer.h"
#include <iostream>
#include <filesystem>

using namespace std;

void AppInitializer::loadInitialSettings() {
    cout << "Loading initial settings..." << endl;
}

void AppInitializer::checkFiles() {
    if (!filesystem::exists("Saved Configs")) {
        filesystem::create_directory("Saved Configs");
        cout << "Created 'Saved Configs' directory." << endl;
    }
    if (!filesystem::exists("User Configs")) {
        filesystem::create_directory("User Configs");
        cout << "Created 'User Configs' directory." << endl;
    }
}

void AppInitializer::initializeComponents() {
    cout << "Initializing components..." << endl;
}
