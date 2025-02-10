#include <iostream>
#include "main.h"

using namespace std;

void displayMenu() {
    cout << "1. Load Initial Settings" << endl;
    cout << "2. Check Folders" << endl;
    cout << "3. Initialize Components" << endl;
    cout << "4. Create Backup" << endl;
    cout << "5. Restore From Backup" << endl;
    cout << "6. Manage Backup Space" << endl;
    cout << "7. Log Changes" << endl;
    cout << "8. Rollback Changes" << endl;
    cout << "9. Display Change History" << endl;
    cout << "10. Validate File Syntax" << endl;
    cout << "11. Detect Errors" << endl;
    cout << "12. Inform User" << endl;
    cout << "0. Exit" << endl;
    cout << "Enter your choice: ";
}

int main() {
    AppInitializer initializer;
    BackupManager backupManager;
    ChangeTracker changeTracker;
    ConfigEditor configEditor;
    FileValidator fileValidator;

    int choice;

    do {
        displayMenu();
        cin >> choice;

        switch (choice) {
            case 1:
                initializer.loadInitialSettings();
                break;
            case 2:
                initializer.checkFolders();
                break;
            case 3:
                initializer.initializeComponents();
                break;
            case 4:
                backupManager.createBackup();
                break;
            case 5:
                backupManager.restoreFromBackup();
                break;
            case 6:
                backupManager.manageBackupSpace();
                break;
            case 7:
                changeTracker.logChanges();
                break;
            case 8:
                changeTracker.rollbackChanges();
                break;
            case 9:
                changeTracker.displayChangeHistory();
                break;
            case 10:
                fileValidator.validateSyntax("Saved Configs/preferences.xml");
                break;
            case 11:
                fileValidator.detectErrors();
                break;
            case 12:
                fileValidator.informUser();
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
