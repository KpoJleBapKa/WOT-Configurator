#include "mainwindow.h"
#include "ui_mainwindow.h" // Підключення згенерованого UI

// Включаємо необхідні заголовки Qt
#include <QDateTime>
#include <QTextStream>
#include <QVBoxLayout>
#include <QPlainTextEdit> // Потрібен для outputLogArea та діалогу показу контенту
#include <QPushButton>
#include <QFontDatabase>
#include <QDialog>
#include <QInputDialog>   // Для введення імені користувача
#include <QFileDialog>    // Для вибору файлів
#include <QMessageBox>    // Для повідомлень користувачу
#include <QScrollBar>     // Для прокрутки логу
#include <QTreeWidget>    // Для дерева налаштувань
#include <QTreeWidgetItem>// Для елементів дерева
#include <QHeaderView>    // Для налаштування заголовків дерева

// --- Конструктор та Деструктор ---

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) // Створюємо UI з дизайнера
{
    ui->setupUi(this); // Застосовуємо дизайн до вікна
    this->setWindowTitle("WOT Settings GUI by Kroll"); // Встановлюємо заголовок

    appendLog("Запуск програми...");
    m_logger.logAction("Application::Start", true);

    // Налаштовуємо з'єднання сигналів від кнопок до слотів
    setupConnections();

    // Виконуємо початкові дії
    onCheckFoldersClicked(); // Перевіряємо папки при старті
    loadUsername();          // Завантажуємо ім'я користувача (якщо є)

    appendLog("Програма готова до роботи.");
}

MainWindow::~MainWindow()
{
    appendLog("Завершення роботи програми...");
    m_logger.logAction("Application::Exit", true);
    delete ui; // Звільняємо пам'ять від UI
}

// --- Налаштування з'єднань сигнал-слот ---

void MainWindow::setupConnections()
{
    // З'єднуємо кожну кнопку з її слотом-обробником
    connect(ui->checkFoldersButton, &QPushButton::clicked, this, &MainWindow::onCheckFoldersClicked);
    connect(ui->createBackupButton, &QPushButton::clicked, this, &MainWindow::onCreateBackupClicked);
    connect(ui->restoreBackupButton, &QPushButton::clicked, this, &MainWindow::onRestoreBackupClicked);
    connect(ui->setUsernameButton, &QPushButton::clicked, this, &MainWindow::onSetUsernameClicked);
    connect(ui->showUsernameButton, &QPushButton::clicked, this, &MainWindow::onShowUsernameClicked);
    connect(ui->showConfigButton, &QPushButton::clicked, this, &MainWindow::onShowConfigClicked); // Показує конфіг користувача (відфільтровано)
    connect(ui->editConfigButton, &QPushButton::clicked, this, &MainWindow::onEditConfigClicked);
    connect(ui->changeConfigButton, &QPushButton::clicked, this, &MainWindow::onChangeConfigClicked); // Застосовує конфіг користувача
    connect(ui->checkCurrentConfigButton, &QPushButton::clicked, this, &MainWindow::onCheckCurrentConfigClicked); // Показує конфіг гри (відфільтровано)
    connect(ui->validateConfigButton, &QPushButton::clicked, this, &MainWindow::onValidateConfigClicked);
    connect(ui->exitButton, &QPushButton::clicked, this, &MainWindow::onExitClicked);
}

// --- Реалізація слотів-обробників ---

void MainWindow::onCheckFoldersClicked()
{
    appendLog("Перевірка необхідних папок...");
    m_logger.logAction("MainWindow::CheckFolders", true, "Starting check");
    try {
        m_initializer.checkFolders(); // Викликаємо метод логіки
        appendLog("Перевірка папок успішно завершена.");
    } catch (const std::exception& e) {
        QString errorMsg = QString("Помилка під час перевірки папок: %1").arg(QString::fromStdString(e.what()));
        appendLog(errorMsg);
        showMessage("Помилка перевірки папок", errorMsg, true);
        m_logger.logAction("AppInitializer::checkFolders", false, e.what());
    }
}

void MainWindow::onCreateBackupClicked()
{
    appendLog("Створення резервної копії...");
    m_logger.logAction("MainWindow::CreateBackup", true, "Starting backup process");
    try {
        fs::path backupPath = m_backupManager.createBackup(); // Метод логіки повертає шлях
        QString filename = QString::fromStdWString(backupPath.filename().wstring());
        QString msg = QString("Резервну копію успішно створено: %1").arg(filename);
        appendLog(msg);
        showMessage("Резервне копіювання", msg);
        m_logger.logAction("BackupManager::createBackup", true, backupPath.filename().string());
    } catch (const std::exception& e) {
        QString errorMsg = QString("Помилка створення резервної копії: %1").arg(QString::fromStdString(e.what()));
        appendLog(errorMsg);
        showMessage("Помилка резервного копіювання", errorMsg, true);
        m_logger.logAction("BackupManager::createBackup", false, e.what());
    }
}

void MainWindow::onRestoreBackupClicked()
{
    QString backupFile = selectBackupFile(); // Вибираємо файл
    if (backupFile.isEmpty()) {
        appendLog("Відновлення з копії скасовано.");
        m_logger.logAction("MainWindow::RestoreBackup", false, "Cancelled by user (file selection)");
        return;
    }

    fs::path backupPath = backupFile.toStdWString();
    QString filename = QFileInfo(backupFile).fileName();
    appendLog(QString("Відновлення конфігурації з файлу: %1").arg(filename));
    m_logger.logAction("MainWindow::RestoreBackup", true, "Attempting restore from " + filename.toStdString());

    // Валідація файлу перед відновленням
    if (!m_fileValidator.validateBeforeAction(backupPath, "Відновлення з копії")) {
        appendLog("Перевірка перед відновленням не пройдена або скасована.");
        m_logger.logAction("MainWindow::RestoreBackup", false, "Pre-restore validation failed or cancelled: " + filename.toStdString());
        return;
    }
    appendLog("Перевірка файлу перед відновленням пройдена.");

    // Виконуємо відновлення
    try {
        m_backupManager.restoreFromBackup(backupPath); // Викликаємо метод логіки
        QString msg = QString("Конфігурацію успішно відновлено з: %1").arg(filename);
        appendLog(msg);
        showMessage("Відновлення завершено", msg);
        m_logger.logAction("BackupManager::restoreFromBackup", true, filename.toStdString());
    } catch (const std::exception& e) {
        QString errorMsg = QString("Помилка відновлення з копії: %1").arg(QString::fromStdString(e.what()));
        appendLog(errorMsg);
        showMessage("Помилка відновлення", errorMsg, true);
        m_logger.logAction("BackupManager::restoreFromBackup", false, e.what());
    }
}

void MainWindow::onSetUsernameClicked()
{
    bool ok;
    QString currentName;
    try { currentName = QString::fromStdString(m_profileManager.loadNameFromFile()); } catch (...) {}

    QString text = QInputDialog::getText(this, "Встановлення імені користувача",
                                         "Введіть ваш нікнейм World of Tanks:", QLineEdit::Normal,
                                         currentName, &ok);
    if (ok && !text.trimmed().isEmpty()) {
        std::string newName = text.trimmed().toStdString();
        try {
            m_profileManager.setName(newName);
            appendLog(QString("Ім'я користувача '%1' успішно встановлено та збережено.").arg(text.trimmed()));
            showMessage("Ім'я користувача", "Ім'я успішно збережено.");
            m_logger.logAction("MainWindow::SetUsername", true, newName);
        } catch (const std::exception& e) {
            QString errorMsg = QString("Не вдалося зберегти ім'я користувача: %1").arg(QString::fromStdString(e.what()));
            appendLog(errorMsg); showMessage("Помилка збереження", errorMsg, true);
            m_logger.logAction("ProfileManager::setName", false, e.what());
        }
    } else if (ok) {
        showMessage("Встановлення імені", "Ім'я користувача не може бути порожнім.", true);
        m_logger.logAction("MainWindow::SetUsername", false, "Attempted to set empty name");
    } else {
        appendLog("Встановлення імені скасовано.");
        m_logger.logAction("MainWindow::SetUsername", false, "Cancelled by user");
    }
}

void MainWindow::onShowUsernameClicked()
{
    appendLog("Завантаження імені користувача для показу...");
    m_logger.logAction("MainWindow::ShowUsername", true, "Attempting to load name for display");
    try {
        std::string username = m_profileManager.loadNameFromFile();
        if (!username.empty()) {
            QString msg = QString("Поточне ім'я користувача: %1").arg(QString::fromStdString(username));
            appendLog(msg);
            showMessage("Ім'я користувача", msg);
        } else {
            appendLog("Ім'я користувача не знайдено.");
            showMessage("Ім'я користувача", "Ім'я користувача ще не встановлено або файл не знайдено.");
            m_logger.logAction("ProfileManager::loadNameFromFile", false, "Username not set or file not found");
        }
    } catch (const std::exception& e) {
        QString errorMsg = QString("Помилка завантаження імені: %1").arg(QString::fromStdString(e.what()));
        appendLog(errorMsg);
        showMessage("Помилка завантаження", errorMsg, true);
        m_logger.logAction("ProfileManager::loadNameFromFile", false, e.what());
    }
}

void MainWindow::onShowConfigClicked() // Показує конфіг користувача (відфільтровано)
{
    QString userConfigFile = selectUserConfigFile();
    if (userConfigFile.isEmpty()) {
        appendLog("Перегляд конфігу користувача скасовано.");
        m_logger.logAction("MainWindow::ShowUserConfig", false, "Cancelled by user (file selection)");
        return;
    }
    // Викликаємо новий метод для показу у дереві
    displaySettingsInTreeDialog(userConfigFile.toStdWString(), "Конфіг Користувача: ");
}

void MainWindow::onEditConfigClicked()
{
    QString userConfigFile = selectUserConfigFile(); // Вибираємо файл
    if (userConfigFile.isEmpty()) {
        appendLog("Редагування конфігу скасовано (файл не вибрано).");
        m_logger.logAction("MainWindow::EditConfig", false, "Cancelled by user (file selection)");
        return;
    }

    fs::path configPath = userConfigFile.toStdWString();
    QString filename = QFileInfo(userConfigFile).fileName();
    appendLog("Спроба редагування файлу: " + filename);
    m_logger.logAction("MainWindow::EditConfig", true, "Attempting to edit " + filename.toStdString());

    // Валідація файлу перед редагуванням
    if (!m_fileValidator.validateBeforeAction(configPath, "Редагування конфігу", false)) {
        appendLog("Перевірка файлу перед редагуванням не пройдена або скасована.");
        m_logger.logAction("MainWindow::EditConfig", false, "Pre-edit validation failed or cancelled: " + filename.toStdString());
        return;
    }

    try {
        // Крок 1: Отримуємо поточні відфільтровані налаштування
        FilteredSettingsMap currentSettings = m_configEditor.getFilteredSettings(configPath);

        if (currentSettings.empty()) {
            showMessage("Редагування", "Не знайдено відомих налаштувань для редагування у файлі " + filename, true);
            appendLog("Не знайдено відомих налаштувань для редагування у: " + filename);
            m_logger.logAction("ConfigEditor::getFilteredSettings", false, "No known settings found for editing in " + filename.toStdString());
            return;
        }

        // Крок 2: Створюємо та показуємо діалог редагування
        ConfigEditDialog editDialog(currentSettings, configPath, this);
        if (editDialog.exec() == QDialog::Accepted) { // Якщо користувач натиснув "Зберегти" і валідація пройшла
            appendLog("Збереження змін після редагування...");
            // Крок 3: Отримуємо оновлені налаштування з діалогу
            FilteredSettingsMap updatedSettings = editDialog.getUpdatedSettings();

            // Крок 4: Зберігаємо зміни у файл через ConfigEditor
            try {
                m_configEditor.saveFilteredSettings(configPath, updatedSettings);
                appendLog("Зміни у файлі '" + filename + "' успішно збережено.");
                showMessage("Редагування", "Зміни успішно збережено.");
                m_logger.logAction("ConfigEditor::saveFilteredSettings", true, filename.toStdString());
            } catch (const std::exception& saveError) {
                QString errorMsg = QString("Помилка збереження змін у файл '%1': %2").arg(filename).arg(QString::fromStdString(saveError.what()));
                appendLog(errorMsg);
                showMessage("Помилка збереження", errorMsg, true);
                m_logger.logAction("ConfigEditor::saveFilteredSettings", false, saveError.what());
            }
        } else { // Якщо користувач натиснув "Скасувати"
            appendLog("Редагування файлу '" + filename + "' скасовано користувачем.");
            m_logger.logAction("MainWindow::EditConfig", false, "Cancelled by user in dialog: " + filename.toStdString());
        }

    } catch (const std::exception& e) { // Ловимо помилки завантаження/парсингу для редагування
        QString errorMsg = QString("Помилка підготовки до редагування файлу '%1': %2").arg(filename).arg(QString::fromStdString(e.what()));
        appendLog(errorMsg);
        showMessage("Помилка редагування", errorMsg, true);
        m_logger.logAction("MainWindow::EditConfig", false, e.what());
    }
}


void MainWindow::onChangeConfigClicked() // Застосовує конфіг користувача
{
    QString userConfigFile = selectUserConfigFile();
    if (userConfigFile.isEmpty()) {
        appendLog("Застосування конфігу користувача скасовано.");
        m_logger.logAction("MainWindow::ChangeConfig", false, "Cancelled by user (file selection)");
        return;
    }

    fs::path sourcePath = userConfigFile.toStdWString();
    QString filename = QFileInfo(userConfigFile).fileName();
    appendLog(QString("Спроба застосування конфігу: %1").arg(filename));
    m_logger.logAction("MainWindow::ChangeConfig", true, "Attempting to apply " + filename.toStdString());

    // Валідація файлу перед застосуванням
    if (!m_fileValidator.validateBeforeAction(sourcePath, "Застосування конфігу")) {
        appendLog("Перевірка перед застосуванням не пройдена або скасована.");
        m_logger.logAction("MainWindow::ChangeConfig", false, "Pre-apply validation failed or cancelled: " + filename.toStdString());
        return;
    }
    appendLog("Перевірка файлу перед застосуванням пройдена.");

    // Виконуємо застосування
    try {
        m_configManager.changeCurrentConfig(sourcePath); // Викликаємо метод логіки
        QString msg = QString("Конфіг '%1' успішно застосовано до гри.").arg(filename);
        appendLog(msg);
        showMessage("Застосування конфігу", msg);
        m_logger.logAction("ConfigManager::changeCurrentConfig", true, filename.toStdString());
    } catch (const std::exception& e) {
        QString errorMsg = QString("Помилка застосування конфігу: %1").arg(QString::fromStdString(e.what()));
        appendLog(errorMsg);
        showMessage("Помилка застосування", errorMsg, true);
        m_logger.logAction("ConfigManager::changeCurrentConfig", false, e.what());
    }
}

void MainWindow::onCheckCurrentConfigClicked() // Показує конфіг гри (відфільтровано)
{
    appendLog("Перегляд поточного конфігу гри...");
    m_logger.logAction("MainWindow::CheckCurrentConfig", true, "Attempting to view game config");
    try {
        fs::path gameConfigPath = m_configManager.getCurrentGameConfigPath(); // Отримуємо шлях
        if (!gameConfigPath.empty() && fs::exists(gameConfigPath)) {
            // Викликаємо новий метод для показу у дереві
            displaySettingsInTreeDialog(gameConfigPath, "Поточний Конфіг Гри: ");
        } else {
            appendLog("Поточний конфіг гри не знайдено.");
            showMessage("Перегляд конфігу", "Файл конфігурації гри (preferences.xml) не знайдено.", true);
            m_logger.logAction("ConfigManager::getCurrentGameConfigPath", false, "preferences.xml not found");
        }
    } catch (const std::exception& e) {
        QString errorMsg = QString("Помилка отримання шляху до конфігу гри: %1").arg(QString::fromStdString(e.what()));
        appendLog(errorMsg);
        showMessage("Помилка", errorMsg, true);
        m_logger.logAction("ConfigManager::getCurrentGameConfigPath", false, e.what());
    }
}

void MainWindow::onValidateConfigClicked()
{
    QString fileToValidate = selectFileToValidate(); // Вибираємо файл
    if (fileToValidate.isEmpty()) {
        appendLog("Валідацію файлу скасовано.");
        m_logger.logAction("MainWindow::ValidateConfig", false, "Cancelled by user (file selection)");
        return;
    }

    fs::path validatePath = fileToValidate.toStdWString();
    QString filename = QFileInfo(fileToValidate).fileName();
    appendLog(QString("--- Валідація файлу: %1 ---").arg(filename));
    m_logger.logAction("MainWindow::ValidateConfig", true, "Starting validation for " + filename.toStdString());

    try {
        ValidationResult result = m_fileValidator.validateFile(validatePath); // Викликаємо метод логіки
        displayValidationResult(result, filename); // Показуємо результат у вікні
        // Логуємо короткий результат
        m_logger.logAction("FileValidator::validateFile", result.isValid(), filename.toStdString() + " - " + formatValidationSummary(result).toStdString());

    } catch (const std::exception& e) { // Ловимо помилки самої валідації
        QString errorMsg = QString("Помилка під час валідації файлу '%1': %2").arg(filename).arg(QString::fromStdString(e.what()));
        appendLog(errorMsg);
        showMessage("Помилка валідації", errorMsg, true);
        m_logger.logAction("FileValidator::validateFile", false, e.what());
    }
}


void MainWindow::onExitClicked()
{
    appendLog("Завершення роботи програми...");
    m_logger.logAction("MainWindow::Exit", true);
    QApplication::quit(); // Завершуємо роботу додатку
}

// --- Допоміжні функції UI ---

void MainWindow::showMessage(const QString& title, const QString& text, bool isWarning)
{
    if (isWarning) { QMessageBox::warning(this, title, text); }
    else { QMessageBox::information(this, title, text); }
}

void MainWindow::appendLog(const QString& message)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    ui->outputLogArea->appendPlainText(timestamp + " | " + message); // Додаємо текст в лог
    // Автоматична прокрутка донизу
    ui->outputLogArea->verticalScrollBar()->setValue(ui->outputLogArea->verticalScrollBar()->maximum());
}

void MainWindow::loadUsername()
{
    try {
        std::string username = m_profileManager.loadNameFromFile();
        if (!username.empty()) {
            appendLog(QString("Завантажено ім'я користувача: %1").arg(QString::fromStdString(username)));
        } else {
            appendLog("Файл імені користувача не знайдено або він порожній при старті.");
        }
    } catch (const std::exception& e) {
        appendLog(QString("Помилка завантаження імені користувача при старті: %1").arg(QString::fromStdString(e.what())));
        m_logger.logAction("MainWindow::loadUsername", false, e.what());
    }
}

// Показує ВІДФІЛЬТРОВАНІ налаштування у новому діалоговому вікні з деревом
void MainWindow::displaySettingsInTreeDialog(const fs::path& configPath, const std::string& windowTitlePrefix)
{
    QString qConfigPath = QString::fromStdWString(configPath.wstring());
    QString filename = QFileInfo(qConfigPath).fileName();
    std::string logActionName = windowTitlePrefix;
    logActionName.pop_back(); logActionName.pop_back(); // Прибираємо ": " з префікса для логу

    appendLog(QString("Спроба відображення відфільтрованих налаштувань: %1").arg(filename));
    m_logger.logAction("MainWindow::"+logActionName, true, "Attempting to display filtered " + filename.toStdString());

    // Валідація файлу перед показом (використовуємо допоміжний метод)
    if (!m_fileValidator.validateBeforeAction(configPath, "Перегляд налаштувань", false)) { // false - не показувати успіх
        appendLog("Перевірка файлу перед показом налаштувань не пройдена або скасована.");
        m_logger.logAction("MainWindow::"+logActionName, false, "Pre-display validation failed or cancelled: " + filename.toStdString());
        return;
    }

    try {
        // Отримуємо відфільтровані дані з ConfigEditor
        FilteredSettingsMap settings = m_configEditor.getFilteredSettings(configPath);

        if (settings.empty()) {
            showMessage("Перегляд налаштувань", "Не знайдено відомих налаштувань у файлі " + filename, true);
            appendLog("Не знайдено відомих налаштувань у файлі: " + filename);
            m_logger.logAction("ConfigEditor::getFilteredSettings", false, "No known settings found in " + filename.toStdString());
            return;
        }

        // Створюємо нове діалогове вікно
        QDialog *dialog = new QDialog(this);
        dialog->setAttribute(Qt::WA_DeleteOnClose); // Важливо! Автоматично видалить вікно при закритті
        dialog->setWindowTitle(QString::fromStdString(windowTitlePrefix) + filename);
        dialog->setMinimumSize(550, 450); // Збільшимо мінімальний розмір

        QVBoxLayout *layout = new QVBoxLayout(dialog);

        // Створюємо дерево
        QTreeWidget *treeWidget = new QTreeWidget(dialog);
        treeWidget->setColumnCount(2); // Дві колонки
        treeWidget->setHeaderLabels(QStringList() << "Категорія / Налаштування" << "Значення");
        treeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents); // Автоширина першої колонки
        treeWidget->header()->setStretchLastSection(true); // Друга колонка розтягується

        // Заповнюємо дерево категоріями та налаштуваннями
        for (const auto& categoryPair : settings) {
            QTreeWidgetItem *categoryItem = new QTreeWidgetItem(treeWidget);
            categoryItem->setText(0, QString::fromStdString(categoryPair.first));
            QFont boldFont = categoryItem->font(0); // Беремо поточний шрифт
            boldFont.setBold(true);                 // Робимо його жирним
            categoryItem->setFont(0, boldFont);     // Встановлюємо жирний шрифт для категорії
            categoryItem->setExpanded(true);        // Розгортаємо категорію

            for (const auto& settingPair : categoryPair.second) {
                QTreeWidgetItem *settingItem = new QTreeWidgetItem(categoryItem); // Додаємо як дочірній
                settingItem->setText(0, QString::fromStdString(settingPair.first));  // Ім'я
                settingItem->setText(1, QString::fromStdString(settingPair.second)); // Значення
            }
        }

        layout->addWidget(treeWidget);

        // Кнопка закриття
        QPushButton *closeButton = new QPushButton("Закрити", dialog);
        connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
        layout->addWidget(closeButton);

        dialog->setLayout(layout);
        dialog->show(); // Показуємо немодально

        appendLog(QString("Відфільтровані налаштування з '%1' відображено.").arg(filename));
        m_logger.logAction("ConfigEditor::getFilteredSettings", true, "Displayed filtered settings for " + filename.toStdString());

    } catch (const std::exception& e) {
        QString errorMsg = QString("Помилка під час отримання/відображення налаштувань з '%1': %2")
                               .arg(filename).arg(QString::fromStdString(e.what()));
        appendLog(errorMsg);
        showMessage("Помилка відображення налаштувань", errorMsg, true);
        m_logger.logAction("ConfigEditor::getFilteredSettings", false, e.what());
    }
}


// Показує ПОВНИЙ вміст файлу у новому діалоговому вікні
void MainWindow::displayFileContent(const QString& filePath, const QString& logActionName)
{
    QFile file(filePath);
    QString filename = QFileInfo(filePath).fileName();
    fs::path fsPath = filePath.toStdWString();

    appendLog(QString("Спроба відображення повного вмісту файлу: %1").arg(filename));
    m_logger.logAction(logActionName.toStdString(), true, "Attempting to display FULL content of " + filename.toStdString());

    if (!file.exists()) {
        QString errorMsg = QString("Файл не знайдено: %1").arg(filename);
        appendLog(errorMsg); showMessage("Помилка читання", errorMsg, true);
        m_logger.logAction(logActionName.toStdString(), false, "File not found: " + filename.toStdString());
        return;
    }

    // --- Валідація перед показом (просто перевірка на XML) ---
    try {
        ValidationResult valResult = m_fileValidator.validateFile(fsPath);
        if (!valResult.isValid()) { // Якщо XML некоректний
            QString errorMsg = QString("Файл '%1' не є коректним XML і не може бути відображений: %2")
                                   .arg(filename).arg(QString::fromStdString(valResult.wellFormedError));
            appendLog(errorMsg); showMessage("Помилка формату", errorMsg, true);
            m_logger.logAction(logActionName.toStdString(), false, "Not well-formed XML: " + filename.toStdString());
            return; // Не показуємо некоректний XML
        }
    } catch (const std::exception& e) {
        appendLog(QString("Помилка під час валідації файлу '%1' перед показом повного вмісту: %2").arg(filename).arg(e.what()));
        m_logger.logAction("FileValidator::validateFile", false, "Exception during validation for full display: " + std::string(e.what()));
        // Спробуємо показати все одно
    }
    // --- Кінець валідації ---

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString content = in.readAll();
        file.close();

        // Створення та показ діалогового вікна
        QDialog *displayDialog = new QDialog(this);
        displayDialog->setAttribute(Qt::WA_DeleteOnClose);
        displayDialog->setWindowTitle(QString("Повний Вміст Файлу: %1").arg(filename));
        QVBoxLayout *layout = new QVBoxLayout(displayDialog);
        QPlainTextEdit *textEdit = new QPlainTextEdit(displayDialog);
        textEdit->setPlainText(content);
        textEdit->setReadOnly(true);
        textEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
        layout->addWidget(textEdit);
        QPushButton *closeButton = new QPushButton("Закрити", displayDialog);
        connect(closeButton, &QPushButton::clicked, displayDialog, &QDialog::accept);
        layout->addWidget(closeButton);
        displayDialog->setLayout(layout);
        displayDialog->resize(700, 500);
        displayDialog->show();

        appendLog(QString("Повний вміст файлу '%1' відображено у окремому вікні.").arg(filename));
        m_logger.logAction(logActionName.toStdString(), true, "Displayed FULL content for " + filename.toStdString());

    } else {
        QString errorMsg = QString("Не вдалося відкрити файл '%1' для читання.").arg(filename);
        appendLog(errorMsg); showMessage("Помилка читання", errorMsg, true);
        m_logger.logAction(logActionName.toStdString(), false, "Failed to open file: " + filename.toStdString());
    }
}

// Допоміжна функція для вибору файлу резервної копії
QString MainWindow::selectBackupFile()
{
    fs::path backupDir = "Restored Configs";
    if (!fs::exists(backupDir)) { fs::create_directories(backupDir); }
    return QFileDialog::getOpenFileName(this,
                                        "Виберіть файл резервної копії",
                                        QString::fromStdWString(fs::absolute(backupDir).wstring()),
                                        "XML files (*.xml)");
}

// Допоміжна функція для вибору файлу конфігурації користувача
QString MainWindow::selectUserConfigFile()
{
    fs::path userDir = "User Configs";
    if (!fs::exists(userDir)) { fs::create_directories(userDir); }
    return QFileDialog::getOpenFileName(this,
                                        "Виберіть конфіг користувача",
                                        QString::fromStdWString(fs::absolute(userDir).wstring()),
                                        "XML files (*.xml)");
}

// Допоміжна функція для вибору файлу для валідації
QString MainWindow::selectFileToValidate()
{
    return QFileDialog::getOpenFileName(this,
                                        "Виберіть файл для валідації",
                                        ".", // Поточна директорія як початкова
                                        "XML files (*.xml);;All files (*)");
}

// Допоміжна функція для формування рядка-резюме результату валідації
QString MainWindow::formatValidationSummary(const ValidationResult& result)
{
    if (!result.isWellFormed) return "Помилка: Некоректний XML. " + QString::fromStdString(result.wellFormedError);
    QString summary = "XML: OK. ";
    summary += QString("Структура: %1. ").arg(QString::fromStdString(result.structureInfo));
    if (result.valueErrors.empty()) {
        summary += "Значення: OK.";
    } else {
        summary += QString("Значення: %1 попереджень.").arg(result.valueErrors.size());
    }
    return summary;
}

// Допоміжна функція для показу результатів валідації у QMessageBox
void MainWindow::displayValidationResult(const ValidationResult& result, const QString& filename)
{
    QString details = formatValidationSummary(result) + "\n";

    if (!result.structureInfo.empty() && result.structureInfo != "OK") {
        details += "\nПопередження структури:\n- " + QString::fromStdString(result.structureInfo);
    }
    if (!result.valueErrors.empty()) {
        details += "\nПопередження значень:\n";
        int count = 0;
        for(const auto& err : result.valueErrors) {
            if (++count > 5) {
                details += "- ... (ще " + QString::number(result.valueErrors.size() - 5) + ")\n";
                break;
            }
            details += QString("- %1\n").arg(QString::fromStdString(err));
        }
    }

    QMessageBox msgBox;
    msgBox.setWindowTitle("Результат валідації: " + filename);
    msgBox.setText(details);
    msgBox.setTextInteractionFlags(Qt::TextSelectableByMouse); // Дозволити копіювання тексту
    msgBox.setIcon(result.isValid() ? QMessageBox::Information : QMessageBox::Warning); // Warning якщо є попередження, але XML валідний
    if (!result.isWellFormed) msgBox.setIcon(QMessageBox::Critical); // Critical якщо XML невалідний
    msgBox.exec();
}
