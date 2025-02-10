#include "main.h"
#include <iostream>
#include <fstream>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

void ProfileManager::setName() {
    cout << "Please enter your real World of Tanks nickname: ";
    cin >> userName;
    saveNameToFile(userName);
    cout << "Name saved successfully." << endl;
}

void ProfileManager::saveNameToFile(const std::string& name) {
    fs::create_directories("User Data");
    ofstream outFile("User Data/userdata.txt");
    if (outFile.is_open()) {
        outFile << name;
        outFile.close();
    } else {
        cout << "Error saving name to file." << endl;
    }
}

std::string ProfileManager::loadNameFromFile() {
    ifstream inFile("User Data/userdata.txt");
    if (inFile.is_open()) {
        getline(inFile, userName);
        inFile.close();
    } else {
        cout << "No saved name found." << endl;
    }
    return userName;
}
