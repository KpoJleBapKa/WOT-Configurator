#include "settingdelegate.h"
#include "main.h" // Для SettingRule

#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QDebug>
#include <limits>
#include <string>
#include <QLocale> // Для QLocale::c()

SettingDelegate::SettingDelegate(const SettingRulesMap* rules, QObject *parent)
    : QStyledItemDelegate(parent), m_rules(rules)
{
    if (!m_rules) {
        qWarning() << "SettingDelegate: Warning - Rules map provided is null!";
    }
}

QWidget *SettingDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                       const QModelIndex &index) const
{
    // Цей код створює редактори з правильними обмеженнями
    Q_UNUSED(option);
    if (index.column() != 1 || !m_rules) { return nullptr; }

    QString settingNameQ = index.siblingAtColumn(0).data(Qt::DisplayRole).toString();
    std::string settingName = settingNameQ.toStdString();
    SettingRule rule;
    auto it = m_rules->find(settingName);
    if (it != m_rules->end()) { rule = it->second; }

    switch (rule.type) {
    case SettingType::NON_EDITABLE:
        return nullptr; // Немає редактора
    case SettingType::BOOL_TF: {
        QComboBox *editor = new QComboBox(parent);
        editor->addItem("true"); editor->addItem("false");
        editor->setToolTip("Виберіть true або false");
        return editor;
    }
    case SettingType::BOOL_01: {
        QComboBox *editor = new QComboBox(parent);
        editor->addItem("1"); editor->addItem("0");
        editor->setToolTip("Виберіть 1 (увімкнено) або 0 (вимкнено)");
        return editor;
    }
    case SettingType::INT: {
        QSpinBox *editor = new QSpinBox(parent);
        int minInt = (rule.minValue > static_cast<double>(std::numeric_limits<int>::min())) ? static_cast<int>(rule.minValue) : std::numeric_limits<int>::min();
        int maxInt = (rule.maxValue < static_cast<double>(std::numeric_limits<int>::max())) ? static_cast<int>(rule.maxValue) : std::numeric_limits<int>::max();
        editor->setRange(minInt, maxInt); // <-- Обмеження діапазону
        editor->setToolTip(QString("Ціле число від %1 до %2").arg(minInt).arg(maxInt));
        editor->setLocale(QLocale::c()); // <-- Встановлюємо C локаль
        return editor;
    }
    case SettingType::FLOAT: {
        QDoubleSpinBox *editor = new QDoubleSpinBox(parent);
        editor->setDecimals(rule.decimals);
        editor->setRange(rule.minValue, rule.maxValue); // <-- Обмеження діапазону
        editor->setToolTip(QString("Число від %1 до %2").arg(rule.minValue, 0, 'g', rule.decimals+2).arg(rule.maxValue, 0, 'g', rule.decimals+2));
        editor->setSingleStep(0.01);
        editor->setLocale(QLocale::c()); // <-- Встановлюємо C локаль
        return editor;
    }
    case SettingType::STRING:
    default: {
        QLineEdit *editor = new QLineEdit(parent);
        editor->setToolTip("Рядкове значення");
        // Тут НЕ встановлюємо валідатор, щоб дозволити будь-який текст
        return editor;
    }
    }
}

// Встановлення початкового значення редактора (з trimmed і C локаллю для чисел)
void SettingDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QString valueStr = index.model()->data(index, Qt::EditRole).toString();
    QLocale cLocale = QLocale::c();

    if (QComboBox *comboBox = qobject_cast<QComboBox*>(editor)) {
        comboBox->setCurrentText(valueStr);
    } else if (QSpinBox *spinBox = qobject_cast<QSpinBox*>(editor)) {
        bool ok;
        // Читаємо значення з C локаллю, попередньо обрізавши пробіли
        int val = cLocale.toInt(valueStr.trimmed(), &ok);
        spinBox->setValue(ok ? val : spinBox->minimum());
    } else if (QDoubleSpinBox *doubleSpinBox = qobject_cast<QDoubleSpinBox*>(editor)) {
        bool ok;
        QString trimmedValue = valueStr.trimmed();
        // Замінюємо кому на крапку, якщо вона є, перед парсингом з C локаллю
        double val = cLocale.toDouble(trimmedValue.replace(',', '.'), &ok);
        doubleSpinBox->setValue(ok ? val : doubleSpinBox->minimum());
    } else if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(editor)) {
        lineEdit->setText(valueStr); // Рядки не обрізаємо при показі
    } else {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

// Запис даних з редактора в модель БЕЗ ВАЛІДАЦІЇ
void SettingDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QString newValueStr;
    QLocale cLocale = QLocale::c();
    // Правило потрібне лише для форматування float
    QString settingNameQ = index.siblingAtColumn(0).data(Qt::DisplayRole).toString();
    std::string settingName = settingNameQ.toStdString();
    SettingRule rule;
    if(m_rules) { auto it = m_rules->find(settingName); if (it != m_rules->end()) rule = it->second; }


    // Отримуємо значення з редактора
    if (QComboBox *comboBox = qobject_cast<QComboBox*>(editor)) {
        newValueStr = comboBox->currentText();
    } else if (QSpinBox *spinBox = qobject_cast<QSpinBox*>(editor)) {
        spinBox->interpretText(); // Перетворює текст на число, якщо можливо
        newValueStr = cLocale.toString(spinBox->value()); // Записуємо числове значення
    } else if (QDoubleSpinBox *doubleSpinBox = qobject_cast<QDoubleSpinBox*>(editor)) {
        doubleSpinBox->interpretText();
        newValueStr = cLocale.toString(doubleSpinBox->value(), 'f', rule.decimals); // Записуємо числове значення
    } else if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(editor)) {
        newValueStr = lineEdit->text();
    } else {
        QStyledItemDelegate::setModelData(editor, model, index);
        return;
    }

    // Просто записуємо отримане значення в модель (без validateValue)
    // За бажанням, можна додати trim() для числових/булевих типів перед записом
    if (rule.type == SettingType::INT || rule.type == SettingType::FLOAT || rule.type == SettingType::BOOL_01 || rule.type == SettingType::BOOL_TF) {
        model->setData(index, newValueStr.trimmed(), Qt::EditRole);
    } else {
        model->setData(index, newValueStr, Qt::EditRole);
    }
}

void SettingDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);
    editor->setGeometry(option.rect);
}

// Функція validateValue ВИДАЛЕНА з цього файлу
