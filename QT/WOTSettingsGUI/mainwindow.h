#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "main.h" // <-- Включаємо твій головний заголовок тут

// Forward-декларація для Ui::MainWindow
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT // Макрос для сигналів/слотів

public:
    MainWindow(QWidget *parent = nullptr); // Конструктор
    ~MainWindow();                         // Деструктор

    // Додай цю секцію для слотів (обробників сигналів від кнопок)
private slots:
               // Приклади можливих слотів (ми їх додамо пізніше)
               // void onCreateBackupClicked();
               // void onRestoreBackupClicked();
               // void onApplyConfigClicked();
               // ... інші слоти ...

private:
    Ui::MainWindow *ui;

    // --- ДОДАЙ АБО ПЕРЕВІР НАЯВНІСТЬ ЦИХ РЯДКІВ ---
    AppInitializer m_initializer;
    BackupManager m_backupManager;
    ChangeTracker m_logger;
    ConfigEditor m_configEditor;
    ConfigManager m_configManager;
    FileValidator m_fileValidator;
    ProfileManager m_profileManager;
    // ---------------------------------------------
};
#endif // MAINWINDOW_H
