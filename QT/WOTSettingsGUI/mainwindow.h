#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "configeditdialog.h"

// Попереднє оголошення для уникнення включення повних заголовків тут
QT_BEGIN_NAMESPACE
class QMessageBox;
class QFileDialog;
class QInputDialog;
class QDateTime;
class QTextStream;
class QDialog;
class QVBoxLayout;
class QPlainTextEdit;
class QPushButton;
class QFontDatabase;
class QTreeWidget;      // <-- Оголошення для QTreeWidget
class QTreeWidgetItem;  // <-- Оголошення для QTreeWidgetItem
class QHeaderView;      // <-- Оголошення для QHeaderView
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// Включаємо ОДИН головний заголовок для всієї логіки
#include "main.h" // Переконайся, що файл називається саме так

// Перемістимо оголошення fs сюди, бо воно використовується в оголошенні displaySettingsInTreeDialog
#include <filesystem>
namespace fs = std::filesystem;

class MainWindow : public QMainWindow
{
    Q_OBJECT // Макрос для сигналів/слотів

public:
    MainWindow(QWidget *parent = nullptr); // Конструктор
    ~MainWindow();                     // Деструктор

private slots: // Слоти для обробки сигналів від UI елементів
    // Слоти для кнопок меню (без змін)
    void onCheckFoldersClicked();
    void onCreateBackupClicked();
    void onRestoreBackupClicked();
    void onSetUsernameClicked();
    void onShowUsernameClicked();
    void onShowConfigClicked(); // Показує конфіг користувача (відфільтровано)
    void onEditConfigClicked();
    void onChangeConfigClicked(); // Застосовує конфіг користувача
    void onCheckCurrentConfigClicked(); // Показує конфіг гри (відфільтровано)
    void onValidateConfigClicked();
    void onExitClicked();

private: // Приватні члени та методи класу
    Ui::MainWindow *ui; // Вказівник на UI, створений дизайнером

    // Екземпляри класів логіки (без змін)
    AppInitializer m_initializer;
    BackupManager m_backupManager;
    ChangeTracker m_logger;
    ConfigEditor m_configEditor;
    ConfigManager m_configManager;
    FileValidator m_fileValidator;
    ProfileManager m_profileManager;

    // Допоміжні функції UI (без змін)
    void showMessage(const QString& title, const QString& text, bool isWarning = false);
    void appendLog(const QString& message);
    void setupConnections(); // Функція для налаштування connect()
    void loadUsername(); // Завантажити ім'я при старті
    void displayFileContent(const QString& filePath, const QString& logActionName); // Показати повний вміст файлу у новому вікні

    // Допоміжні функції для вибору файлів (без змін)
    QString selectBackupFile();
    QString selectUserConfigFile();
    QString selectFileToValidate();

    // Допоміжні функції для відображення результатів валідації (без змін)
    void displayValidationResult(const ValidationResult& result, const QString& filename);
    QString formatValidationSummary(const ValidationResult& result); // Формує рядок з резюме для показу

    // <-- ОГОЛОШЕННЯ НОВОЇ ФУНКЦІЇ
    // Оголошуємо функцію, яка показує відфільтровані налаштування
    void displaySettingsInTreeDialog(const fs::path& configPath, const std::string& windowTitlePrefix);

};
#endif // MAINWINDOW_H
