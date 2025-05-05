#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include <QDialog>

// Попередні оголошення
QT_BEGIN_NAMESPACE
class QTextBrowser;
class QPushButton;
class QVBoxLayout;
QT_END_NAMESPACE

class HelpDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HelpDialog(QWidget *parent = nullptr);
    // Деструктор не потрібен для програмно створених віджетів (lol)

private:
    // Віджети
    QTextBrowser *helpTextBrowser;
    QPushButton *closeButton;

    // Функції налаштування
    void setupUi();
    void populateHelpText();
};

#endif // HELPDIALOG_H
