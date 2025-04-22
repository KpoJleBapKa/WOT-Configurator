#include <iostream>
#include <limits>      // Рекомендується для cin.ignore
#include "main.h"      // Переконайся, що цей файл включає всі потрібні заголовки

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
    cout << "10. Validate Config File" << endl; // <-- Додано пункт меню
    cout << "0. Exit" << endl;
    cout << "Enter your choice: ";
}

int main() {
    // Створюємо екземпляри класів
    AppInitializer initializer;
    BackupManager backupManager;
    ConfigEditor configeditor;
    ConfigManager configManager;
    FileValidator fileValidator;
    ProfileManager profilemanager;
    ChangeTracker logger; // <-- Створюємо екземпляр логгера

    int choice;

    logger.logAction("Application::Start", true); // Логуємо запуск програми

    do {
        displayMenu();
        cin >> choice;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << endl;

        bool actionSuccess = true; // Прапорець успішності для логування
        string actionDetails = "";  // Додаткові деталі для логу
        string actionName = "";     // Назва дії для логу

        switch (choice) {
            case 1:
                actionName = "AppInitializer::checkFolders";
                try {
                    initializer.checkFolders();
                    // Припускаємо успіх, якщо не було винятків (краще б метод повертав bool)
                    actionSuccess = true;
                } catch(...) { // Ловимо будь-які можливі винятки
                    actionSuccess = false;
                    actionDetails = "Exception occurred";
                }
                logger.logAction(actionName, actionSuccess, actionDetails);
                break; // Не забуваємо break

            case 2:
                actionName = "BackupManager::createBackup";
                 try {
                     backupManager.createBackup();
                     // Успіх визначається тим, чи вивелось повідомлення про помилку всередині методу
                     // Це не найкращий спосіб, але працює для поточного коду
                     // Краще б метод повертав bool або кидав винятки при помилці копіювання
                      actionSuccess = true; // Припускаємо успіх, якщо дійшли сюди
                 } catch (...) {
                      actionSuccess = false;
                      actionDetails = "Exception occurred";
                 }
                logger.logAction(actionName, actionSuccess, actionDetails);
                break;

            case 3:
                actionName = "BackupManager::restoreFromBackup";
                 try {
                     backupManager.restoreFromBackup();
                     // Аналогічно case 2, успіх припускається
                      actionSuccess = true;
                 } catch (...) {
                      actionSuccess = false;
                      actionDetails = "Exception occurred";
                 }
                 logger.logAction(actionName, actionSuccess, actionDetails);
                break;

            case 4:
                actionName = "ProfileManager::setName";
                 try {
                      profilemanager.setName();
                      actionSuccess = true; // Припускаємо успіх
                 } catch (...) {
                      actionSuccess = false;
                      actionDetails = "Exception occurred";
                 }
                logger.logAction(actionName, actionSuccess, actionDetails);
                break;

            case 5:
                actionName = "ProfileManager::showName";
                 try {
                      profilemanager.showName();
                      actionSuccess = true;
                 } catch (...) {
                      actionSuccess = false;
                      actionDetails = "Exception occurred";
                 }
                logger.logAction(actionName, actionSuccess, actionDetails);
                break;

            case 6:
                actionName = "ConfigEditor::readCurrentSettings";
                 try {
                      configeditor.readCurrentSettings();
                      actionSuccess = true;
                 } catch (...) {
                      actionSuccess = false;
                      actionDetails = "Exception occurred";
                 }
                logger.logAction(actionName, actionSuccess, actionDetails);
                break;

            case 7:
                actionName = "ConfigEditor::modifySettings";
                 try {
                      configeditor.modifySettings();
                      actionSuccess = true;
                 } catch (...) {
                      actionSuccess = false;
                      actionDetails = "Exception occurred";
                 }
                logger.logAction(actionName, actionSuccess, actionDetails);
                break;

            case 8:
                actionName = "ConfigManager::changeCurrentConfig";
                 try {
                      configManager.changeCurrentConfig();
                      actionSuccess = true;
                 } catch (...) {
                      actionSuccess = false;
                      actionDetails = "Exception occurred";
                 }
                logger.logAction(actionName, actionSuccess, actionDetails);
                break;

            case 9:
                actionName = "ConfigManager::viewCurrentGameConfig";
                 try {
                      configManager.viewCurrentGameConfig();
                      actionSuccess = true;
                 } catch (...) {
                      actionSuccess = false;
                      actionDetails = "Exception occurred";
                 }
                logger.logAction(actionName, actionSuccess, actionDetails);
                break;

            case 10:
                actionName = "FileValidator::runValidationWizard";
                 try {
                      fileValidator.runValidationWizard();
                      actionSuccess = true;
                 } catch (...) {
                      actionSuccess = false;
                      actionDetails = "Exception occurred";
                 }
                logger.logAction(actionName, actionSuccess, actionDetails);
                break;

            case 0:
                cout << "Exiting..." << endl;
                logger.logAction("Application::Exit", true); // Логуємо вихід
                break;

            default:
                actionName = "Menu::InvalidChoice";
                actionDetails = "User entered: " + to_string(choice); // Додаємо деталі
                logger.logAction(actionName, false, actionDetails); // Логуємо невірний вибір як помилку
                cout << "Invalid choice. Please try again." << endl;
                break;
        }
        // Не виводимо роздільник після виходу
        if (choice != 0) {
             cout << "\n---------------------------------\n" << endl;
        }

    } while (choice != 0);

    return 0;
}