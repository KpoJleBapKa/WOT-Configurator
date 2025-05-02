#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "configeditdialog.h"
// НЕ включаємо helpdialog.h тут, щоб уникнути потенційних циклічних залежностей,
// включимо його в mainwindow.cpp

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
class QTreeWidget;
class QTreeWidgetItem;
class QHeaderView;
class QStackedWidget;
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#include "main.h"
#include <filesystem>
namespace fs = std::filesystem;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Старі слоти
    void onCheckFoldersClicked();
    void onCreateBackupClicked();
    void onRestoreBackupClicked();
    void onSetUsernameClicked();
    void onShowUsernameClicked();
    void onShowConfigClicked();
    void onEditConfigClicked();
    void onChangeConfigClicked();
    void onCheckCurrentConfigClicked();
    void onValidateConfigClicked();
    void onExitClicked();

    // --- НОВИЙ СЛОТ ---
    void onHelpButtonClicked(); // Слот для кнопки "Довідка"
    void onStatsButtonClicked();
    void onAIChatButtonClicked();   // Слот для кнопки "AI Помічник"

private:
    Ui::MainWindow *ui;

    // Екземпляри класів логіки
    AppInitializer m_initializer;
    BackupManager m_backupManager;
    ChangeTracker m_logger;
    ConfigEditor m_configEditor;
    ConfigManager m_configManager;
    FileValidator m_fileValidator;
    ProfileManager m_profileManager;

    // Допоміжні функції UI
    void showMessage(const QString& title, const QString& text, bool isWarning = false);
    void appendLog(const QString& message);
    void setupConnections(); // Оновимо цю функцію
    void loadUsername();
    void displayFileContent(const QString& filePath, const QString& logActionName);

    // Допоміжні функції для вибору файлів
    QString selectBackupFile();
    QString selectUserConfigFile();
    QString selectFileToValidate();

    // Допоміжні функції для відображення результатів валідації
    void displayValidationResult(const ValidationResult& result, const QString& filename);
    QString formatValidationSummary(const ValidationResult& result);

    // Функція для відображення налаштувань
    void displaySettingsInTreeDialog(const fs::path& configPath, const std::string& windowTitlePrefix);

};
#endif // MAINWINDOW_H
