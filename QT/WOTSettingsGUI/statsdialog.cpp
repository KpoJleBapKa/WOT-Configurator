#include "statsdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTextBrowser>
#include <QLabel>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QDebug>

StatsDialog::StatsDialog(QWidget *parent) :
    QDialog(parent),
    nicknameInput(nullptr),
    searchButton(nullptr),
    closeButton(nullptr),
    resultsOutput(nullptr),
    networkManager(new QNetworkAccessManager(this)),
    m_apiKey("d889298af2382fa0cfeb010e26874b63")
{
    setupUi(); // Налаштовуємо інтерфейс

    // З'єднуємо кнопки зі слотами
    connect(searchButton, &QPushButton::clicked, this, &StatsDialog::onSearchButtonClicked);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept); // Просто закриває діалог
}

StatsDialog::~StatsDialog()
{
    // networkManager видалиться автоматично, бо QDialog є його батьком
}

void StatsDialog::setupUi()
{
    this->setWindowTitle("Статистика гравця World of Tanks");
    this->setMinimumSize(550, 450);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // Layout для введення нікнейму
    QHBoxLayout *inputLayout = new QHBoxLayout();
    QLabel *nicknameLabel = new QLabel("Нікнейм:", this);
    nicknameInput = new QLineEdit(this);
    nicknameInput->setPlaceholderText("Введіть нікнейм гравця...");
    searchButton = new QPushButton("Пошук", this);
    inputLayout->addWidget(nicknameLabel);
    inputLayout->addWidget(nicknameInput, 1); // Додаємо stretch factor для поля вводу
    inputLayout->addWidget(searchButton);

    // Поле для виведення результатів
    resultsOutput = new QTextBrowser(this);
    resultsOutput->setReadOnly(true);
    resultsOutput->setOpenExternalLinks(true); // Щоб посилання відкривалися
    resultsOutput->setPlaceholderText("Статистика гравця: \n Клан: \n Кількість боїв: \n Відсоток перемог: \n Відсоток влучень: \n Середня шкода: \n Середній досвід: \n Максимум знищено за бій: \n Максимальний досвід за бій: \n");
    resultsOutput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Кнопка закриття
    QHBoxLayout *closeLayout = new QHBoxLayout();
    closeButton = new QPushButton("Закрити", this);
    closeButton->setMinimumHeight(30);
    closeLayout->addStretch();
    closeLayout->addWidget(closeButton);

    // Додаємо елементи до головного layout
    mainLayout->addLayout(inputLayout);
    mainLayout->addWidget(resultsOutput); // Текстове поле займає основний простір
    mainLayout->addLayout(closeLayout);

    this->setLayout(mainLayout);
}

// --- Слот для кнопки "Пошук" ---
void StatsDialog::onSearchButtonClicked()
{
    QString nickname = nicknameInput->text().trimmed();
    if (nickname.isEmpty()) {
        QMessageBox::warning(this, "Помилка вводу", "Будь ласка, введіть нікнейм гравця.");
        return;
    }
    resetStateBeforeSearch(); // Скидаємо стан
    m_searchedNickname = nickname;
    searchPlayerAccountID(nickname); // Запускаємо пошук ID
}

// --- Скидання стану перед новим пошуком ---
void StatsDialog::resetStateBeforeSearch() {
    resultsOutput->clear();
    resultsOutput->setPlaceholderText(QString("Йде пошук гравця '%1'...").arg(m_searchedNickname));
    searchButton->setEnabled(false); // Блокуємо кнопку
    nicknameInput->setEnabled(false); // Блокуємо поле вводу
    m_accountId.clear();
    m_playerStatsData = QJsonObject(); // Очищуємо збережені дані

    // Відключаємо ВСІ можливі з'єднання від finished, щоб уникнути плутанини
    disconnect(networkManager, &QNetworkAccessManager::finished, nullptr, nullptr);
}

// --- Запит 1: Пошук ID гравця ---
void StatsDialog::searchPlayerAccountID(const QString &nickname)
{
    qInfo() << "Requesting Account ID for:" << nickname;
    QUrl url("https://api.worldoftanks.eu/wot/account/list/");
    QUrlQuery query;
    query.addQueryItem("application_id", m_apiKey);
    query.addQueryItem("search", nickname);
    query.addQueryItem("limit", "1");
    url.setQuery(query);

    QNetworkRequest request(url);
    // Підключаємо слот ОБРОБКИ ID до сигналу finished
    connect(networkManager, &QNetworkAccessManager::finished, this, &StatsDialog::onAccountIdReplyFinished);
    networkManager->get(request);
}

// --- Обробка відповіді з ID гравця ---
void StatsDialog::onAccountIdReplyFinished(QNetworkReply *reply)
{
    // Важливо: відключаємо цей слот одразу, щоб він не спрацював на наступні відповіді
    disconnect(networkManager, &QNetworkAccessManager::finished, this, &StatsDialog::onAccountIdReplyFinished);

    if (reply->error() != QNetworkReply::NoError) {
        displayError(QString("Помилка мережі при пошуку ID: %1").arg(reply->errorString()));
        reply->deleteLater();
        searchButton->setEnabled(true); // Розблоковуємо кнопку
        nicknameInput->setEnabled(true);
        return;
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater(); // Очищуємо відповідь якомога раніше
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        displayError("Помилка: Не вдалося розпарсити JSON відповідь для ID гравця.");
        searchButton->setEnabled(true); nicknameInput->setEnabled(true); return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    if (jsonObj["status"].toString() != "ok") {
        QString errorMsg = "Невідома помилка API";
        if(jsonObj.contains("error") && jsonObj["error"].isObject()){
            errorMsg = jsonObj["error"].toObject()["message"].toString("Не вдалося прочитати помилку API");
        }
        displayError(QString("Помилка API пошуку ID: %1").arg(errorMsg));
        searchButton->setEnabled(true); nicknameInput->setEnabled(true); return;
    }
    if (!jsonObj.contains("data") || !jsonObj["data"].isArray()) {
        displayError("Помилка: Неправильна структура відповіді API (відсутній масив 'data').");
        searchButton->setEnabled(true); nicknameInput->setEnabled(true); return;
    }


    QJsonArray dataArray = jsonObj["data"].toArray();
    if (dataArray.isEmpty()) {
        displayError(QString("Гравець з нікнеймом '%1' не знайдений.").arg(m_searchedNickname));
        searchButton->setEnabled(true); nicknameInput->setEnabled(true); return;
    }

    m_accountId = QString::number(dataArray[0].toObject()["account_id"].toVariant().toLongLong());
    qInfo() << "Account ID found:" << m_accountId;
    resultsOutput->setPlaceholderText(QString("Отримання статистики для ID: %1...").arg(m_accountId));

    // Запускаємо наступний запит - на отримання статистики
    requestPlayerInfo(m_accountId);
}

// --- Запит 2: Статистика гравця ---
void StatsDialog::requestPlayerInfo(const QString &accountId)
{
    qInfo() << "Requesting Player Info for ID:" << accountId;
    QUrl url("https://api.worldoftanks.eu/wot/account/info/");
    QUrlQuery query;
    query.addQueryItem("application_id", m_apiKey);
    query.addQueryItem("account_id", accountId);
    // query.addQueryItem("fields", "statistics.all"); // Можна запитати лише потрібні поля
    url.setQuery(query);

    QNetworkRequest request(url);
    // Підключаємо слот ОБРОБКИ СТАТИСТИКИ до сигналу finished
    connect(networkManager, &QNetworkAccessManager::finished, this, &StatsDialog::onPlayerInfoReplyFinished);
    networkManager->get(request);
}

// --- Обробка відповіді зі статистикою гравця ---
void StatsDialog::onPlayerInfoReplyFinished(QNetworkReply *reply)
{
    disconnect(networkManager, &QNetworkAccessManager::finished, this, &StatsDialog::onPlayerInfoReplyFinished);

    if (reply->error() != QNetworkReply::NoError) {
        displayError(QString("Помилка мережі при отриманні статистики: %1").arg(reply->errorString()));
        reply->deleteLater();
        searchButton->setEnabled(true); nicknameInput->setEnabled(true); return;
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        displayError("Помилка: Не вдалося розпарсити JSON відповідь статистики гравця.");
        searchButton->setEnabled(true); nicknameInput->setEnabled(true); return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    if (jsonObj["status"].toString() != "ok") {
        QString errorMsg = "Невідома помилка API";
        if(jsonObj.contains("error") && jsonObj["error"].isObject()){
            errorMsg = jsonObj["error"].toObject()["message"].toString("Не вдалося прочитати помилку API");
        }
        displayError(QString("Помилка API статистики гравця: %1").arg(errorMsg));
        searchButton->setEnabled(true); nicknameInput->setEnabled(true); return;
    }
    if (!jsonObj.contains("data") || !jsonObj["data"].isObject()) {
        displayError("Помилка: Неправильна структура відповіді API статистики (відсутній об'єкт 'data').");
        searchButton->setEnabled(true); nicknameInput->setEnabled(true); return;
    }


    QJsonObject dataObj = jsonObj["data"].toObject();
    if (!dataObj.contains(m_accountId) || dataObj[m_accountId].isNull()) {
        displayError("Помилка: Дані для гравця з ID " + m_accountId + " не знайдені у відповіді статистики.");
        searchButton->setEnabled(true); nicknameInput->setEnabled(true); return;
    }

    // Зберігаємо дані статистики гравця
    m_playerStatsData = dataObj[m_accountId].toObject();
    qInfo() << "Player stats data received.";
    resultsOutput->setPlaceholderText(QString("Отримання інформації про клан для ID: %1...").arg(m_accountId));


    // Запускаємо наступний запит - на отримання інформації про клан
    requestClanInfo(m_accountId);
}

// --- Запит 3: Інформація про клан ---
void StatsDialog::requestClanInfo(const QString &accountId) {
    qInfo() << "Requesting Clan Info for ID:" << accountId;
    QUrl url("https://api.worldoftanks.eu/wot/clans/accountinfo/");
    QUrlQuery query;
    query.addQueryItem("application_id", m_apiKey);
    query.addQueryItem("account_id", accountId);
    query.addQueryItem("fields", "clan.tag"); // Запитуємо тільки тег
    url.setQuery(query);

    QNetworkRequest request(url);
    // Підключаємо слот ОБРОБКИ КЛАНУ до сигналу finished
    connect(networkManager, &QNetworkAccessManager::finished, this, &StatsDialog::onClanInfoReplyFinished);
    networkManager->get(request);
}

// --- Обробка відповіді про клан ---
void StatsDialog::onClanInfoReplyFinished(QNetworkReply* reply) {
    disconnect(networkManager, &QNetworkAccessManager::finished, this, &StatsDialog::onClanInfoReplyFinished);
    QString clanTag = "N/A"; // Значення за замовчуванням

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Network error requesting clan info:" << reply->errorString();
        clanTag = "[Помилка мережі]";
    } else {
        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        if (!jsonDoc.isNull() && jsonDoc.isObject()) {
            QJsonObject jsonObj = jsonDoc.object();
            if (jsonObj["status"].toString() == "ok" && jsonObj.contains("data") && jsonObj["data"].isObject()) {
                QJsonObject dataObj = jsonObj["data"].toObject();
                // Перевіряємо, чи є дані для гравця і чи вони не null
                if (dataObj.contains(m_accountId) && !dataObj[m_accountId].isNull()) {
                    // Гравець у клані, отримуємо тег
                    clanTag = dataObj[m_accountId].toObject()["clan"].toObject()["tag"].toString("[Немає тегу]");
                } else {
                    // Гравець не в клані
                    clanTag = "Без клану";
                }
            } else {
                qWarning() << "API error getting clan info:" << jsonObj["error"].toObject()["message"].toString("Unknown API error");
                clanTag = "[Помилка API]";
            }
        } else {
            qWarning() << "Failed to parse clan info JSON response.";
            clanTag = "[Помилка даних]";
        }
    }
    reply->deleteLater();

    // Зберігаємо тег клану
    m_clanTag = clanTag;
    qInfo() << "Clan info processed. Tag:" << m_clanTag;

    // Тепер, коли є всі дані, викликаємо фінальне відображення
    displayResults();

    // Розблоковуємо кнопку пошуку та поле вводу
    searchButton->setEnabled(true);
    nicknameInput->setEnabled(true);
}


// --- Відображення зібраних результатів ---
void StatsDialog::displayResults()
{
    resultsOutput->clear(); // Очищуємо поле перед виводом

    // Перевіряємо, чи є основні дані статистики
    if (!m_playerStatsData.contains("statistics") || !m_playerStatsData["statistics"].isObject()) {
        displayError("Помилка: Відсутні дані статистики для відображення.");
        return;
    }
    QJsonObject statsAll = m_playerStatsData["statistics"].toObject()["all"].toObject();

    // Витягуємо значення, обережно конвертуючи типи
    long long battles = statsAll["battles"].toVariant().toLongLong();
    long long wins = statsAll["wins"].toVariant().toLongLong();
    long long hits = statsAll["hits"].toVariant().toLongLong();
    long long shots = statsAll["shots"].toVariant().toLongLong();
    long long damage_dealt = statsAll["damage_dealt"].toVariant().toLongLong();
    long long xp = statsAll["xp"].toVariant().toLongLong();
    int max_frags = statsAll["max_frags"].toInt();
    int max_xp = statsAll["max_xp"].toInt();

    // Формуємо HTML для виводу
    QString resultsHtml = QString("<h3>Статистика гравця: %1</h3>").arg(m_searchedNickname);
    resultsHtml += QString("<p><b>Клан:</b> [%1]</p>").arg(m_clanTag); // Додаємо клан
    resultsHtml += "<hr>";
    resultsHtml += QString("<p><b>Кількість боїв:</b> %L1</p>").arg(battles);

    if (battles > 0) {
        double winRate = static_cast<double>(wins) * 100.0 / battles;
        resultsHtml += QString("<p><b>Відсоток перемог:</b> %1%</p>").arg(QString::number(winRate, 'f', 2));

        if (shots > 0) {
            double hitRate = static_cast<double>(hits) * 100.0 / shots;
            resultsHtml += QString("<p><b>Відсоток влучень:</b> %1%</p>").arg(QString::number(hitRate, 'f', 2));
        } else {
            resultsHtml += QString("<p><b>Відсоток влучень:</b> 0.00%</p>");
        }
        double avgDamage = static_cast<double>(damage_dealt) / battles;
        double avgExp = static_cast<double>(xp) / battles;
        resultsHtml += QString("<p><b>Середня шкода:</b> %1</p>").arg(QString::number(avgDamage, 'f', 2));
        resultsHtml += QString("<p><b>Середній досвід:</b> %1</p>").arg(QString::number(avgExp, 'f', 2));
    } else {
        resultsHtml += "<p><i>Недостатньо боїв для розрахунку середніх значень.</i></p>";
    }
    resultsHtml += QString("<p><b>Максимум знищено за бій:</b> %1</p>").arg(max_frags);
    resultsHtml += QString("<p><b>Максимальний досвід за бій:</b> %1</p>").arg(max_xp);

    resultsOutput->setHtml(resultsHtml); // Встановлюємо HTML у QTextBrowser
}

// --- Відображення помилки ---
void StatsDialog::displayError(const QString &errorText)
{
    qWarning() << "StatsDialog Error:" << errorText;
    resultsOutput->setHtml(QString("<font color='red'><b>Помилка:</b><br>%1</font>").arg(errorText));
    // Переконуємося, що кнопки розблоковані у випадку помилки
    searchButton->setEnabled(true);
    nicknameInput->setEnabled(true);
}
