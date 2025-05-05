#include "main.h" // Головний заголовок (містить оголошення FileValidator та ValidationResult)
#include "pugixml/pugixml.hpp"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <limits>    // Для numeric_limits (якщо потрібен)
#include <iostream>  // Для std::cerr у внутрішніх методах
#include <QMessageBox> // Для вікон повідомлень у validateBeforeAction
#include <QString>     // Для роботи з QMessageBox


// --- Допоміжні функції парсингу ---
namespace { // Анонімний простір імен для локальних допоміжних функцій
bool tryParseFloat(const char* text, float& outValue) {
    if (!text) return false;
    std::string s(text); // Копіюємо в рядок для роботи з istringstream
    std::istringstream iss(s);
    iss >> outValue;
    // Перевіряємо стан та чи не залишилось нечислових символів (окрім початкових/кінцевих пробілів)
    // iss.eof() може не спрацювати, якщо є пробіли в кінці
    char remaining;
    return !iss.fail() && !(iss >> remaining); // Чи вдалося прочитати ще щось після числа
}
bool tryParseInt(const char* text, int& outValue) {
    if (!text) return false;
    std::string s(text);
    std::istringstream iss(s);
    iss >> outValue;
    char remaining;
    return !iss.fail() && !(iss >> remaining);
}
} // кінець анонімного простору імен


// --- Реалізація основного методу валідації ---
ValidationResult FileValidator::validateFile(const fs::path& filePath) {
    ValidationResult result;
    pugi::xml_document doc;

    if (!fs::exists(filePath)) {
        result.wellFormedError = "Файл не знайдено: " + filePath.string();
        return result; // isWellFormed = false
    }
    if (!fs::is_regular_file(filePath)) {
        result.wellFormedError = "Шлях не є файлом: " + filePath.string();
        return result; // isWellFormed = false
    }

    // 1. Перевірка Well-Formed
    std::string wfError;
    result.isWellFormed = isXmlWellFormedInternal(filePath, doc, wfError);
    if (!result.isWellFormed) {
        result.wellFormedError = wfError;
        return result;
    }

    // 2. Перевірка структури (тільки якщо XML коректний)
    std::string structWarn;
    result.hasStructure = hasExpectedStructureInternal(doc, structWarn);
    result.structureInfo = structWarn; // Записуємо повідомлення (може бути "OK" або попередження)

    // 3. Перевірка значень (тільки якщо XML коректний)
    result.valueErrors = findInvalidSimpleValuesInternal(doc);

    return result;
}

// --- Реалізація допоміжного методу для GUI ---
bool FileValidator::validateBeforeAction(const fs::path& filePath, const std::string& actionNameStd, bool showSuccess) {
    QString actionName = QString::fromStdString(actionNameStd); // Конвертуємо для QMessageBox
    ValidationResult result = validateFile(filePath);

    if (result.isValid()) { // XML коректний
        QString summary; // Резюме для повідомлення
        bool hasWarnings = false;

        summary += "XML: OK.\n";
        summary += QString("Структура: %1\n").arg(QString::fromStdString(result.structureInfo));
        if (!result.hasStructure) hasWarnings = true;

        if (result.valueErrors.empty()) {
            summary += "Значення: OK.";
        } else {
            summary += QString("Значення: Знайдено %1 потенційних проблем.").arg(result.valueErrors.size());
            hasWarnings = true;
            // Додамо перші кілька помилок для наочності
            summary += "\nПриклади помилок значень:\n";
            int count = 0;
            for(const auto& err : result.valueErrors) {
                if (++count > 3) { // Обмежимо кількість для вікна
                    summary += "- ...\n";
                    break;
                }
                summary += QString("- %1\n").arg(QString::fromStdString(err));
            }
        }

        if (hasWarnings) {
            QMessageBox::StandardButton reply;
            QString warningText = QString("Увага! Файл '%1' містить попередження:\n\n%2\n\nПродовжити дію '%3'?")
                                      .arg(QString::fromStdWString(filePath.filename().wstring()))
                                      .arg(summary)
                                      .arg(actionName);
            reply = QMessageBox::warning(nullptr, actionName + " - Попередження валідації",
                                         warningText,
                                         QMessageBox::Yes | QMessageBox::No,
                                         QMessageBox::No);
            return (reply == QMessageBox::Yes);
        } else {
            // Немає помилок і попереджень
            if (showSuccess) {
                QMessageBox::information(nullptr, actionName + " - Валідація успішна",
                                         QString("Файл '%1' успішно пройшов валідацію.")
                                             .arg(QString::fromStdWString(filePath.filename().wstring())));
            }
            return true; // Можна продовжувати
        }
    } else { // Критична помилка XML
        QMessageBox::critical(nullptr, actionName + " - Помилка валідації",
                              QString("Не вдалося виконати дію '%1'.\nФайл '%2' не пройшов валідацію:\n%3")
                                  .arg(actionName)
                                  .arg(QString::fromStdWString(filePath.filename().wstring()))
                                  .arg(QString::fromStdString(result.wellFormedError)));
        return false; // Не можна продовжувати
    }
}


// --- Внутрішні методи реалізації ---

bool FileValidator::isXmlWellFormedInternal(const fs::path& filePath, pugi::xml_document& doc, std::string& errorMsg) {
    // doc вже передається за посиланням, перевіряємо тільки результат завантаження
    pugi::xml_parse_result result = doc.load_file(filePath.c_str()); // Використовуємо c_str()
    if (result.status != pugi::status_ok) {
        // Формуємо повідомлення про помилку
        std::stringstream ss;
        ss << "Помилка XML: " << result.description() << " (позиція " << result.offset << ")";
        errorMsg = ss.str();
        return false;
    }
    errorMsg = "OK";
    return true;
}

bool FileValidator::hasExpectedStructureInternal(const pugi::xml_document& doc, std::string& warnings) {
    // doc вже завантажено в isXmlWellFormedInternal
    bool structureOk = true;
    std::stringstream warningStream;

    const pugi::xml_node root = doc.child("root"); // Використовуємо const, бо не змінюємо
    if (!root) {
        warnings = "Відсутній кореневий елемент <root>.";
        return false; // Це критична помилка структури
    }

    if (!root.child("scriptsPreferences")) {
        warningStream << "Відсутня секція <scriptsPreferences>. ";
        structureOk = false;
    }
    if (!root.child("graphicsPreferences")) {
        warningStream << "Відсутня секція <graphicsPreferences>. ";
        structureOk = false;
    }
    if (!root.child("devicePreferences")) {
        warningStream << "Відсутня секція <devicePreferences>. ";
        structureOk = false;
    }

    // Додаткові перевірки
    const pugi::xml_node scriptsPrefs = root.child("scriptsPreferences");
    if (scriptsPrefs && !scriptsPrefs.child("soundPrefs")) {
        warningStream << "Відсутня підсекція <soundPrefs> всередині <scriptsPreferences>. ";
        // Можна не вважати це критичною помилкою (structureOk = false;)
    }
    if (scriptsPrefs && !scriptsPrefs.child("controlMode")) {
        warningStream << "Відсутня підсекція <controlMode> всередині <scriptsPreferences>. ";
    }


    warnings = warningStream.str();
    if (structureOk && warnings.empty()) {
        warnings = "OK"; // Якщо все добре і попереджень немає
    }
    return structureOk; // Повертаємо загальний статус (true, якщо основні секції є)
}

std::vector<std::string> FileValidator::findInvalidSimpleValuesInternal(const pugi::xml_document& doc) {
    // doc вже завантажено
    std::vector<std::string> errors;
    const pugi::xml_node root = doc.child("root");
    if (!root) return {"Помилка: Відсутній <root> елемент для перевірки значень."}; // Повертаємо вектор з однією помилкою

    // 1. Перевірка <soundPrefs>
    const pugi::xml_node scriptsPrefs = root.child("scriptsPreferences");
    if (scriptsPrefs) {
        const pugi::xml_node soundPrefs = scriptsPrefs.child("soundPrefs");
        if (soundPrefs) {
            const std::vector<std::string> volumeTags = { /* ... список тегів гучності ... */
                "masterVolume", "volume_micVivox", "volume_vehicles", "volume_music",
                "volume_effects", "volume_ambient", "volume_gui", "volume_voice",
                "volume_masterFadeVivox", "volume_masterVivox", "volume_music_hangar",
                "volume_ev_ambient", "volume_ev_effects", "volume_ev_gui",
                "volume_ev_music", "volume_ev_vehicles", "volume_ev_voice"
            };
            for (const std::string& tagName : volumeTags) {
                pugi::xml_node node = soundPrefs.child(tagName.c_str());
                if (node) {
                    float value;
                    const char* text = node.text().as_string();
                    if (tryParseFloat(text, value)) {
                        if (value < 0.0f || value > 1.0f) {
                            errors.push_back("<" + tagName + ">: '" + text + "'. Очікується [0.0, 1.0].");
                        }
                    } else {
                        errors.push_back("<" + tagName + ">: '" + text + "'. Очікується число.");
                    }
                }
            }
        } // Не додаємо помилку, якщо немає <soundPrefs>, це перевіряє структура

        // Перевірка <fov>
        const pugi::xml_node fovNode = scriptsPrefs.child("fov");
        if(fovNode) {
            float value;
            const char* text = fovNode.text().as_string();
            if(tryParseFloat(text, value)) {
                if (value < 50.0f || value > 130.0f) { // Допустимий діапазон FOV
                    errors.push_back("<fov>: '" + std::string(text) + "'. Нетипове значення (очікується 50-130).");
                }
            } else {
                errors.push_back("<fov>: '" + std::string(text) + "'. Очікується число.");
            }
        } // Не додаємо помилку, якщо немає <fov>

    } // Не додаємо помилку, якщо немає <scriptsPreferences>


    // 2. Перевірка <devicePreferences>
    const pugi::xml_node devicePrefs = root.child("devicePreferences");
    if (devicePrefs) {
        // Window Mode
        pugi::xml_node wmNode = devicePrefs.child("windowMode");
        if (wmNode) {
            int value;
            const char* text = wmNode.text().as_string();
            if(tryParseInt(text, value)) {
                if (value < 0 || value > 2) { // 0 - вікно, 1 - повноекранне вікно, 2 - повний екран
                    errors.push_back("<windowMode>: '" + std::string(text) + "'. Очікується 0, 1, або 2.");
                }
            } else {
                errors.push_back("<windowMode>: '" + std::string(text) + "'. Очікується ціле число.");
            }
        } // Не додаємо помилку про відсутність

        // Resolutions
        const std::vector<std::pair<std::string, int>> resolutionTags = {
            {"windowedWidth", 640}, {"windowedHeight", 480},
            {"fullscreenWidth", 800}, {"fullscreenHeight", 600}
        };
        for (const auto& pair : resolutionTags) {
            pugi::xml_node resNode = devicePrefs.child(pair.first.c_str());
            if(resNode) {
                int value;
                const char* text = resNode.text().as_string();
                if(tryParseInt(text, value)) {
                    if (value < pair.second) {
                        errors.push_back("<" + pair.first + ">: '" + text + "'. Очікується >= " + std::to_string(pair.second) + ".");
                    }
                } else {
                    errors.push_back("<" + pair.first + ">: '" + text + "'. Очікується ціле число.");
                }
            } // Не додаємо помилку про відсутність
        }
        // Refresh Rate
        pugi::xml_node rrNode = devicePrefs.child("fullscreenRefresh");
        if(rrNode) {
            int value;
            const char* text = rrNode.text().as_string();
            if(tryParseInt(text, value)) {
                if (value < 10 || value > 400) { // Розширений діапазон для сучасних моніторів
                    errors.push_back("<fullscreenRefresh>: '" + std::string(text) + "'. Нетипове значення (очікується 50-240).");
                }
            } else {
                errors.push_back("<fullscreenRefresh>: '" + std::string(text) + "'. Очікується ціле число.");
            }
        } // Не додаємо помилку про відсутність
    } // Не додаємо помилку, якщо немає <devicePreferences>

    // 3. Перевірка графіки
    const pugi::xml_node graphPrefs = root.child("graphicsPreferences");
    if (graphPrefs) {
        const std::vector<std::string> graphTags = {
            "colorGradingStrength", "brightnessDeferred", "contrastDeferred", "saturationDeferred"
        };
        for (const std::string& tagName : graphTags) {
            pugi::xml_node node = graphPrefs.child(tagName.c_str());
            if (node) {
                float value;
                const char* text = node.text().as_string();
                if (tryParseFloat(text, value)) {
                    if (value < 0.0f || value > 1.5f) { // Трохи розширимо діапазон
                        errors.push_back("<" + tagName + ">: '" + text + "'. Нетипове значення (очікується ~[0.0, 1.0]).");
                    }
                } else {
                    errors.push_back("<" + tagName + ">: '" + text + "'. Очікується число.");
                }
            } // Не додаємо помилку про відсутність
        }
    } // Не додаємо помилку, якщо немає <graphicsPreferences>

    return errors;
}
