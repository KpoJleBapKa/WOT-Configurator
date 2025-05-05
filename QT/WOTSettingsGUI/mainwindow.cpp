#include "mainwindow.h"
#include "ui_mainwindow.h"     // Підключення згенерованого UI
#include "helpdialog.h"        // <-- ВКЛЮЧЕНО helpdialog.h
#include "statsdialog.h"
#include "aichatdialog.h"

// Включаємо необхідні заголовки Qt
#include <QDateTime>
#include <QTextStream>
#include <QVBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QFontDatabase>
#include <QDialog>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QStackedWidget>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("WOT Settings GUI by Kroll");

    appendLog("Запуск програми...");
    m_logger.logAction("Application::Start", true);

    // Налаштовуємо з'єднання сигналів
    setupConnections();

    // Виконуємо початкові дії
    onCheckFoldersClicked();
    loadUsername();

    appendLog("Програма готова до роботи.");
}

MainWindow::~MainWindow()
{
    appendLog("Завершення роботи програми...");
    m_logger.logAction("Application::Exit", true);
    delete ui;
}

// --- Налаштування з'єднань ---
void MainWindow::setupConnections()
{
    connect(ui->checkFoldersButton, &QPushButton::clicked, this, &MainWindow::onCheckFoldersClicked);
    connect(ui->createBackupButton, &QPushButton::clicked, this, &MainWindow::onCreateBackupClicked);
    connect(ui->restoreBackupButton, &QPushButton::clicked, this, &MainWindow::onRestoreBackupClicked);
    connect(ui->setUsernameButton, &QPushButton::clicked, this, &MainWindow::onSetUsernameClicked);
    connect(ui->showUsernameButton, &QPushButton::clicked, this, &MainWindow::onShowUsernameClicked);
    connect(ui->showConfigButton, &QPushButton::clicked, this, &MainWindow::onShowConfigClicked);
    connect(ui->editConfigButton, &QPushButton::clicked, this, &MainWindow::onEditConfigClicked);
    connect(ui->changeConfigButton, &QPushButton::clicked, this, &MainWindow::onChangeConfigClicked);
    connect(ui->checkCurrentConfigButton, &QPushButton::clicked, this, &MainWindow::onCheckCurrentConfigClicked);
    connect(ui->validateConfigButton, &QPushButton::clicked, this, &MainWindow::onValidateConfigClicked);
    connect(ui->exitButton, &QPushButton::clicked, this, &MainWindow::onExitClicked);

    if (ui->helpButton) {
        connect(ui->helpButton, &QPushButton::clicked, this, &MainWindow::onHelpButtonClicked);
    } else {
        qWarning() << "setupConnections: ui->helpButton is null!";
    }
    if (ui->statsButton) {
        connect(ui->statsButton, &QPushButton::clicked, this, &MainWindow::onStatsButtonClicked);
    } else {
        qWarning() << "setupConnections: ui->statsButton is null!";
    }
    if (ui->aiChatButton) {
        connect(ui->aiChatButton, &QPushButton::clicked, this, &MainWindow::onAIChatButtonClicked);
    } else {
        qWarning() << "setupConnections: ui->aiChatButton is null!";
    }
}

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
    displaySettingsInTreeDialog(userConfigFile.toStdWString(), "Конфіг Користувача: ");
}

void MainWindow::onEditConfigClicked()
{
    QString userConfigFile = selectUserConfigFile();
    if (userConfigFile.isEmpty()) {
        appendLog("Редагування конфігу скасовано (файл не вибрано).");
        m_logger.logAction("MainWindow::EditConfig", false, "Cancelled by user (file selection)");
        return;
    }

    fs::path configPath = userConfigFile.toStdWString();
    QString filename = QFileInfo(userConfigFile).fileName();
    appendLog("Спроба редагування файлу: " + filename);
    m_logger.logAction("MainWindow::EditConfig", true, "Attempting to edit " + filename.toStdString());

    if (!m_fileValidator.validateBeforeAction(configPath, "Редагування конфігу", false)) {
        appendLog("Перевірка файлу перед редагуванням не пройдена або скасована.");
        m_logger.logAction("MainWindow::EditConfig", false, "Pre-edit validation failed or cancelled: " + filename.toStdString());
        return;
    }

    try {
        FilteredSettingsMap currentSettings = m_configEditor.getFilteredSettings(configPath);
        if (currentSettings.empty()) {
            showMessage("Редагування", "Не знайдено відомих налаштувань для редагування у файлі " + filename, true);
            appendLog("Не знайдено відомих налаштувань для редагування у: " + filename);
            m_logger.logAction("ConfigEditor::getFilteredSettings", false, "No known settings found for editing in " + filename.toStdString());
            return;
        }

        ConfigEditDialog editDialog(currentSettings, configPath, this);
        if (editDialog.exec() == QDialog::Accepted) {
            appendLog("Збереження змін після редагування...");
            FilteredSettingsMap updatedSettings = editDialog.getUpdatedSettings();
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
        } else {
            appendLog("Редагування файлу '" + filename + "' скасовано користувачем.");
            m_logger.logAction("MainWindow::EditConfig", false, "Cancelled by user in dialog: " + filename.toStdString());
        }

    } catch (const std::exception& e) {
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

    if (!m_fileValidator.validateBeforeAction(sourcePath, "Застосування конфігу")) {
        appendLog("Перевірка перед застосуванням не пройдена або скасована.");
        m_logger.logAction("MainWindow::ChangeConfig", false, "Pre-apply validation failed or cancelled: " + filename.toStdString());
        return;
    }
    appendLog("Перевірка файлу перед застосуванням пройдена.");

    try {
        m_configManager.changeCurrentConfig(sourcePath);
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
        fs::path gameConfigPath = m_configManager.getCurrentGameConfigPath();
        if (!gameConfigPath.empty() && fs::exists(gameConfigPath)) {
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
    QString fileToValidate = selectFileToValidate();
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
        ValidationResult result = m_fileValidator.validateFile(validatePath);
        displayValidationResult(result, filename);
        m_logger.logAction("FileValidator::validateFile", result.isValid(), filename.toStdString() + " - " + formatValidationSummary(result).toStdString());

    } catch (const std::exception& e) {
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
    QApplication::quit();
}


// --- НОВИЙ СЛОТ для кнопки "Довідка" ---
void MainWindow::onHelpButtonClicked()
{
    appendLog("Відкриття вікна довідки...");
    m_logger.logAction("MainWindow::ShowHelp", true);

    // Створюємо та показуємо діалог довідки модально
    HelpDialog helpDlg(this); // Створюємо екземпляр діалогу
    helpDlg.exec(); // Показуємо діалог і чекаємо, поки його закриють

    appendLog("Вікно довідки закрито.");
}

void MainWindow::onStatsButtonClicked()
{
    appendLog("Відкриття вікна статистики гравця...");
    m_logger.logAction("MainWindow::ShowStats", true);

    // Створюємо та показуємо діалог статистики модально
    StatsDialog statsDlg(this); // Створюємо екземпляр діалогу
    statsDlg.exec(); // Показуємо діалог і чекаємо, поки його закриють

    appendLog("Вікно статистики гравця закрито.");
    // Логування результату пошуку відбувається всередині StatsDialog
}

void MainWindow::onAIChatButtonClicked()
{
    appendLog("Відкриття AI Помічника...");
    m_logger.logAction("MainWindow::ShowAIChat", true);

    // Створюємо та показуємо діалог AI модально
    AIChatDialog aiDlg(this);
    aiDlg.exec(); // Показуємо діалог і чекаємо, поки його закриють

    appendLog("Вікно AI Помічника закрито.");
}

// --- Допоміжні функції UI (без змін) ---
// ... (showMessage, appendLog, loadUsername, displaySettingsInTreeDialog, displayFileContent) ...
void MainWindow::showMessage(const QString& title, const QString& text, bool isWarning)
{
    if (isWarning) { QMessageBox::warning(this, title, text); }
    else { QMessageBox::information(this, title, text); }
}

void MainWindow::appendLog(const QString& message)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    if(ui->outputLogArea) {
        ui->outputLogArea->appendPlainText(timestamp + " | " + message);
        ui->outputLogArea->verticalScrollBar()->setValue(ui->outputLogArea->verticalScrollBar()->maximum());
    } else {
        qWarning() << "appendLog: ui->outputLogArea is null!";
    }
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

void MainWindow::displaySettingsInTreeDialog(const fs::path& configPath, const std::string& windowTitlePrefix)
{
    QString qConfigPath = QString::fromStdWString(configPath.wstring());
    QString filename = QFileInfo(qConfigPath).fileName();
    std::string logActionNameBase = windowTitlePrefix;
    if (logActionNameBase.length() >= 2 && logActionNameBase.substr(logActionNameBase.length() - 2) == ": ") {
        logActionNameBase.resize(logActionNameBase.length() - 2);
    }
    std::string logActionName = "MainWindow::Display" + logActionNameBase;

    appendLog(QString("Спроба відображення відфільтрованих налаштувань: %1").arg(filename));
    m_logger.logAction(logActionName, true, "Attempting to display filtered " + filename.toStdString());

    if (!m_fileValidator.validateBeforeAction(configPath, "Перегляд налаштувань", false)) {
        appendLog("Перевірка файлу перед показом налаштувань не пройдена або скасована.");
        m_logger.logAction(logActionName, false, "Pre-display validation failed or cancelled: " + filename.toStdString());
        return;
    }

    try {
        FilteredSettingsMap settings = m_configEditor.getFilteredSettings(configPath);

        if (settings.empty()) {
            showMessage("Перегляд налаштувань", "Не знайдено відомих налаштувань у файлі " + filename, true);
            appendLog("Не знайдено відомих налаштувань у файлі: " + filename);
            m_logger.logAction("ConfigEditor::getFilteredSettings", false, "No known settings found in " + filename.toStdString());
            return;
        }

        QDialog *dialog = new QDialog(this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setWindowTitle(QString::fromStdString(windowTitlePrefix) + filename);
        dialog->setMinimumSize(600, 500);

        QVBoxLayout *layout = new QVBoxLayout(dialog);

        QTreeWidget *treeWidget = new QTreeWidget(dialog);
        treeWidget->setColumnCount(2);
        treeWidget->setHeaderLabels(QStringList() << "Налаштування" << "Значення");
        treeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        treeWidget->header()->setStretchLastSection(true);
        treeWidget->setAlternatingRowColors(true);

        std::map<std::string, std::string> nameMap;
        nameMap["masterVolume"] = "Загальна гучність";
        nameMap["volume_micVivox"] = "Гучність мікрофону (Vivox)";
        nameMap["volume_vehicles"] = "Гучність: Техніка";
        nameMap["volume_music"] = "Гучність: Музика";
        nameMap["volume_effects"] = "Гучність: Ефекти";
        nameMap["volume_ambient"] = "Гучність: Оточення";
        nameMap["volume_gui"] = "Гучність: Інтерфейс";
        nameMap["volume_voice"] = "Гучність: Голосові повідомлення";
        nameMap["soundMode"] = "Режим звуку";
        nameMap["graphicsSettingsVersion"] = "Версія налаштувань графіки";
        nameMap["COLOR_GRADING_TECHNIQUE"] = "Техніка корекції кольору";
        nameMap["CUSTOM_AA_MODE"] = "Режим згладжування";
        nameMap["DECOR_LEVEL"] = "Якість декалей";
        nameMap["EFFECTS_QUALITY"] = "Якість ефектів";
        nameMap["FAR_PLANE"] = "Дальність промальовки";
        nameMap["FLORA_QUALITY"] = "Якість трави";
        nameMap["HAVOK_ENABLED"] = "Фізика руйнувань Havok";
        nameMap["HAVOK_QUALITY"] = "Якість фізики Havok";
        nameMap["LIGHTING_QUALITY"] = "Якість освітлення";
        nameMap["MOTION_BLUR_QUALITY"] = "Якість розмиття в русі";
        nameMap["OBJECT_LOD"] = "Деталізація об'єктів";
        nameMap["POST_PROCESSING_QUALITY"] = "Якість постобробки";
        nameMap["SEMITRANSPARENT_LEAVES_ENABLED"] = "Прозорість листя";
        nameMap["SHADOWS_QUALITY"] = "Якість тіней";
        nameMap["SNIPER_MODE_EFFECTS_QUALITY"] = "Якість ефектів (снайп. режим)";
        nameMap["SNIPER_MODE_GRASS_ENABLED"] = "Трава в снайп. режимі";
        nameMap["SNIPER_MODE_SWINGING_ENABLED"] = "Хитання камери (снайп. режим)";
        nameMap["SPEEDTREE_QUALITY"] = "Якість дерев";
        nameMap["TERRAIN_QUALITY"] = "Якість ландшафту";
        nameMap["TERRAIN_TESSELLATION_ENABLED"] = "Теселяція ландшафту";
        nameMap["TEXTURE_QUALITY"] = "Якість текстур";
        nameMap["TRACK_PHYSICS_QUALITY"] = "Фізика гусениць";
        nameMap["VEHICLE_DUST_ENABLED"] = "Пил з-під техніки";
        nameMap["VEHICLE_TRACES_ENABLED"] = "Сліди техніки";
        nameMap["WATER_QUALITY"] = "Якість води";
        nameMap["colorGradingStrength"] = "Сила корекції кольору";
        nameMap["brightnessDeferred"] = "Яскравість";
        nameMap["contrastDeferred"] = "Контрастність";
        nameMap["saturationDeferred"] = "Насиченість";
        nameMap["windowMode"] = "Режим вікна";
        nameMap["windowedWidth"] = "Ширина (вікно)";
        nameMap["windowedHeight"] = "Висота (вікно)";
        nameMap["fullscreenWidth"] = "Ширина (повний екран)";
        nameMap["fullscreenHeight"] = "Висота (повний екран)";
        nameMap["fullscreenRefresh"] = "Частота оновлення";
        nameMap["aspectRatio"] = "Співвідношення сторін";
        nameMap["gamma"] = "Гамма";
        nameMap["tripleBuffering"] = "Потрійна буферизація";
        std::map<std::string, std::string> modeNamesUKR = {
            {"strategicMode", "Стратегічний"}, {"artyMode", "Арт-САУ"},
            {"arcadeMode", "Аркадний"}, {"sniperMode", "Снайперський"},
            {"freeVideoMode", "Вільна камера"}
        };
        for(const auto& modePair : modeNamesUKR) {
            std::string modePrefix = modePair.second + ": ";
            nameMap[modePair.first + "/horzInvert"] = modePrefix + "Інверсія по горизонталі";
            nameMap[modePair.first + "/vertInvert"] = modePrefix + "Інверсія по вертикалі";
            nameMap[modePair.first + "/keySensitivity"] = modePrefix + "Чутливість клавіатури";
            nameMap[modePair.first + "/sensitivity"] = modePrefix + "Чутливість миші";
            nameMap[modePair.first + "/scrollSensitivity"] = modePrefix + "Чутливість прокрутки";
        }

        QFont categoryFont = treeWidget->font();
        categoryFont.setBold(true);
        for (const auto& categoryPair : settings) {
            QTreeWidgetItem *categoryItem = new QTreeWidgetItem(treeWidget);
            categoryItem->setText(0, QString::fromStdString(categoryPair.first));
            categoryItem->setFont(0, categoryFont);
            categoryItem->setExpanded(true);

            for (const auto& settingPair : categoryPair.second) {
                std::string settingName = settingPair.first;
                QString displayValue = QString::fromStdString(settingPair.second);
                QString displayNameToShow = QString::fromStdString(settingName);

                if (nameMap.count(settingName)) {
                    displayNameToShow = QString::fromStdString(nameMap[settingName]);
                }

                QTreeWidgetItem *settingItem = new QTreeWidgetItem(categoryItem);
                settingItem->setText(0, displayNameToShow);
                settingItem->setText(1, displayValue);
            }
        }

        layout->addWidget(treeWidget);

        QPushButton *closeButton = new QPushButton("Закрити", dialog);
        connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
        layout->addWidget(closeButton);

        dialog->setLayout(layout);
        dialog->show();

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

// Допоміжні функції для вибору файлів (без змін)
QString MainWindow::selectBackupFile()
{
    fs::path backupDir = "Restored Configs";
    if (!fs::exists(backupDir)) { fs::create_directories(backupDir); }
    return QFileDialog::getOpenFileName(this,
                                        "Виберіть файл резервної копії",
                                        QString::fromStdWString(fs::absolute(backupDir).wstring()),
                                        "XML files (*.xml)");
}

QString MainWindow::selectUserConfigFile()
{
    fs::path userDir = "User Configs";
    if (!fs::exists(userDir)) { fs::create_directories(userDir); }
    return QFileDialog::getOpenFileName(this,
                                        "Виберіть конфіг користувача",
                                        QString::fromStdWString(fs::absolute(userDir).wstring()),
                                        "XML files (*.xml)");
}

QString MainWindow::selectFileToValidate()
{
    return QFileDialog::getOpenFileName(this,
                                        "Виберіть файл для валідації",
                                        ".", // Поточна директорія як початкова
                                        "XML files (*.xml);;All files (*)");
}

// Допоміжні функції для відображення результатів валідації (без змін)
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
    msgBox.setTextInteractionFlags(Qt::TextSelectableByMouse);
    msgBox.setIcon(result.isValid() ? QMessageBox::Information : QMessageBox::Warning);
    if (!result.isWellFormed) msgBox.setIcon(QMessageBox::Critical);
    msgBox.exec();
}
