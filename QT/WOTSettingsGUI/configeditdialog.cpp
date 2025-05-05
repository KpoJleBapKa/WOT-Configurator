#include "configeditdialog.h"
#include "ui_configeditdialog.h"
#include "settingdelegate.h"
#include "main.h"

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
#include <QBrush>
#include <QColor>
#include <QLocale>
#include <map>
#include <utility> // для std::move

// Includes для налаштування Layout
#include <QVBoxLayout>
#include <QSizePolicy>
#include <QSpacerItem>

// валідатіон
namespace {
bool ends_with_compat(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
};

bool check_setting_value_local(const std::string& settingName, const QString& qValue, const SettingRule& rule) {
    std::string value = qValue.toStdString();
    bool ok = false;
    QLocale locale = QLocale::system();

    switch(rule.type) {
    case SettingType::NON_EDITABLE: return true;
    case SettingType::BOOL_TF:      return (value == "true" || value == "false");
    case SettingType::BOOL_01:      return (value == "0" || value == "1");
    case SettingType::INT: {
        int intVal = locale.toInt(qValue.trimmed(), &ok);
        if (!ok) return false;
        int minInt = (rule.minValue > static_cast<double>(std::numeric_limits<int>::min())) ? static_cast<int>(rule.minValue) : std::numeric_limits<int>::min();
        int maxInt = (rule.maxValue < static_cast<double>(std::numeric_limits<int>::max())) ? static_cast<int>(rule.maxValue) : std::numeric_limits<int>::max();
        return ok && intVal >= minInt && intVal <= maxInt;
    }
    case SettingType::FLOAT: {
        QLocale cLocale = QLocale::c();
        QString valueToCheck = qValue.trimmed();
        valueToCheck.replace(',', '.');
        double doubleVal = cLocale.toDouble(valueToCheck, &ok);
        if (!ok) return false;

        if(qValue.trimmed() == "1" && rule.minValue == 0.0 && rule.maxValue == 1.0) return true;

        const double epsilon = 1e-9;
        return ok && (doubleVal >= rule.minValue - epsilon && doubleVal <= rule.maxValue + epsilon);
    }
    case SettingType::STRING: default: return true;
    }
}
} // кінець анонімного простору імен

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

    QSizePolicy treePolicy = ui->settingsTreeWidget->sizePolicy();
    treePolicy.setHorizontalPolicy(QSizePolicy::Expanding);
    treePolicy.setVerticalPolicy(QSizePolicy::Expanding);
    treePolicy.setHorizontalStretch(1);
    treePolicy.setVerticalStretch(1);
    ui->settingsTreeWidget->setSizePolicy(treePolicy);

    if (!this->layout()) {
        qWarning() << "ConfigEditDialog: No top-level layout found, creating default QVBoxLayout.";
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->addWidget(ui->settingsTreeWidget);
        if (ui->saveButton && ui->cancelButton) {
            QHBoxLayout *buttonLayout = new QHBoxLayout();
            buttonLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
            buttonLayout->addWidget(ui->saveButton);
            buttonLayout->addWidget(ui->cancelButton);
            mainLayout->addLayout(buttonLayout);
        }
        this->setLayout(mainLayout);
    }

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

    this->setMinimumSize(600, 450);

    connect(ui->saveButton, &QPushButton::clicked, this, &ConfigEditDialog::onSaveClicked);
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

ConfigEditDialog::~ConfigEditDialog()
{
    delete ui;
}

// Налаштування
void ConfigEditDialog::initializeValidationRules() {
    m_rules.clear();

    // --- Sound Settings ---
    m_rules["masterVolume"] = SettingRule(SettingType::FLOAT, 0.0, 1.0, "Загальна гучність");
    m_rules["volume_micVivox"] = SettingRule(SettingType::FLOAT, 0.0, 1.0, "Гучність мікрофону (Vivox)");
    m_rules["volume_vehicles"] = SettingRule(SettingType::FLOAT, 0.0, 1.0, "Гучність: Техніка");
    m_rules["volume_music"] = SettingRule(SettingType::FLOAT, 0.0, 1.0, "Гучність: Музика");
    m_rules["volume_effects"] = SettingRule(SettingType::FLOAT, 0.0, 1.0, "Гучність: Ефекти");
    m_rules["volume_ambient"] = SettingRule(SettingType::FLOAT, 0.0, 1.0, "Гучність: Оточення");
    m_rules["volume_gui"] = SettingRule(SettingType::FLOAT, 0.0, 1.0, "Гучність: Інтерфейс");
    m_rules["volume_voice"] = SettingRule(SettingType::FLOAT, 0.0, 1.0, "Гучність: Голосові повідомлення");
    m_rules["soundMode"] = SettingRule(SettingType::NON_EDITABLE, "Режим звуку");

    // --- Graphics Settings ---
    m_rules["graphicsSettingsVersion"] = SettingRule(SettingType::NON_EDITABLE, "Версія налаштувань графіки");
    m_rules["graphicsSettingsVersionMinor"] = SettingRule(SettingType::NON_EDITABLE, "Мінорна версія нал. графіки");
    m_rules["graphicsSettingsVersionMaintainance"] = SettingRule(SettingType::NON_EDITABLE, "Патч версія нал. графіки");
    m_rules["graphicsSettingsStatus"] = SettingRule(SettingType::NON_EDITABLE, "Статус налаштувань графіки");
    m_rules["COLOR_GRADING_TECHNIQUE"] = SettingRule(SettingType::NON_EDITABLE, "Техніка корекції кольору");
    m_rules["CUSTOM_AA_MODE"] = SettingRule(SettingType::INT, 0, 2, "Режим згладжування"); // 0: Вимк, 1: FXAA, 2: TAA (припущення)
    m_rules["DECOR_LEVEL"] = SettingRule(SettingType::INT, 0, 4, "Якість ефектів (дрібні об'єкти)"); // Наприклад: 0=Макс, 4=Вимк
    m_rules["DRR_AUTOSCALER_ENABLED"] = SettingRule(SettingType::NON_EDITABLE, "Динамічна роздільна здатність");
    m_rules["EFFECTS_QUALITY"] = SettingRule(SettingType::INT, 0, 4, "Якість ефектів (вибухи, дим)"); // Наприклад: 0=Макс, 4=Низька
    m_rules["FAR_PLANE"] = SettingRule(SettingType::INT, 0, 3, "Дальність промальовки"); // Наприклад: 0=Макс, 3=Низька
    m_rules["FLORA_QUALITY"] = SettingRule(SettingType::INT, 0, 4, "Якість трави та рослинності"); // Наприклад: 0=Макс, 4=Вимк
    m_rules["HAVOK_ENABLED"] = SettingRule(SettingType::BOOL_01, "Фізика руйнувань Havok");
    m_rules["HAVOK_QUALITY"] = SettingRule(SettingType::INT, 0, 2, "Якість фізики Havok"); // Наприклад: 0=Висока, 2=Низька
    m_rules["LIGHTING_QUALITY"] = SettingRule(SettingType::INT, 0, 4, "Якість освітлення"); // Наприклад: 0=Макс, 4=Низька
    m_rules["MOTION_BLUR_QUALITY"] = SettingRule(SettingType::INT, 0, 3, "Якість розмиття в русі"); // Наприклад: 0=Макс, 3=Вимк
    m_rules["MSAA_QUALITY"] = SettingRule(SettingType::NON_EDITABLE, "Якість MSAA");
    m_rules["OBJECT_LOD"] = SettingRule(SettingType::INT, 0, 4, "Деталізація об'єктів"); // Наприклад: 0=Макс, 4=Низька
    m_rules["POST_PROCESSING_QUALITY"] = SettingRule(SettingType::INT, 0, 4, "Якість постобробки"); // Наприклад: 0=Макс, 4=Низька
    m_rules["RENDER_PIPELINE"] = SettingRule(SettingType::NON_EDITABLE, "Тип рендеру"); // 0 = DX11, 1 = ?
    m_rules["SEMITRANSPARENT_LEAVES_ENABLED"] = SettingRule(SettingType::BOOL_01, "Прозорість листя");
    m_rules["SHADER_DEBUG"] = SettingRule(SettingType::NON_EDITABLE, "Відладка шейдерів");
    m_rules["SHADOWS_QUALITY"] = SettingRule(SettingType::INT, 0, 2, "Якість тіней"); // Наприклад: 0=Макс, 2=Низька
    m_rules["SNIPER_MODE_EFFECTS_QUALITY"] = SettingRule(SettingType::INT, 0, 3, "Якість ефектів (снайп. режим)"); // Наприклад: 0=Макс, 3=Низька
    m_rules["SNIPER_MODE_GRASS_ENABLED"] = SettingRule(SettingType::BOOL_01, "Трава в снайп. режимі");
    m_rules["SNIPER_MODE_SWINGING_ENABLED"] = SettingRule(SettingType::BOOL_01, "Хитання камери (снайп. режим)");
    m_rules["SNIPER_MODE_TERRAIN_TESSELLATION_ENABLED"] = SettingRule(SettingType::BOOL_01, "Теселяція ландшафту (снайп. режим)");
    m_rules["SPEEDTREE_QUALITY"] = SettingRule(SettingType::INT, 0, 3, "Якість дерев"); // Наприклад: 0=Макс, 3=Низька
    m_rules["TERRAIN_QUALITY"] = SettingRule(SettingType::INT, 0, 5, "Якість ландшафту"); // Наприклад: 0=Макс, 5=Низька
    m_rules["TERRAIN_TESSELLATION_ENABLED"] = SettingRule(SettingType::BOOL_01, "Теселяція ландшафту");
    m_rules["TEXTURE_QUALITY"] = SettingRule(SettingType::INT, 0, 4, "Якість текстур"); // Наприклад: 0=Макс, 4=Низька
    m_rules["TRACK_PHYSICS_QUALITY"] = SettingRule(SettingType::INT, 0, 3, "Фізика гусениць"); // Наприклад: 0=Вимк, 1=Вкл, 2=Вкл+Ефекти?
    m_rules["VEHICLE_DUST_ENABLED"] = SettingRule(SettingType::BOOL_01, "Пил з-під техніки");
    m_rules["VEHICLE_TRACES_ENABLED"] = SettingRule(SettingType::BOOL_01, "Сліди техніки");
    m_rules["WATER_QUALITY"] = SettingRule(SettingType::INT, 0, 3, "Якість води"); // Наприклад: 0=Макс, 3=Низька
    m_rules["ParticlSystemNoRenderGroup"] = SettingRule(SettingType::INT, 65535, 65535, "Група невидимих часток");
    m_rules["distributionLevel"] = SettingRule(SettingType::NON_EDITABLE, "Рівень дистрибуції");
    m_rules["colorGradingStrength"] = SettingRule(SettingType::FLOAT, 0.0, 1.0, "Сила корекції кольору");
    m_rules["brightnessDeferred"] = SettingRule(SettingType::FLOAT, 0.0, 1.0, "Яскравість");
    m_rules["contrastDeferred"] = SettingRule(SettingType::FLOAT, 0.0, 1.0, "Контрастність");
    m_rules["saturationDeferred"] = SettingRule(SettingType::FLOAT, 0.0, 1.0, "Насиченість");

    // --- Control Settings ---
    const std::vector<std::string> controlModes = {"strategicMode", "artyMode", "arcadeMode", "sniperMode", "freeVideoMode"};
    std::map<std::string, std::string> modeNamesUKR = {
        {"strategicMode", "Стратегічний"}, {"artyMode", "Арт-САУ"},
        {"arcadeMode", "Аркадний"}, {"sniperMode", "Снайперський"},
        {"freeVideoMode", "Вільна камера"}
    };
    for(const auto& mode : controlModes) {
        std::string modePrefix = modeNamesUKR.count(mode) ? modeNamesUKR[mode] + ": " : mode + ": ";
        const int floatPrecision = 2;
        m_rules[mode + "/horzInvert"] = SettingRule(SettingType::BOOL_TF, modePrefix + "Інверсія по горизонталі");
        m_rules[mode + "/vertInvert"] = SettingRule(SettingType::BOOL_TF, modePrefix + "Інверсія по вертикалі");
        m_rules[mode + "/keySensitivity"] = SettingRule(SettingType::FLOAT, 0.01, 1.0, floatPrecision, modePrefix + "Чутливість клавіатури");
        m_rules[mode + "/sensitivity"] = SettingRule(SettingType::FLOAT, 0.01, 1.0, floatPrecision, modePrefix + "Чутливість миші");
        m_rules[mode + "/scrollSensitivity"] = SettingRule(SettingType::FLOAT, 0.01, 1.0, floatPrecision, modePrefix + "Чутливість прокрутки");
    }

    // --- Device Settings ---
    m_rules["windowMode"] = SettingRule(SettingType::INT, 0, 2, "Режим вікна");
    m_rules["windowedWidth"] = SettingRule(SettingType::INT, 800, 7680, "Ширина (вікно)");
    m_rules["windowedHeight"] = SettingRule(SettingType::INT, 600, 4320, "Висота (вікно)");
    m_rules["fullscreenWidth"] = SettingRule(SettingType::INT, 800, 7680, "Ширина (повний екран)");
    m_rules["fullscreenHeight"] = SettingRule(SettingType::INT, 600, 4320, "Висота (повний екран)");
    m_rules["fullscreenRefresh"] = SettingRule(SettingType::INT, 10, 400, "Частота оновлення");
    m_rules["aspectRatio"] = SettingRule(SettingType::FLOAT, 0.1, 10.0, 6, "Співвідношення сторін");
    m_rules["gamma"] = SettingRule(SettingType::FLOAT, 0.5, 2.5, 2, "Гамма");
    m_rules["tripleBuffering"] = SettingRule(SettingType::BOOL_TF, "Потрійна буферизація");
}

// populateTree
void ConfigEditDialog::populateTree() {
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
            std::string settingName = settingPair.first;
            QString displayNameToShow = QString::fromStdString(settingName);
            QString displayValue = QString::fromStdString(settingPair.second);
            bool editable = true;
            QString tooltip = "";

            auto ruleIt = m_rules.find(settingName);
            if (ruleIt != m_rules.end()) {
                if (!ruleIt->second.displayName.empty()) {
                    displayNameToShow = QString::fromStdString(ruleIt->second.displayName);
                }
                if (ruleIt->second.type == SettingType::NON_EDITABLE) {
                    editable = false;
                    tooltip = "Це значення не редагується";
                } else {
                    switch(ruleIt->second.type) {
                    case SettingType::INT: tooltip = QString("Ціле число від %1 до %2").arg(static_cast<int>(ruleIt->second.minValue)).arg(static_cast<int>(ruleIt->second.maxValue)); break;
                    case SettingType::FLOAT: tooltip = QString("Число від %L1 до %L2 (%3 зн.)").arg(ruleIt->second.minValue, 0, 'f', ruleIt->second.decimals).arg(ruleIt->second.maxValue, 0, 'f', ruleIt->second.decimals).arg(ruleIt->second.decimals); break;
                    case SettingType::BOOL_TF: tooltip = "Виберіть true або false"; break;
                    case SettingType::BOOL_01: tooltip = "Виберіть 1 (увімк.) або 0 (вимк.)"; break;
                    default: tooltip = "Рядкове значення"; break;
                    }
                }
            } else {
                tooltip = "Рядкове значення (правило не визначено)";
            }

            QTreeWidgetItem *settingItem = new QTreeWidgetItem(categoryItem);
            settingItem->setText(0, displayNameToShow);
            settingItem->setText(1, displayValue);
            settingItem->setData(0, Qt::UserRole, QString::fromStdString(settingName));
            settingItem->setToolTip(1, tooltip);

            if (editable) {
                settingItem->setFlags(settingItem->flags() | Qt::ItemIsEditable);
            } else {
                settingItem->setFlags(settingItem->flags() & ~Qt::ItemIsEditable);
                settingItem->setForeground(1, Qt::gray);
            }
        }
    }
    ui->settingsTreeWidget->resizeColumnToContents(0);
}

FilteredSettingsMap ConfigEditDialog::getUpdatedSettings() const {
    return m_currentSettings;
}

bool ConfigEditDialog::collectSettingsFromTree() {
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

            QString originalKeyQStr = settingItem->data(0, Qt::UserRole).toString();
            std::string settingName = originalKeyQStr.toStdString();

            if (settingName.empty()) {
                qWarning() << "Collect Settings: Could not retrieve original setting key from UserRole for displayed name:" << settingItem->text(0);
                continue;
            }

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

            QString originalKeyQStr = settingItem->data(0, Qt::UserRole).toString();
            std::string settingName = originalKeyQStr.toStdString();

            if (settingName.empty()) {
                qWarning() << "Validation Check: Could not retrieve original setting key from UserRole for displayed name:" << settingItem->text(0);
                continue;
            }

            QString qValue = settingItem->text(1);
            SettingRule rule;
            auto ruleIt = m_rules.find(settingName);
            if (ruleIt != m_rules.end()) rule = ruleIt->second;

            if (!check_setting_value_local(settingName, qValue, rule)) {
                QString displayNameToShow = settingItem->text(0);
                validationErrors.append(QString("Неприпустиме значення для '%1': '%2'")
                                            .arg(displayNameToShow)
                                            .arg(qValue));
                settingItem->setBackground(1, QBrush(Qt::red));
                allValid = false;
            } else {
                if (settingItem->background(1).color() == Qt::red) {
                    settingItem->setBackground(1, QBrush());
                }
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

void ConfigEditDialog::onSaveClicked()
{
    if (!collectSettingsFromTree()) {
        QMessageBox::critical(this, "Помилка", "Не вдалося зібрати дані з дерева налаштувань.");
        return;
    }

    if (finalValidationCheck()) {
        accept();
    }
}
