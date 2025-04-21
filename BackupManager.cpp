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
#include <cstdlib>     // Для getenv
#include <limits>      // Для cin.ignore
#include <cctype>      // Для tolower

// Включаємо FileValidator, якщо main.h його не включає напряму
// #include "FileValidator.h"

using namespace std;
namespace fs = std::filesystem;

void BackupManager::createBackup() {
    cout << "Creating backup..." << endl;
    FileValidator fileValidator; // Валідатор

    // --- Шлях до файлу гри (використовуємо APPDATA) ---
    const char* appDataPath = getenv("APPDATA");
    if (!appDataPath) {
        cerr << "Error: Unable to get APPDATA path." << endl;
        return;
    }
    fs::path sourcePath = fs::path(appDataPath) / "Wargaming.net/WorldOfTanks/preferences.xml";

    if (!fs::exists(sourcePath)) {
        cerr << "Error: preferences.xml not found at " << sourcePath.string() << endl;
        return;
    }

    // --- ВАЛІДАЦІЯ ДЖЕРЕЛА ---
    cout << "Validating source file before backup..." << endl;
    if (!fileValidator.isXmlWellFormed(sourcePath)) {
        cerr << "Error: Source file is not a well-formed XML. Backup aborted." << endl;
        return;
    }
    if (!fileValidator.hasExpectedStructure(sourcePath)) {
        cerr << "Warning: Source file might be missing expected WoT structure sections. Continuing backup..." << endl;
    }
    vector<string> valueErrors = fileValidator.findInvalidSimpleValues(sourcePath);
    if (!valueErrors.empty()) {
        cerr << "Warning: Found potential issues in source file values (backup will still be created):" << endl;
        for(const auto& err : valueErrors) cerr << "  - " << err << endl;
    }
    cout << "Source file validation completed." << endl;
    // --- КІНЕЦЬ ВАЛІДАЦІЇ ---

    // --- Логіка створення імені бекапу (як було, але з безпечним localtime та часом) ---
    auto now = chrono::system_clock::now();
    auto in_time_t = chrono::system_clock::to_time_t(now);
    tm local_tm;
    #ifdef _WIN32
        localtime_s(&local_tm, &in_time_t);
    #else
        localtime_r(&in_time_t, &local_tm);
    #endif
    stringstream ss;
    ss << "preferences_" << put_time(&local_tm, "%Y_%m_%d_%H%M%S") << ".xml"; // Додав час

    fs::path backupDir = "Restored Configs";
    fs::path backupPath = backupDir / ss.str();

    // --- Копіювання файлу (як було) ---
    try {
        fs::create_directories(backupDir);
        fs::copy_file(sourcePath, backupPath, fs::copy_options::overwrite_existing);
        cout << "Backup created at " << backupPath.string() << endl;
    } catch (fs::filesystem_error& e) {
        cerr << "File system error during backup: " << e.what() << endl;
    }
}


void BackupManager::restoreFromBackup() {
    cout << "Restore from backup..." << endl; // Додав повідомлення
    FileValidator fileValidator; // Валідатор

    fs::path backupDir = "Restored Configs";
    const char* appDataPath = getenv("APPDATA");
    if (!appDataPath) {
        cerr << "Error: Unable to get APPDATA path." << endl;
        return;
    }
    fs::path targetDir = fs::path(appDataPath) / "Wargaming.net/WorldOfTanks"; // Потрібна директорія
    fs::path targetFile = targetDir / "preferences.xml";

    vector<fs::path> backupFilesPaths; // Зберігаємо повні шляхи
    vector<string> backupFilesNames;  // Зберігаємо імена для виводу

    if (!fs::exists(backupDir) || !fs::is_directory(backupDir)) {
         cerr << "Error: Backup directory '" << backupDir.string() << "' not found." << endl;
         return;
    }

    try {
        for (const auto& entry : fs::directory_iterator(backupDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".xml") {
                backupFilesPaths.push_back(entry.path());
                backupFilesNames.push_back(entry.path().filename().string());
            }
        }
    } catch(const fs::filesystem_error& e) {
         cerr << "Error reading backup directory: " << e.what() << endl;
         return;
    }


    if (backupFilesNames.empty()) {
        cout << "No backup files found in " << backupDir.string() << endl;
        return;
    }

    cout << "Select a backup file to restore:" << endl;
    for (size_t i = 0; i < backupFilesNames.size(); ++i) {
        cout << i + 1 << ": " << backupFilesNames[i] << endl;
    }
     cout << "0. Cancel" << endl; // Додав опцію скасування

    int choice;
    cout << "Enter the number of the file you want to restore: ";
    // Додав кращу обробку вводу
     while (!(cin >> choice) || choice < 0 || choice > static_cast<int>(backupFilesNames.size())) {
         cout << "Invalid input. Please enter a number between 0 and " << backupFilesNames.size() << ": ";
         cin.clear();
         cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');


    if (choice == 0) { // Обробка скасування
        cout << "Restore cancelled." << endl;
        return;
    }

    // Використовуємо збережений повний шлях
    fs::path selectedFile = backupFilesPaths[choice - 1];

    // --- ВАЛІДАЦІЯ ВИБРАНОГО ФАЙЛУ ---
    cout << "Validating selected backup file before restoring..." << endl;
    if (!fileValidator.isXmlWellFormed(selectedFile)) {
        cerr << "Error: Selected backup file is not a well-formed XML. Restore aborted." << endl;
        return;
    }
    if (!fileValidator.hasExpectedStructure(selectedFile)) {
        cerr << "Error: Selected backup file is missing expected WoT structure. Restore aborted." << endl;
        return;
    }
    vector<string> valueErrors = fileValidator.findInvalidSimpleValues(selectedFile);
    if (!valueErrors.empty()) {
        cout << "\n--- Validation Warnings for selected file ---" << endl;
        for(const auto& err : valueErrors) cout << "  - " << err << endl;
        cout << "-------------------------------------------" << endl;
        cout << "Do you want to continue restoring this file despite the warnings? (y/n): ";
        char confirm = 'n';
        cin >> confirm;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        if (tolower(confirm) != 'y') {
            cout << "Restore cancelled by user." << endl;
            return;
        }
         cout << "Continuing restore despite warnings..." << endl;
    } else {
        cout << "Selected backup file validation passed." << endl;
    }
    // --- КІНЕЦЬ ВАЛІДАЦІЇ ---

    // --- Відновлення (як було, з видаленням) ---
    try {
        if (!fs::exists(targetDir)) {
             fs::create_directories(targetDir);
        }
        if (fs::exists(targetFile)) {
            fs::remove(targetFile);
        }
        fs::copy_file(selectedFile, targetFile, fs::copy_options::overwrite_existing);
        cout << "Backup restored successfully from " << selectedFile.filename().string() << endl; // Додав ім'я файлу
    } catch (const fs::filesystem_error& e) {
        cout << "Error restoring backup: " << e.what() << endl;
         cerr << "Path1: " << e.path1().string() << endl;
         cerr << "Path2: " << e.path2().string() << endl;
    }
}