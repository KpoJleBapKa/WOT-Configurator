#ifndef APPINITIALIZER_H
#define APPINITIALIZER_H

class AppInitializer {
public:
    void loadInitialSettings();
    void checkFolders();
    void initializeComponents();
};

#endif // APPINITIALIZER_H

#ifndef BACKUPMANAGER_H
#define BACKUPMANAGER_H

class BackupManager {
public:
    void createBackup();
    void restoreFromBackup();
    void manageBackupSpace();
};

#endif // BACKUPMANAGER_H

#ifndef CHANGETRACKER_H
#define CHANGETRACKER_H

#include <string> // Потрібен для std::string

class ChangeTracker {
public:
    // Метод для запису дії в лог
    // functionName - назва функції/операції, що логується
    // success - чи була операція успішною (true/false)
    // details - необов'язковий рядок з додатковою інформацією (напр., ім'я файлу, помилка)
    void logAction(const std::string& functionName, bool success, const std::string& details = "");

    // Старі методи logChanges, rollbackChanges, displayChangeHistory видаляємо або коментуємо,
    // оскільки вони не відповідають поточній задачі простого логування.
    // void logChanges();
    // void rollbackChanges();
    // void displayChangeHistory();
};

#endif // CHANGETRACKER_H

#ifndef CONFIGEDITOR_H
#define CONFIGEDITOR_H

#include <string>

using namespace std;

class ConfigEditor {
public:
    void readCurrentSettings();
    void modifySettings();
};

#endif // CONFIGEDITOR_H

#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <string>

class ConfigManager {
public:
    void uploadConfig();
    void changeCurrentConfig();
    void viewCurrentGameConfig(); // <-- Новий метод для перегляду конфігу гри
};

#endif // CONFIGMANAGER_H

#ifndef FILEVALIDATOR_H
#define FILEVALIDATOR_H

#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

class FileValidator {
public:
    void runValidationWizard();
    bool isXmlWellFormed(const fs::path& filePath);
    bool hasExpectedStructure(const fs::path& filePath);
    std::vector<std::string> findInvalidSimpleValues(const fs::path& filePath);
};

#endif // FILEVALIDATOR_H

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

class MainWindow {
public:
    void displaySettings();
    void editAndSaveChanges();
    void integrateComponents();
};

#endif // MAINWINDOW_H

#ifndef PROFILEMANAGER_H
#define PROFILEMANAGER_H

#include <string>

class ProfileManager {
public:
    void setName();
    void changeName();
    void showName();
private:
    std::string userName;
    void saveNameToFile(const std::string& name);
    std::string loadNameFromFile();
};

#endif // PROFILEMANAGER_H

#ifndef MAIN_H
#define MAIN_H

void displayMenu();

#endif // MAIN_H
