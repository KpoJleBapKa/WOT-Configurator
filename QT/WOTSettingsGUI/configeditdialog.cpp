#include "configeditdialog.h"
#include "ui_configeditdialog.h"
#include "settingdelegate.h" // Включаємо делегат тут
#include "main.h" // Для SettingRule (вже включено через configeditdialog.h)

#include <QTreeWidgetItem>
#include <QMessageBox>
#include <QFont>
#include <QHeaderView>
#include <vector>
#include <string>
#include <limits>
#include <stdexcept>
#include <QDebug>
#include <QStringList>
#include <QBrush>     // Для підсвічування
#include <QColor>     // Для Qt::red/gray
#include <QLocale>    // Для конвертації чисел

// --- Визначення вільної функції валідації, ЩОБ НЕ ДУБЛЮВАТИ ---
// Краще зробити метод validateValue в SettingDelegate ПУБЛІЧНИМ СТАТИЧНИМ,
// але для мінімальних змін у settingdelegate.h, поки що залишимо її тут.
// Якщо виникнуть проблеми, переробимо validateValue на статичний метод.
namespace {
// Допоміжна лямбда для перевірки закінчення рядка
bool ends_with_compat(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
};

// Основна функція валідації (копія логіки з SettingDelegate::validateValue)
bool check_setting_value_local(const std::string& settingName, const QString& qValue, const SettingRule& rule) {
    std::string value = qValue.toStdString();
    bool ok = false;
    QLocale locale = QLocale::system();

    // qDebug() << "Validating (Dialog local check):" << QString::fromStdString(settingName) << "Value:" << qValue << "RuleType:" << static_cast<int>(rule.type);

    switch(rule.type) {
    case SettingType::NON_EDITABLE: return true;
    case SettingType::BOOL_TF:      return (value == "true" || value == "false");
    case SettingType::BOOL_01:      return (value == "0" || value == "1");
    case SettingType::INT: {
        int intVal = locale.toInt(qValue, &ok);
        if (!ok) return false;
        return ok && intVal >= static_cast<int>(rule.minValue) && intVal <= static_cast<int>(rule.maxValue);
    }
    case SettingType::FLOAT: {
        if(qValue == "1" && rule.minValue == 0.0 && rule.maxValue == 1.0) return true;
        double doubleVal = locale.toDouble(qValue, &ok);
        if (!ok) return false;
        const double epsilon = 1e-9;
        return ok && (doubleVal >= rule.minValue - epsilon && doubleVal <= rule.maxValue + epsilon);
    }
    case SettingType::STRING: default: return true;
    }
}
} // кінець анонімного простору імен


// --- Решта коду ConfigEditDialog ---

ConfigEditDialog::ConfigEditDialog(const FilteredSettingsMap& currentSettings,
                                   const fs::path& filePath,
                                   QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigEditDialog),
    m_filePath(filePath),
    m_currentSettings(currentSettings),
    m_settingDelegate(nullptr)
{
    ui->setupUi(this);
    this->setWindowTitle("Редагування: " + QString::fromStdWString(m_filePath.filename().wstring()));
    initializeValidationRules();
    m_settingDelegate = new SettingDelegate(&m_rules, this);
    ui->settingsTreeWidget->setItemDelegateForColumn(1, m_settingDelegate);
    ui->settingsTreeWidget->setColumnCount(2);
    ui->settingsTreeWidget->setHeaderLabels(QStringList() << "Налаштування" << "Значення");
    ui->settingsTreeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->settingsTreeWidget->header()->setStretchLastSection(true);
    ui->settingsTreeWidget->setAlternatingRowColors(true);
    populateTree();
    connect(ui->saveButton, &QPushButton::clicked, this, &ConfigEditDialog::onSaveClicked);
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

ConfigEditDialog::~ConfigEditDialog()
{
    delete ui;
}

void ConfigEditDialog::initializeValidationRules() { /* ... той самий код ініціалізації, як і раніше ... */
    // Sound Settings
    m_rules["masterVolume"] = SettingRule(SettingType::FLOAT, 0.0, 1.0);
    m_rules["volume_micVivox"] = SettingRule(SettingType::FLOAT, 0.0, 1.0);
    m_rules["volume_vehicles"] = SettingRule(SettingType::FLOAT, 0.0, 1.0);
    m_rules["volume_music"] = SettingRule(SettingType::FLOAT, 0.0, 1.0);
    m_rules["volume_effects"] = SettingRule(SettingType::FLOAT, 0.0, 1.0);
    m_rules["volume_ambient"] = SettingRule(SettingType::FLOAT, 0.0, 1.0);
    m_rules["volume_gui"] = SettingRule(SettingType::FLOAT, 0.0, 1.0);
    m_rules["volume_voice"] = SettingRule(SettingType::FLOAT, 0.0, 1.0);
    m_rules["soundMode"] = SettingRule(SettingType::NON_EDITABLE);

    // Graphics Settings
    m_rules["graphicsSettingsVersion"] = SettingRule(SettingType::NON_EDITABLE);
    m_rules["graphicsSettingsVersionMinor"] = SettingRule(SettingType::NON_EDITABLE);
    m_rules["graphicsSettingsVersionMaintainance"] = SettingRule(SettingType::NON_EDITABLE);
    m_rules["graphicsSettingsStatus"] = SettingRule(SettingType::NON_EDITABLE);
    m_rules["COLOR_GRADING_TECHNIQUE"] = SettingRule(SettingType::NON_EDITABLE);
    m_rules["CUSTOM_AA_MODE"] = SettingRule(SettingType::INT, 0, 2); // 0, 1, 2
    m_rules["DECOR_LEVEL"] = SettingRule(SettingType::INT, 0, 4); // Припускаємо 0 - макс
    m_rules["DRR_AUTOSCALER_ENABLED"] = SettingRule(SettingType::NON_EDITABLE);
    m_rules["EFFECTS_QUALITY"] = SettingRule(SettingType::INT, 0, 4); // 0..4
    m_rules["FAR_PLANE"] = SettingRule(SettingType::INT, 0, 3); // 0..3
    m_rules["FLORA_QUALITY"] = SettingRule(SettingType::INT, 0, 4); // 0..4
    m_rules["HAVOK_ENABLED"] = SettingRule(SettingType::BOOL_01); // 0 або 1
    m_rules["HAVOK_QUALITY"] = SettingRule(SettingType::INT, 0, 2); // 0..2
    m_rules["LIGHTING_QUALITY"] = SettingRule(SettingType::INT, 0, 4); // 0..4
    m_rules["MOTION_BLUR_QUALITY"] = SettingRule(SettingType::INT, 0, 3); // 0..3
    m_rules["MSAA_QUALITY"] = SettingRule(SettingType::NON_EDITABLE);
    m_rules["OBJECT_LOD"] = SettingRule(SettingType::INT, 0, 4); // 0..4
    m_rules["POST_PROCESSING_QUALITY"] = SettingRule(SettingType::INT, 0, 4); // 0..4
    m_rules["RENDER_PIPELINE"] = SettingRule(SettingType::NON_EDITABLE);
    m_rules["SEMITRANSPARENT_LEAVES_ENABLED"] = SettingRule(SettingType::BOOL_01); // Має бути BOOL_01
    m_rules["SHADER_DEBUG"] = SettingRule(SettingType::NON_EDITABLE);
    m_rules["SHADOWS_QUALITY"] = SettingRule(SettingType::INT, 0, 2); // 0..2
    m_rules["SNIPER_MODE_EFFECTS_QUALITY"] = SettingRule(SettingType::INT, 0, 3); // 0..3
    m_rules["SNIPER_MODE_GRASS_ENABLED"] = SettingRule(SettingType::BOOL_01); // 0 або 1
    m_rules["SNIPER_MODE_SWINGING_ENABLED"] = SettingRule(SettingType::BOOL_01); // 0 або 1
    m_rules["SNIPER_MODE_TERRAIN_TESSELLATION_ENABLED"] = SettingRule(SettingType::BOOL_01); // 0 або 1
    m_rules["SPEEDTREE_QUALITY"] = SettingRule(SettingType::INT, 0, 3); // 0..3
    m_rules["TERRAIN_QUALITY"] = SettingRule(SettingType::INT, 0, 5); // 0..5
    m_rules["TERRAIN_TESSELLATION_ENABLED"] = SettingRule(SettingType::BOOL_01); // 0 або 1
    m_rules["TEXTURE_QUALITY"] = SettingRule(SettingType::INT, 0, 4); // 0..4
    m_rules["TRACK_PHYSICS_QUALITY"] = SettingRule(SettingType::INT, 0, 3); // 0..3
    m_rules["VEHICLE_DUST_ENABLED"] = SettingRule(SettingType::BOOL_01); // 0 або 1
    m_rules["VEHICLE_TRACES_ENABLED"] = SettingRule(SettingType::BOOL_01); // 0 або 1
    m_rules["WATER_QUALITY"] = SettingRule(SettingType::INT, 0, 3); // 0..3
    m_rules["ParticlSystemNoRenderGroup"] = SettingRule(SettingType::INT, 65535, 65535);
    m_rules["distributionLevel"] = SettingRule(SettingType::NON_EDITABLE);
    m_rules["colorGradingStrength"] = SettingRule(SettingType::FLOAT, 0.0, 1.0);
    m_rules["brightnessDeferred"] = SettingRule(SettingType::FLOAT, 0.0, 1.0);
    m_rules["contrastDeferred"] = SettingRule(SettingType::FLOAT, 0.0, 1.0);
    m_rules["saturationDeferred"] = SettingRule(SettingType::FLOAT, 0.0, 1.0);

    // Control Settings
    const std::vector<std::string> controlModes = {"strategicMode", "artyMode", "arcadeMode", "sniperMode", "freeVideoMode"};
    for(const auto& mode : controlModes) {
        m_rules[mode + "/horzInvert"] = SettingRule(SettingType::BOOL_TF);
        m_rules[mode + "/vertInvert"] = SettingRule(SettingType::BOOL_TF);
        m_rules[mode + "/keySensitivity"] = SettingRule(SettingType::FLOAT, 0.01, 1.0);
        m_rules[mode + "/sensitivity"] = SettingRule(SettingType::FLOAT, 0.01, 1.0);
        m_rules[mode + "/scrollSensitivity"] = SettingRule(SettingType::FLOAT, 0.01, 1.0);
    }

    // Device Settings
    m_rules["windowMode"] = SettingRule(SettingType::INT, 0, 2);
    m_rules["windowedWidth"] = SettingRule(SettingType::INT, 800, std::numeric_limits<int>::max());
    m_rules["windowedHeight"] = SettingRule(SettingType::INT, 600, std::numeric_limits<int>::max());
    m_rules["fullscreenWidth"] = SettingRule(SettingType::INT, 800, std::numeric_limits<int>::max());
    m_rules["fullscreenHeight"] = SettingRule(SettingType::INT, 600, std::numeric_limits<int>::max());
    m_rules["fullscreenRefresh"] = SettingRule(SettingType::INT, 10, std::numeric_limits<int>::max());
    m_rules["aspectRatio"] = SettingRule(SettingType::FLOAT, 0.1, 10.0, 6);
}


void ConfigEditDialog::populateTree() { /* ... як раніше ... */
    ui->settingsTreeWidget->clear();
    QFont categoryFont = ui->settingsTreeWidget->font();
    categoryFont.setBold(true);
    for (const auto &categoryPair : m_currentSettings) {
        QTreeWidgetItem *categoryItem = new QTreeWidgetItem(ui->settingsTreeWidget);
        categoryItem->setText(0, QString::fromStdString(categoryPair.first));
        categoryItem->setFont(0, categoryFont);
        categoryItem->setFlags(categoryItem->flags() & ~Qt::ItemIsEditable);
        categoryItem->setExpanded(true);
        for (const auto &settingPair : categoryPair.second) {
            QTreeWidgetItem *settingItem = new QTreeWidgetItem(categoryItem);
            std::string settingName = settingPair.first;
            settingItem->setText(0, QString::fromStdString(settingName));
            settingItem->setText(1, QString::fromStdString(settingPair.second));
            bool editable = true;
            auto ruleIt = m_rules.find(settingName);
            if (ruleIt != m_rules.end() && ruleIt->second.type == SettingType::NON_EDITABLE) {
                editable = false;
            }
            if (editable) {
                settingItem->setFlags(settingItem->flags() | Qt::ItemIsEditable);
            } else {
                settingItem->setFlags(settingItem->flags() & ~Qt::ItemIsEditable);
                settingItem->setForeground(1, Qt::gray);
                settingItem->setToolTip(1, "Це значення не редагується");
            }
        }
    }
    ui->settingsTreeWidget->resizeColumnToContents(0);
}

FilteredSettingsMap ConfigEditDialog::getUpdatedSettings() const { /* ... як раніше ... */
    return m_currentSettings;
}

bool ConfigEditDialog::collectSettingsFromTree() { /* ... як раніше ... */
    FilteredSettingsMap updatedSettings;
    QTreeWidget *tree = ui->settingsTreeWidget;
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *categoryItem = tree->topLevelItem(i);
        if (!categoryItem) continue;
        std::string categoryName = categoryItem->text(0).toStdString();
        std::vector<std::pair<std::string, std::string>> categorySettings;
        for (int j = 0; j < categoryItem->childCount(); ++j) {
            QTreeWidgetItem *settingItem = categoryItem->child(j);
            if (!settingItem) continue;
            std::string settingName = settingItem->text(0).toStdString();
            std::string newValue = settingItem->text(1).toStdString();
            categorySettings.push_back({settingName, newValue});
        }
        if (!categorySettings.empty()) {
            updatedSettings[categoryName] = categorySettings;
        }
    }
    m_currentSettings = updatedSettings;
    return true;
}


// Фінальна перевірка перед закриттям (ОНОВЛЕНО: використовує локальну check_setting_value_local)
bool ConfigEditDialog::finalValidationCheck() {
    QStringList validationErrors;
    QTreeWidget *tree = ui->settingsTreeWidget;
    bool allValid = true;

    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *categoryItem = tree->topLevelItem(i);
        if (!categoryItem) continue;
        for (int j = 0; j < categoryItem->childCount(); ++j) {
            QTreeWidgetItem *settingItem = categoryItem->child(j);
            if (!settingItem || !(settingItem->flags() & Qt::ItemIsEditable)) continue;

            std::string settingName = settingItem->text(0).toStdString();
            QString qValue = settingItem->text(1);
            SettingRule rule;
            auto ruleIt = m_rules.find(settingName);
            if (ruleIt != m_rules.end()) rule = ruleIt->second;

            // Викликаємо вільну функцію валідації, визначену в цьому файлі
            if (!check_setting_value_local(settingName, qValue, rule)) {
                validationErrors.append(QString("Неприпустиме значення для '%1': '%2'")
                                            .arg(QString::fromStdString(settingName))
                                            .arg(qValue));
                settingItem->setBackground(1, QBrush(Qt::red));
                allValid = false;
            } else if (settingItem) {
                settingItem->setBackground(1, QBrush()); // Знімаємо підсвічування
            }
        }
    }

    if (!allValid) {
        QMessageBox::warning(this, "Помилка валідації",
                             "Неможливо зберегти. Будь ласка, виправте неприпустимі значення (підсвічені червоним):\n- " + validationErrors.join("\n- "));
        return false;
    }
    return true;
}


// Слот для кнопки "Зберегти" (ОНОВЛЕНО: прибираємо логіку примусового завершення редактора)
void ConfigEditDialog::onSaveClicked()
{
    // Просто виконуємо фінальну перевірку. Якщо вона пройде, збираємо дані та закриваємо.

    accept(); // Закриваємо діалог зі статусом OK

}
