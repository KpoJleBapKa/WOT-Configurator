#ifndef SETTINGDELEGATE_H
#define SETTINGDELEGATE_H

#include <QStyledItemDelegate>
#include <map>
#include <string>
#include "main.h" // Для SettingRule

using SettingRulesMap = std::map<std::string, SettingRule>;

QT_BEGIN_NAMESPACE
class QWidget;
class QStyleOptionViewItem;
class QModelIndex;
class QAbstractItemModel;
class QLocale;
QT_END_NAMESPACE

class SettingDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit SettingDelegate(const SettingRulesMap* rules, QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;

    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override;

private:
    const SettingRulesMap* m_rules;
};

#endif // SETTINGDELEGATE_H
