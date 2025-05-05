#ifndef CONFIGEDITDIALOG_H
#define CONFIGEDITDIALOG_H

#include <QDialog>
#include <map>
#include <string>
#include <filesystem>
#include "main.h" // Для FilteredSettingsMap та SettingRule
// #include "settingdelegate.h" // Не включаємо тут, щоб уникнути циклічної залежності, використаємо попереднє оголошення

namespace fs = std::filesystem;

QT_BEGIN_NAMESPACE
namespace Ui { class ConfigEditDialog; }
class QTreeWidgetItem;
class SettingDelegate; // <-- Попереднє оголошення делегата
class QBrush;          // <-- Попереднє оголошення QBrush
QT_END_NAMESPACE

using SettingRulesMap = std::map<std::string, SettingRule>;

class ConfigEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigEditDialog(const FilteredSettingsMap& currentSettings,
                              const fs::path& filePath,
                              QWidget *parent = nullptr);
    ~ConfigEditDialog();

    FilteredSettingsMap getUpdatedSettings() const;

private slots:
    void onSaveClicked();

private:
    Ui::ConfigEditDialog *ui;
    fs::path m_filePath;
    FilteredSettingsMap m_currentSettings; // Оновлюється при збереженні
    SettingRulesMap m_rules;           // Мапа з правилами валідації
    SettingDelegate *m_settingDelegate; // Вказівник на делегат

    // Допоміжні функції
    void initializeValidationRules(); // Ініціалізує мапу m_rules
    void populateTree();
    bool collectSettingsFromTree(); // Збирає дані з дерева БЕЗ валідації
    bool finalValidationCheck();    // <-- Фінальна перевірка перед збереженням
};

#endif // CONFIGEDITDIALOG_H
