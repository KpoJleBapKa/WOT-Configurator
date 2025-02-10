#include "main.h"
#include "pugixml/pugixml.hpp"  
#include <iostream>

using namespace std;

void ConfigEditor::readCurrentSettings(const string& filePath) {
    cout << "Reading current settings from: " << filePath << endl;
}

void ConfigEditor::makeChanges(const string& filePath) {
    pugi::xml_document doc;
    if (!doc.load_file(filePath.c_str())) {
        cerr << "Failed to load XML file." << endl;
        return;
    }

    auto resolution = doc.child("preferences").child("graphics").child("resolution");
    resolution.attribute("width").set_value("1280");
    resolution.attribute("height").set_value("720");

    doc.save_file(filePath.c_str());
    cout << "Changes made to configuration." << endl;
}

void ConfigEditor::saveUpdatedSettings(const string& filePath) {
    cout << "Saving updated settings to: " << filePath << endl;
}
