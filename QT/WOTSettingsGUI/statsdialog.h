#ifndef STATSDIALOG_H
#define STATSDIALOG_H

#include <QDialog>
#include <QtNetwork/QNetworkAccessManager>
#include <QString>
#include <QJsonObject> // Включено раніше

// Попередні оголошення класів Qt
QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
class QTextBrowser;
class QVBoxLayout;
class QHBoxLayout;
class QNetworkReply;
namespace Ui { class StatsDialog; }
QT_END_NAMESPACE

class StatsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StatsDialog(QWidget *parent = nullptr);
    ~StatsDialog();

private slots:
    void onSearchButtonClicked();
    void onAccountIdReplyFinished(QNetworkReply *reply);
    void onPlayerInfoReplyFinished(QNetworkReply *reply);
    void onClanInfoReplyFinished(QNetworkReply *reply);

private:
    // Елементи UI
    QLineEdit *nicknameInput;
    QPushButton *searchButton;
    QPushButton *closeButton;
    QTextBrowser *resultsOutput;

    // Мережевий менеджер
    QNetworkAccessManager *networkManager;

    // Змінні для зберігання даних під час процесу запитів
    QString m_apiKey;
    QString m_searchedNickname;
    QString m_accountId;
    QJsonObject m_playerStatsData; // Зберігаємо об'єкт статистики
    QString m_clanTag;

    // Приватні функції
    void setupUi();
    void searchPlayerAccountID(const QString &nickname);
    void requestPlayerInfo(const QString &accountId);
    void requestClanInfo(const QString &accountId);
    void displayResults();
    void displayError(const QString &errorText);
    void resetStateBeforeSearch();
};

#endif // STATSDIALOG_H
