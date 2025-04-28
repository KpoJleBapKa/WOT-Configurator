#ifndef MAIN_LOGIC_H
#define MAIN_LOGIC_H

#include <string>
#include <vector>
#include <filesystem> // Для fs::path
#include <stdexcept>  // Для std::runtime_error
#include <map>        // Для std::map
#include <utility>    // Для std::pair
#include <limits>     // Для std::numeric_limits

// Використовуємо простір імен filesystem
namespace fs = std::filesystem;

// --- Структура для Правил Валідації ---
enum class SettingType {
    STRING,         // Звичайний рядок (редагується через QLineEdit)
    INT,            // Ціле число (QSpinBox)
    FLOAT,          // Число з плаваючою комою (QDoubleSpinBox)
    BOOL_TF,        // Логічне (true/false, QComboBox)
    BOOL_01,        // Логічне (0/1, QComboBox)
    NON_EDITABLE    // Нередаговане поле
};

struct SettingRule {
    SettingType type = SettingType::STRING; // Тип за замовчуванням
    double minValue = -std::numeric_limits<double>::max(); // Використовуємо double для універсальності min/max
    double maxValue = std::numeric_limits<double>::max();
    int decimals = 6; // Кількість знаків після коми для FLOAT (QDoubleSpinBox)

    // Конструктор за замовчуванням (для STRING)
    SettingRule() : type(SettingType::STRING) {}

    // Конструктор для типів БЕЗ діапазону (BOOL_TF, BOOL_01, NON_EDITABLE, STRING)
    explicit SettingRule(SettingType t) : type(t) {}

    // Конструктор для типів З діапазоном (INT, FLOAT)
    SettingRule(SettingType t, double min, double max = std::numeric_limits<double>::max(), int dec = 6)
        : type(t), minValue(min), maxValue(max), decimals(dec) {
        if (t != SettingType::INT && t != SettingType::FLOAT) { /* Попередження */ }
        if (t == SettingType::INT) decimals = 0;
    }
};
// --- Кінець визначення SettingRule ---

// Тип для передачі даних між діалогом та ConfigEditor (без змін)
using FilteredSettingsMap = std::map<std::string, std::vector<std::pair<std::string, std::string>>>;


// --- Оголошення Класів Логіки ---

// FileValidator (з ValidationResult)
#ifndef FILEVALIDATOR_H
#define FILEVALIDATOR_H
namespace pugi { class xml_document; }
struct ValidationResult {
    bool isWellFormed = false; std::string wellFormedError = "";
    bool hasStructure = false; std::string structureInfo = "";
    std::vector<std::string> valueErrors;
    bool isValid() const { return isWellFormed; }
};
class FileValidator {
public:
    ValidationResult validateFile(const fs::path& filePath);
    bool validateBeforeAction(const fs::path& filePath, const std::string& actionNameStd, bool showSuccess = false);
private:
    bool isXmlWellFormedInternal(const fs::path& filePath, pugi::xml_document& doc, std::string& errorMsg);
    bool hasExpectedStructureInternal(const pugi::xml_document& doc, std::string& warnings);
    std::vector<std::string> findInvalidSimpleValuesInternal(const pugi::xml_document& doc);
};
#endif // FILEVALIDATOR_H

// ConfigEditor (з saveFilteredSettings)
#ifndef CONFIGEDITOR_H
#define CONFIGEDITOR_H
class ConfigEditor {
public:
    std::string readConfigContent(const fs::path& configPath);
    FilteredSettingsMap getFilteredSettings(const fs::path& configPath);
    void saveFilteredSettings(const fs::path& configPath, const FilteredSettingsMap& settings);
};
#endif // CONFIGEDITOR_H

// --- Решта оголошень класів (AppInitializer, BackupManager, ChangeTracker, ConfigManager, ProfileManager) ---
// (Скопіюй їх сюди з попередніх версій main.h)
#ifndef APPINITIALIZER_H
#define APPINITIALIZER_H
class AppInitializer { public: void loadInitialSettings(); void checkFolders(); void initializeComponents(); };
#endif // APPINITIALIZER_H
#ifndef BACKUPMANAGER_H
#define BACKUPMANAGER_H
class BackupManager { private: FileValidator m_validator; fs::path getGameConfigPath(); public: fs::path createBackup(); void restoreFromBackup(const fs::path& backupPath); void manageBackupSpace(); };
#endif // BACKUPMANAGER_H
#ifndef CHANGETRACKER_H
#define CHANGETRACKER_H
class ChangeTracker { public: void logAction(const std::string& functionName, bool success, const std::string& details = ""); };
#endif // CHANGETRACKER_H
#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H
class ConfigManager { private: FileValidator m_validator; fs::path getGameConfigPathInternal(); public: void changeCurrentConfig(const fs::path& sourceConfigPath); fs::path getCurrentGameConfigPath(); std::string viewCurrentGameConfigContent(); void uploadConfig(); };
#endif // CONFIGMANAGER_H
#ifndef PROFILEMANAGER_H
#define PROFILEMANAGER_H
class ProfileManager { public: void setName(const std::string& name); std::string loadNameFromFile(); private: void saveNameToFile(const std::string& name); };
#endif // PROFILEMANAGER_H

#endif // MAIN_LOGIC_H
