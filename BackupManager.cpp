#include "main.h"
#include <iostream>
#include <filesystem>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>


using namespace std;
namespace fs = std::filesystem;

void BackupManager::createBackup() {
    cout << "Creating backup..." << endl;

    // Отримання імені користувача
    const char* userProfile = getenv("USERPROFILE");
    if (!userProfile) {
        cerr << "Error: Unable to get user profile path." << endl;
        return;
    }

    // Формування шляху до файлу налаштувань
    fs::path sourcePath = fs::path(userProfile) / "AppData/Roaming/Wargaming.net/WorldOfTanks/preferences.xml";

    if (!fs::exists(sourcePath)) {
        cerr << "Error: preferences.xml not found at " << sourcePath << endl;
        return;
    }

    // Отримання поточної дати
    auto now = chrono::system_clock::now();
    auto in_time_t = chrono::system_clock::to_time_t(now);
    tm local_tm;
    localtime_s(&local_tm, &in_time_t);

    // Формування назви файлу бекапу
    stringstream ss;
    ss << "preferences_"
       << put_time(&local_tm, "%d_%m_%Y") << ".xml";

    // Формування шляху до файлу бекапу
    fs::path backupDir = "Restored Configs";
    fs::path backupPath = backupDir / ss.str();

    // Копіювання файлу
    try {
        fs::create_directories(backupDir); // Переконайтеся, що директорія існує
        fs::copy_file(sourcePath, backupPath, fs::copy_options::overwrite_existing);
        cout << "Backup created at " << backupPath << endl;
    } catch (fs::filesystem_error& e) {
        cerr << "File system error: " << e.what() << endl;
    }
}


void BackupManager::restoreFromBackup() {
    fs::path backupDir = "Restored Configs";
    fs::path targetFile = fs::path(std::getenv("APPDATA")) / "Wargaming.net/WorldOfTanks/preferences.xml";

    vector<string> backupFiles;

    for (const auto& entry : fs::directory_iterator(backupDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".xml") {
            backupFiles.push_back(entry.path().filename().string());
        }
    }

    if (backupFiles.empty()) {
        cout << "No backup files found in " << backupDir << endl;
        return;
    }

    cout << "Select a backup file to restore:" << endl;
    for (size_t i = 0; i < backupFiles.size(); ++i) {
        cout << i + 1 << ": " << backupFiles[i] << endl;
    }

    int choice;
    cout << "Enter the number of the file you want to restore: ";
    cin >> choice;

    if (choice < 1 || choice > static_cast<int>(backupFiles.size())) {
        cout << "Invalid choice." << endl;
        return;
    }

    fs::path selectedFile = backupDir / backupFiles[choice - 1];

    try {
        // Видаляємо цільовий файл, якщо він існує
        if (fs::exists(targetFile)) {
            fs::remove(targetFile);
        }
        
        fs::copy_file(selectedFile, targetFile, fs::copy_options::overwrite_existing);
        cout << "Backup restored successfully." << endl;
    } catch (const fs::filesystem_error& e) {
        cout << "Error restoring backup: " << e.what() << endl;
    }
}
