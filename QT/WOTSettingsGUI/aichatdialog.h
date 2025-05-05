#ifndef AICHATDIALOG_H
#define AICHATDIALOG_H

#include <QDialog>
#include <QtNetwork/QNetworkAccessManager>
#include <QString>
#include <QJsonObject>

// Попередні оголошення класів Qt
QT_BEGIN_NAMESPACE
class QTextBrowser;
class QLineEdit;
class QPushButton;
class QLabel;
class QNetworkReply;
namespace Ui { class AIChatDialog; }
QT_END_NAMESPACE

class AIChatDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AIChatDialog(QWidget *parent = nullptr);
    ~AIChatDialog();

private slots:
    void onSendButtonClicked();
    void onNetworkReply(QNetworkReply *reply);

private:
    // UI елементи
    QTextBrowser *chatDisplay;
    QLineEdit *messageInput;
    QPushButton *sendButton;
    QPushButton *closeButton;
    QLabel *statusLabel;

    // Мережеві компоненти
    QNetworkAccessManager *networkManager;
    QString m_apiKey;
    QString m_apiUrl;
    QString m_basePrompt;

    // Приватні функції
    void setupUi();
    void sendMessageToAPI(const QString &message);
    void appendMessage(const QString &sender, const QString &message);
    QString parseGeminiResponse(const QByteArray& jsonData);
};

#endif // AICHATDIALOG_H
