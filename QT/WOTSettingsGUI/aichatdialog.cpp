#include "aichatdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextBrowser>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSpacerItem>
#include <QSizePolicy>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QApplication>
#include <QScrollBar>
#include <QDebug>

AIChatDialog::AIChatDialog(QWidget *parent) :
    QDialog(parent),
    chatDisplay(nullptr),
    messageInput(nullptr),
    sendButton(nullptr),
    closeButton(nullptr),
    statusLabel(nullptr),
    networkManager(new QNetworkAccessManager(this))
{
    m_apiKey = "AIzaSyCDVA2LFFLTZF5Nuw-kqRtJNB3pYGOjv6M";

    if (m_apiKey == "YOUR_API_KEY_HERE" || m_apiKey.isEmpty()) {
        qWarning() << "API Key is missing or is a placeholder in aichatdialog.cpp! AI features will not work.";
        QMessageBox::critical(this, "Помилка конфігурації", "API ключ для AI Помічника не налаштовано.");
    }

    m_apiUrl = QString("https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key=%1").arg(m_apiKey);

    // Визначення базового промпту
    m_basePrompt = "Ти - корисний AI-асистент для гравців World of Tanks. "
                   "Грати на САУ - дуже погано. "
                   "Універсальне обладнання для важкого, середнього танку - гарматний досилач, стабілізатор, поліпшене загартування/турбонагнітач. "
                   "Універсальне обладнання для пт-сау - гарматний досилач, поліпшене загартування та турбонагнітач. "
                   "Універсальне обладнання для легкого танку - просвітлена оптика, малошумна система вихлопу, командирський оглядовий прилад. "
                   "Універсальне обладнання для сау - ніяке, на цьому класі техніки грати не потрібно. "
                   "Відповідай на запитання щодо механік гри, танків, тактик, налаштувань гри, "
                   "але уникай тем, не пов'язаних з World of Tanks. "
                   "Будь ввічливим та надавай чіткі відповіді. "
                   "У своїх відповідях не використовуй спеціальні символи *. "
                   "Якщо запитання не стосується World of Tanks, ввічливо вкажи на це. \n\n"
                   "Запитання користувача: ";
    // ---------------------------------------------

    setupUi();

    connect(sendButton, &QPushButton::clicked, this, &AIChatDialog::onSendButtonClicked);
    connect(messageInput, &QLineEdit::returnPressed, this, &AIChatDialog::onSendButtonClicked);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(networkManager, &QNetworkAccessManager::finished, this, &AIChatDialog::onNetworkReply);

    appendMessage("AI", "Привіт! Я AI Помічник. Запитуйте про World of Tanks.");
}

AIChatDialog::~AIChatDialog()
{
    // networkManager видалиться автоматично
}

// --- Налаштування UI ---
void AIChatDialog::setupUi()
{
    this->setWindowTitle("AI Помічник Танкіста");
    this->setMinimumSize(550, 450);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    chatDisplay = new QTextBrowser(this);
    chatDisplay->setReadOnly(true);
    chatDisplay->setOpenExternalLinks(true);
    chatDisplay->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    statusLabel = new QLabel(" ", this);
    statusLabel->setAlignment(Qt::AlignLeft);
    statusLabel->setStyleSheet("QLabel { color : gray; font-style: italic; }");
    statusLabel->setFixedHeight(statusLabel->fontMetrics().height());

    QHBoxLayout *inputLayout = new QHBoxLayout();
    messageInput = new QLineEdit(this);
    messageInput->setPlaceholderText("Ваше запитання...");
    sendButton = new QPushButton("Надіслати", this);
    inputLayout->addWidget(messageInput, 1);
    inputLayout->addWidget(sendButton);

    QHBoxLayout *closeLayout = new QHBoxLayout();
    closeButton = new QPushButton("Закрити", this);
    closeButton->setMinimumHeight(30);
    closeLayout->addStretch();
    closeLayout->addWidget(closeButton);

    mainLayout->addWidget(chatDisplay);
    mainLayout->addWidget(statusLabel);
    mainLayout->addLayout(inputLayout);
    mainLayout->addLayout(closeLayout);

    this->setLayout(mainLayout);

    // Встановлюємо кнопку "Надіслати" як основну для Enter
    sendButton->setDefault(true);
    // Встановлюємо кнопку "Закрити", щоб вона НЕ була кнопкою за замовчуванням
    closeButton->setAutoDefault(false); // <-- ДОДАНО

    messageInput->setFocus();
}


// --- Слот кнопки "Надіслати" ---
void AIChatDialog::onSendButtonClicked()
{
    QString userMessage = messageInput->text().trimmed();
    // Перевіряємо, чи не порожнє повідомлення і чи активна кнопка (тобто не йде попередній запит)
    if (userMessage.isEmpty() || !sendButton->isEnabled()) {
        return;
    }
    // Також перевіряємо наявність ключа ще раз, lol
    if (m_apiKey == "YOUR_API_KEY_HERE" || m_apiKey.isEmpty()) {
        QMessageBox::warning(this, "Помилка", "API ключ не налаштовано.");
        return;
    }


    appendMessage("Ви", userMessage);
    messageInput->clear();
    sendMessageToAPI(userMessage);
}

// --- Додавання повідомлення у чат ---
void AIChatDialog::appendMessage(const QString &sender, const QString &message)
{
    QString color = (sender == "AI") ? "blue" : "green";
    if (sender == "Помилка") color = "red";

    QString formattedMessage = QString("<p style='margin: 2px 0px'><font color='%1'><b>%2:</b></font> %3</p>")
                                   .arg(color, sender, message.toHtmlEscaped().replace("\n", "<br>"));

    chatDisplay->append(formattedMessage);
    chatDisplay->verticalScrollBar()->setValue(chatDisplay->verticalScrollBar()->maximum());
}

// --- Надсилання запиту до API ---
void AIChatDialog::sendMessageToAPI(const QString &message)
{

    statusLabel->setText("<i>AI генерує відповідь...</i>");
    sendButton->setEnabled(false);
    messageInput->setEnabled(false);
    QApplication::processEvents();

    QNetworkRequest request(m_apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Формуємо JSON тіло запиту
    QJsonObject requestBody;
    QJsonArray contentsArray;
    QJsonObject content;
    QJsonArray partsArray;
    QJsonObject part;

    QString fullPrompt = m_basePrompt + message;
    part["text"] = fullPrompt; // Надсилаємо базовий промпт разом із запитанням

    partsArray.append(part);
    content["parts"] = partsArray;
    contentsArray.append(content);
    requestBody["contents"] = contentsArray;

    QJsonDocument doc(requestBody);
    QByteArray jsonData = doc.toJson();

    qInfo() << "Sending API Request to Gemini with base prompt...";

    networkManager->post(request, jsonData);
}

// --- Обробка відповіді від API ---
void AIChatDialog::onNetworkReply(QNetworkReply *reply)
{
    statusLabel->setText(" "); // Прибираємо статус
    sendButton->setEnabled(true); // Розблоковуємо кнопку і поле
    messageInput->setEnabled(true);
    messageInput->setFocus(); // Повертаємо фокус

    if (reply->error() != QNetworkReply::NoError) {
        // Спробуємо отримати більше інформації про помилку з тіла відповіді (якщо є)
        QByteArray errorData = reply->readAll();
        QString extraInfo;
        QJsonDocument errorDoc = QJsonDocument::fromJson(errorData);
        if(!errorDoc.isNull() && errorDoc.isObject() && errorDoc.object().contains("error")) {
            extraInfo = errorDoc.object()["error"].toObject()["message"].toString();
        }

        QString errorMsg = QString("Помилка мережі: %1. %2").arg(reply->errorString(), extraInfo).trimmed();
        qWarning() << errorMsg;
        appendMessage("Помилка", errorMsg);
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    QString aiResponse = parseGeminiResponse(responseData); // Парсимо відповідь
    appendMessage("AI", aiResponse); // Відображаємо
}

// --- Парсинг відповіді Gemini ---
QString AIChatDialog::parseGeminiResponse(const QByteArray& jsonData) {
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    QString defaultError = "[Не вдалося обробити відповідь від AI (неправильний формат JSON)]";
    QString parseError = "[Помилка парсингу відповіді AI]";

    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        qWarning() << "Failed to parse API JSON response:" << jsonData;
        return defaultError;
    }

    QJsonObject rootObj = jsonDoc.object();
    // qInfo() << "API Response JSON:" << rootObj; // Для відладки

    // Перевірка на стандартну помилку API
    if (rootObj.contains("error") && rootObj["error"].isObject()) {
        QJsonObject errorObj = rootObj["error"].toObject();
        QString apiErrorMsg = errorObj["message"].toString("[Невідома помилка API]");
        qWarning() << "API Error:" << apiErrorMsg << "Status:" << errorObj["status"].toString();
        return QString("[Помилка API: %1]").arg(apiErrorMsg);
    }

    // Парсинг успішної відповіді
    if (rootObj.contains("candidates") && rootObj["candidates"].isArray()) {
        QJsonArray candidates = rootObj["candidates"].toArray();
        if (!candidates.isEmpty() && candidates[0].isObject()) {
            QJsonObject firstCandidate = candidates[0].toObject();

            // Перевірка на блок (безпека/цитування/інше)
            if (firstCandidate.contains("finishReason") &&
                firstCandidate["finishReason"].toString() != "STOP") // "STOP" означає нормальне завершення
            {
                QString reason = firstCandidate["finishReason"].toString();
                qWarning() << "AI Response potentially blocked. Finish Reason:" << reason;
                return QString("[Відповідь не завершена або заблокована (Причина: %1)]").arg(reason);
            }

            // Отримання тексту
            if (firstCandidate.contains("content") && firstCandidate["content"].isObject()) {
                QJsonObject content = firstCandidate["content"].toObject();
                if (content.contains("parts") && content["parts"].isArray()) {
                    QJsonArray parts = content["parts"].toArray();
                    if (!parts.isEmpty() && parts[0].isObject() && parts[0].toObject().contains("text")) {
                        // Успішно отримали текст
                        return parts[0].toObject()["text"].toString(parseError).trimmed();
                    }
                }
            }
        }
    }

    // Якщо структура відповіді неочікувана
    qWarning() << "Could not find valid text part in Gemini response structure:" << rootObj;
    return parseError;
}
