#include <iostream>
#include "main.h" // Переконайся, що цей файл включає всі потрібні заголовки класів

using namespace std;

// Додаємо новий пункт меню
void displayMenu() {
    cout << "1. Check Folders" << endl;
    cout << "2. Create Backup" << endl;
    cout << "3. Restore From Backup" << endl;
    cout << "4. Set UserName" << endl;
    cout << "5. Show current username" << endl;
    cout << "6. Show cfg settings" << endl;
    cout << "7. Edit cfg settings" << endl;
    cout << "8. Change current cfg" << endl; // <-- Додано
    cout << "9. Check current cfg" << endl; // <-- Додано
    cout << "0. Exit" << endl;
    cout << "Enter your choice: ";
}

int main() {
    // Створюємо екземпляри класів, як і було
    AppInitializer initializer;
    BackupManager backupManager;
    ChangeTracker changeTracker; // Не використовується, але залишаємо
    ConfigEditor configeditor;
    ConfigManager configManager; // <-- Потрібен екземпляр ConfigManager
    FileValidator fileValidator; // Не використовується, але залишаємо
    ProfileManager profilemanager;
    // MainWindow mainWindow; // Не потрібен для консолі

    int choice;

    do {
        displayMenu();
        // Просте читання вводу, як було
        cin >> choice;

        // Додаємо обробку case 8
        switch (choice) {
            case 1:
                initializer.checkFolders();
                break;
            case 2:
                backupManager.createBackup();
                break;
            case 3:
                backupManager.restoreFromBackup();
                break;
            case 4:
                profilemanager.setName();
                break;
            case 5:
                profilemanager.showName();
                break;
            case 6:
                configeditor.readCurrentSettings();
                break;
            case 7:
                configeditor.modifySettings();
                break;
            case 8: // <-- Додано обробку
                configManager.changeCurrentConfig(); // Викликаємо новий метод
                break;
            case 9:
                configManager.viewCurrentGameConfig(); // <-- Викликає новий метод ConfigManager
                break;
            case 0:
                cout << "Exiting..." << endl;
                break;
            default:
                cout << "Invalid choice. Please try again." << endl;
                break;
        }
        cout << endl; // Залишаємо порожній рядок після виконання дії
    } while (choice != 0);

    return 0;
}