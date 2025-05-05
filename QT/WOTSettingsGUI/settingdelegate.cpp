#include "settingdelegate.h"
#include "main.h" // Для SettingRule

#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QDebug>
#include <limits>
#include <string>
#include <QLocale> // Для QLocale::c() та системної локалі

SettingDelegate::SettingDelegate(const SettingRulesMap* rules, QObject *parent)
    : QStyledItemDelegate(parent), m_rules(rules)
{
    if (!m_rules) {
        qWarning() << "SettingDelegate: Warning - Rules map provided is null!";
    }
}

// --- Створення редактора ---
QWidget *SettingDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                       const QModelIndex &index) const
{
    Q_UNUSED(option);
    if (index.column() != 1 || !m_rules) { return nullptr; }

    QString settingNameQ = index.siblingAtColumn(0).data(Qt::UserRole).toString();
    if (settingNameQ.isEmpty()) {
        settingNameQ = index.siblingAtColumn(0).data(Qt::DisplayRole).toString();
        qWarning() << "SettingDelegate::createEditor: Could not get key from UserRole for" << settingNameQ << ". Using display text.";
    }
    std::string settingName = settingNameQ.toStdString();

    SettingRule rule;
    auto it = m_rules->find(settingName);
    if (it != m_rules->end()) {
        rule = it->second;
    } else {
        qWarning() << "SettingDelegate::createEditor: No rule found for setting" << settingNameQ << ". Treating as STRING.";
        rule.type = SettingType::STRING;
    }

    switch (rule.type) {
    case SettingType::NON_EDITABLE:
        return nullptr;

    case SettingType::BOOL_TF: {
        QComboBox *editor = new QComboBox(parent);
        editor->addItem("true");
        editor->addItem("false");
        editor->setToolTip("Виберіть true або false");
        return editor;
    }
    case SettingType::BOOL_01: {
        QComboBox *editor = new QComboBox(parent);
        editor->addItem("1");
        editor->addItem("0");
        editor->setToolTip("Виберіть 1 (увімкнено) або 0 (вимкнено)");
        return editor;
    }
    case SettingType::INT: {
        QSpinBox *editor = new QSpinBox(parent);
        int minInt = (rule.minValue > static_cast<double>(std::numeric_limits<int>::min()))
                         ? static_cast<int>(rule.minValue) : std::numeric_limits<int>::min();
        int maxInt = (rule.maxValue < static_cast<double>(std::numeric_limits<int>::max()))
                         ? static_cast<int>(rule.maxValue) : std::numeric_limits<int>::max();
        editor->setRange(minInt, maxInt);
        editor->setToolTip(QString("Ціле число від %1 до %2").arg(minInt).arg(maxInt));
        editor->setLocale(QLocale::system());
        return editor;
    }
    case SettingType::FLOAT: {
        QDoubleSpinBox *editor = new QDoubleSpinBox(parent);
        editor->setDecimals(rule.decimals);
        editor->setRange(rule.minValue, rule.maxValue);
        editor->setToolTip(QString("Число від %L1 до %L2 (%3 зн.)")
                               .arg(rule.minValue)
                               .arg(rule.maxValue)
                               .arg(rule.decimals));
        if (rule.maxValue - rule.minValue < 10.0) editor->setSingleStep(0.01);
        else editor->setSingleStep(0.1);

        editor->setLocale(QLocale::system()); // <-- Встановлюємо системну локаль для відображення/введення
        return editor;
    }
    case SettingType::STRING:
    default: {
        QLineEdit *editor = new QLineEdit(parent);
        editor->setToolTip("Рядкове значення");
        return editor;
    }
    }
}

// --- Встановлення даних у редактор ---
void SettingDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QString valueStr = index.model()->data(index, Qt::EditRole).toString().trimmed(); // Обрізаємо пробіли одразу
    QLocale cLocale = QLocale::c(); // Локаль, що очікує крапку (.) як роздільник

    if (QComboBox *comboBox = qobject_cast<QComboBox*>(editor)) {
        int comboIndex = comboBox->findText(valueStr);
        if (comboIndex != -1) {
            comboBox->setCurrentIndex(comboIndex);
        } else {
            if (comboBox->count() > 0) comboBox->setCurrentIndex(0);
            qWarning() << "SettingDelegate::setEditorData: Value" << valueStr << "not found in ComboBox for" << index.siblingAtColumn(0).data(Qt::UserRole).toString();
        }
    } else if (QSpinBox *spinBox = qobject_cast<QSpinBox*>(editor)) {
        bool ok;
        // Пробуємо прочитати як ціле число, використовуючи C-локаль (менш залежно від системи)
        int val = cLocale.toInt(valueStr, &ok);
        spinBox->setValue(ok ? val : spinBox->minimum()); // Встановлюємо значення (SpinBox сам обмежить)
    } else if (QDoubleSpinBox *doubleSpinBox = qobject_cast<QDoubleSpinBox*>(editor)) {
        bool ok;
        // Пробуємо прочитати як double, використовуючи C-локаль (очікує крапку)
        // Додатково замінюємо кому на крапку про всяк випадок
        QString preparedValueStr = valueStr;
        preparedValueStr.replace(',', '.');
        double val = cLocale.toDouble(preparedValueStr, &ok);
        doubleSpinBox->setValue(ok ? val : doubleSpinBox->minimum()); // Встановлюємо значення (DoubleSpinBox сам обмежить)
    } else if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(editor)) {
        lineEdit->setText(valueStr); // Для рядків просто встановлюємо текст
    } else {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}


// --- Збереження даних з редактора в модель ---
void SettingDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QString newValueStr;
    QLocale systemLocale = QLocale::system();
    QLocale cLocale = QLocale::c();

    QString settingNameQ = index.siblingAtColumn(0).data(Qt::UserRole).toString();
    std::string settingName = settingNameQ.toStdString();
    SettingRule rule;
    if(m_rules){ auto it = m_rules->find(settingName); if(it != m_rules->end()) rule = it->second; }


    if (QComboBox *comboBox = qobject_cast<QComboBox*>(editor)) {
        newValueStr = comboBox->currentText();
    } else if (QSpinBox *spinBox = qobject_cast<QSpinBox*>(editor)) {
        // Використовуємо системну локаль для конвертації числа в рядок для показу
        newValueStr = systemLocale.toString(spinBox->value());
    } else if (QDoubleSpinBox *doubleSpinBox = qobject_cast<QDoubleSpinBox*>(editor)) {
        // Використовуємо C-локаль для конвертації float/double в рядок з крапкою і потрібною точністю
        newValueStr = cLocale.toString(doubleSpinBox->value(), 'f', rule.decimals);
    } else if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(editor)) {
        newValueStr = lineEdit->text();
    } else {
        QStyledItemDelegate::setModelData(editor, model, index);
        return;
    }

    if (rule.type == SettingType::INT || rule.type == SettingType::FLOAT || rule.type == SettingType::BOOL_01 || rule.type == SettingType::BOOL_TF) {
        model->setData(index, newValueStr.trimmed(), Qt::EditRole);
    } else {
        model->setData(index, newValueStr, Qt::EditRole);
    }
}

// --- Геометрія редактора ---
void SettingDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);
    editor->setGeometry(option.rect);
}
