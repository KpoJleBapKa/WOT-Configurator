#include <iostream>
#include "main.h"

using namespace std;


void displayMenu() {
    cout << "1. Check Folders" << endl;
    cout << "2. Create Backup" << endl;
    cout << "3. Restore From Backup" << endl;
    cout << "4. Set UserName" << endl;
    cout << "5. Show current username" << endl;
    cout << "6. Show cfg settings" << endl;
    cout << "7. Edit cfg settings" << endl;
    cout << "0. Exit" << endl;
    cout << "Enter your choice: ";
}

int main() {
    AppInitializer initializer;
    BackupManager backupManager;
    ChangeTracker changeTracker;
    ConfigEditor configeditor;
    FileValidator fileValidator;
    ProfileManager profilemanager;

    int choice;

    do {
        displayMenu();
        cin >> choice;

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
            case 0:
                cout << "Exiting..." << endl;
                break;
            default:
                cout << "Invalid choice. Please try again." << endl;
                break;
        }
        cout << endl;
    } while (choice != 0);

    return 0;
}
