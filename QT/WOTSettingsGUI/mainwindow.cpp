#include "mainwindow.h" // Підключення заголовка для нашого вікна
#include "ui_mainwindow.h" // Підключення заголовка, згенерованого з .ui файлу дизайнера

// Конструктор класу MainWindow
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) // Викликаємо конструктор базового класу QMainWindow
    , ui(new Ui::MainWindow) // Створюємо об'єкт ui (представлення форми з дизайнера)
{
    // Цей рядок застосовує дизайн з файлу mainwindow.ui до поточного вікна
    ui->setupUi(this);

    // --- Тут пізніше буде код для підключення кнопок (connect) ---
    // connect(ui->createBackupButton, &QPushButton::clicked, this, &MainWindow::onCreateBackupClicked);
    // --- та інша ініціалізація ---
}

// Деструктор класу MainWindow
MainWindow::~MainWindow()
{
    // Обов'язково видаляємо об'єкт ui, щоб уникнути витоків пам'яті
    delete ui;
}

// --- Тут пізніше будуть реалізації твоїх слотів (обробників подій) ---
/*
void MainWindow::onCreateBackupClicked()
{
    // код для кнопки Create Backup
}
*/
