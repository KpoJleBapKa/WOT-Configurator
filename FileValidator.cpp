#include "main.h" // Або "FileValidator.h", якщо ти створив окремий файл
#include "pugixml/pugixml.hpp" // Переконайся, що шлях правильний
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <limits>      // Для numeric_limits
#include <cstdlib>     // Для getenv
#include <sstream>     // Для tryParseFloat/Int

// Використання просторів імен
using namespace std;
namespace fs = std::filesystem;

// --- Допоміжна функція для безпечної конвертації рядка в число (float) ---
bool tryParseFloat(const char* text, float& outValue) {
    if (!text) return false;
    std::istringstream iss(text);
    // Дозволяємо читати, навіть якщо не весь рядок розібрано (на випадок пробілів)
    iss >> outValue;
    // Перевіряємо лише на помилку читання
    return !iss.fail();
}

// --- Допоміжна функція для безпечної конвертації рядка в ціле число (int) ---
bool tryParseInt(const char* text, int& outValue) {
     if (!text) return false;
    std::istringstream iss(text);
    iss >> outValue;
    return !iss.fail();
}


// --- Метод runValidationWizard ---
// Керує процесом валідації через консоль
void FileValidator::runValidationWizard() {
    cout << "\n--- Validate Config File ---" << endl;
    cout << "Select directory to validate files from:" << endl;
    cout << "1. User Configs" << endl;
    cout << "2. Game Config Directory (checks preferences.xml)" << endl;
    cout << "0. Cancel" << endl;
    cout << "Enter choice: ";
    int dirChoice = -1;

    // Читання вибору директорії
    while (!(cin >> dirChoice) || dirChoice < 0 || dirChoice > 2) {
         cout << "Invalid input. Please enter a number between 0 and 2: ";
         cin.clear();
         cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Очистка буфера

    fs::path fileToValidate; // Шлях до файлу, який будемо валідувати

    if (dirChoice == 1) {
        fs::path sourceDir = "User Configs";
        cout << "\nAvailable files in '" << sourceDir.string() << "':" << endl;
        vector<fs::path> configFiles;
        try {
            if (!fs::exists(sourceDir) || !fs::is_directory(sourceDir)) {
                 cerr << "Error: Directory not found: " << sourceDir.string() << endl;
            } else {
                 for (const auto& entry : fs::directory_iterator(sourceDir)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".xml") {
                        configFiles.push_back(entry.path());
                    }
                 }
            }
        } catch (const fs::filesystem_error& e) {
             cerr << "Error reading directory '" << sourceDir.string() << "': " << e.what() << endl;
        }

        if (configFiles.empty()) {
            cout << "No config files found to validate." << endl;
        } else {
            for (size_t i = 0; i < configFiles.size(); ++i) {
                cout << i + 1 << ". " << configFiles[i].filename().string() << endl;
            }
            cout << "0. Cancel" << endl;
            cout << "Enter the number of the file to validate: ";
            int fileChoice = -1;
            // Читання вибору файлу
             while (!(cin >> fileChoice) || fileChoice < 0 || fileChoice > static_cast<int>(configFiles.size())) {
                 cout << "Invalid input. Please enter a number between 0 and " << configFiles.size() << ": ";
                 cin.clear();
                 cin.ignore(numeric_limits<streamsize>::max(), '\n');
            }
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            if (fileChoice > 0) {
                fileToValidate = configFiles[fileChoice - 1];
            } else {
                 cout << "Validation cancelled." << endl;
            }
        }

    } else if (dirChoice == 2) {
         const char* appDataPath = getenv("APPDATA");
         if (appDataPath) {
              fileToValidate = fs::path(appDataPath) / "Wargaming.net" / "WorldOfTanks" / "preferences.xml";
              if (!fs::exists(fileToValidate)) {
                   cerr << "Error: preferences.xml not found in game directory: " << fileToValidate.string() << endl;
                   fileToValidate.clear(); // Скидаємо шлях, якщо файлу немає
              }
         } else {
              cerr << "Error: Could not get APPDATA path." << endl;
         }
    } else {
         cout << "Validation cancelled." << endl;
    }

    // --- Запуск валідації, якщо файл вибрано ---
    if (!fileToValidate.empty()) {
        cout << "\n--- Running Validation for: " << fileToValidate.string() << " ---" << endl;

        // Викликаємо інші методи цього ж класу
        bool isWellFormed = this->isXmlWellFormed(fileToValidate);
        cout << "1. XML Well-Formed Check: " << (isWellFormed ? "Passed" : "Failed (See error message above)") << endl;

        if (isWellFormed) {
            bool hasStructure = this->hasExpectedStructure(fileToValidate);
            cout << "2. Expected Structure Check: " << (hasStructure ? "Passed (Basic structure seems OK)" : "Warnings/Failed (Check messages above)") << endl;

            // Запускаємо перевірку значень незалежно від результату перевірки структури (якщо XML коректний)
            vector<string> valueErrors = this->findInvalidSimpleValues(fileToValidate);
            cout << "3. Simple Value Validation Check: ";
            if (valueErrors.empty()) {
                cout << "Passed (No obvious errors in simple values found)" << endl;
            } else {
                cout << "Found " << valueErrors.size() << " potential issue(s):" << endl;
                for (const string& err : valueErrors) {
                    cout << "   - " << err << endl;
                }
            }
        } else {
             cout << "Skipping structure and value checks due to XML parsing error." << endl;
        }
        cout << "--- Validation Finished ---" << endl;
    }
}


// --- Реалізація isXmlWellFormed ---
// Перевіряє, чи є файл синтаксично коректним XML
bool FileValidator::isXmlWellFormed(const fs::path& filePath) {
    if (!fs::exists(filePath)) { // Перевіряємо існування перед is_regular_file
        cerr << "Validation Error: File not found: " << filePath.string() << endl;
        return false;
    }
     if (!fs::is_regular_file(filePath)) {
         cerr << "Validation Error: Path is not a regular file: " << filePath.string() << endl;
         return false;
     }


    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(filePath.c_str(), pugi::parse_default | pugi::parse_declaration); // Додамо прапор для можливого <?xml ...?>

    if (result.status != pugi::status_ok) {
        cerr << "XML Parsing Error: " << result.description() << " at offset " << result.offset << endl;
        // Не виводимо шлях тут, бо він буде виведений у runValidationWizard
        return false;
    }
    return true;
}

// --- Реалізація hasExpectedStructure ---
// Перевіряє наявність основних очікуваних секцій WoT
bool FileValidator::hasExpectedStructure(const fs::path& filePath) {
    pugi::xml_document doc;
    // Не перевіряємо результат load_file ще раз, бо це має зробити isXmlWellFormed перед викликом
    if (!doc.load_file(filePath.c_str())) {
        // Цей випадок не мав би статися, якщо isXmlWellFormed викликався першим
        return false;
    }

    bool structureOk = true;
    auto root = doc.child("root"); // Використовуємо auto для зручності

    if (!root) {
        cerr << "Structure Error: Missing <root> element." << endl;
        return false; // Без <root> далі немає сенсу перевіряти
    }

    if (!root.child("scriptsPreferences")) {
         cerr << "Structure Warning: Missing <scriptsPreferences> section." << endl;
         structureOk = false; // Вважаємо це помилкою структури
    }
     if (!root.child("graphicsPreferences")) {
         cerr << "Structure Warning: Missing <graphicsPreferences> section." << endl;
         structureOk = false;
    }
     if (!root.child("devicePreferences")) {
         cerr << "Structure Warning: Missing <devicePreferences> section." << endl;
         structureOk = false;
    }

    // Можна додати перевірки глибше, якщо потрібно
    // if (root.child("scriptsPreferences") && !root.child("scriptsPreferences").child("soundPrefs")) {
    //     cerr << "Structure Warning: Missing <soundPrefs> inside <scriptsPreferences>." << endl;
    // }

    return structureOk;
}


// --- Реалізація findInvalidSimpleValues ---
// Знаходить прості значення, які виходять за межі очікуваних діапазонів
// --- FileValidator.cpp ---
// ... (includes, namespaces, tryParseFloat/Int, isXmlWellFormed, hasExpectedStructure) ...

vector<string> FileValidator::findInvalidSimpleValues(const fs::path& filePath) {
    vector<string> errors;
    pugi::xml_document doc;

    if (!doc.load_file(filePath.c_str())) {
         errors.push_back("Failed to load XML file for value validation.");
        return errors;
    }

    auto root = doc.child("root");
    if (!root) {
        errors.push_back("Missing <root> element, cannot validate values.");
        return errors;
    }

    // 1. Перевірка <soundPrefs>
    auto scriptsPrefs = root.child("scriptsPreferences");
    if (scriptsPrefs) {
        auto soundPrefs = scriptsPrefs.child("soundPrefs");
        if (soundPrefs) {
            const vector<string> volumeTags = {
                "masterVolume", "volume_micVivox", "volume_vehicles", "volume_music",
                "volume_effects", "volume_ambient", "volume_gui", "volume_voice",
                "volume_masterFadeVivox", "volume_masterVivox", "volume_music_hangar",
                "volume_ev_ambient", "volume_ev_effects", "volume_ev_gui",
                "volume_ev_music", "volume_ev_vehicles", "volume_ev_voice"
            };
            for (const string& tagName : volumeTags) {
                pugi::xml_node node = soundPrefs.child(tagName.c_str());
                if (node) {
                    float value;
                    const char* text = node.text().as_string();
                    if (tryParseFloat(text, value)) {
                        if (value < 0.0f || value > 1.0f) {
                            // ВИПРАВЛЕНО: Використовуємо string() для конкатенації
                            errors.push_back(string("Invalid value for <") + tagName + ">: '" + text + "'. Expected range [0.0, 1.0].");
                        }
                    } else {
                        // ВИПРАВЛЕНО: Використовуємо string()
                        errors.push_back(string("Invalid format for <") + tagName + ">: '" + text + "'. Expected a number.");
                    }
                }
            }
        } else {
            errors.push_back("Missing <soundPrefs> section inside <scriptsPreferences>.");
        }

         // 2. Перевірка <fov>
          auto fovNode = scriptsPrefs.child("fov");
           if(fovNode) {
                float value;
                const char* text = fovNode.text().as_string();
                if(tryParseFloat(text, value)) {
                    if (value < 50.0f || value > 130.0f) {
                         // ВИПРАВЛЕНО: Використовуємо string()
                         errors.push_back(string("Suspicious value for <fov>: '") + text + "'. Typical range [60.0, 120.0].");
                    }
                } else {
                     // ВИПРАВЛЕНО: Використовуємо string()
                     errors.push_back(string("Invalid format for <fov>: '") + text + "'. Expected a number.");
                }
           } else {
                errors.push_back("Missing <fov> tag inside <scriptsPreferences>.");
           }
    } else {
        errors.push_back("Missing <scriptsPreferences> section, cannot check sound/fov.");
    }


     // 3. Перевірка <devicePreferences>
     auto devicePrefs = root.child("devicePreferences");
     if (devicePrefs) {
         // Window Mode
         pugi::xml_node wmNode = devicePrefs.child("windowMode");
         if (wmNode) {
             int value;
             const char* text = wmNode.text().as_string();
             if(tryParseInt(text, value)) {
                 if (value < 0 || value > 2) {
                      // ВИПРАВЛЕНО: Використовуємо string()
                      errors.push_back(string("Invalid value for <windowMode>: '") + text + "'. Expected 0, 1, or 2.");
                 }
             } else {
                 // ВИПРАВЛЕНО: Використовуємо string()
                 errors.push_back(string("Invalid format for <windowMode>: '") + text + "'. Expected an integer.");
             }
         } else {
              errors.push_back("Missing <windowMode> tag in <devicePreferences>.");
         }

         // Resolutions
         const vector<pair<string, int>> resolutionTags = {
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
                           // ВИПРАВЛЕНО: Використовуємо string() та to_string() для числа
                           errors.push_back(string("Invalid value for <") + pair.first + ">: '" + text + "'. Expected >= " + to_string(pair.second) + ".");
                      }
                  } else {
                       // ВИПРАВЛЕНО: Використовуємо string()
                      errors.push_back(string("Invalid format for <") + pair.first + ">: '" + text + "'. Expected an integer.");
                  }
              } else {
                   errors.push_back(string("Missing <") + pair.first + "> tag in <devicePreferences>.");
              }
         }
         // Refresh Rate
         pugi::xml_node rrNode = devicePrefs.child("fullscreenRefresh");
          if(rrNode) {
              int value;
              const char* text = rrNode.text().as_string();
              if(tryParseInt(text, value)) {
                  if (value < 10) {
                       // ВИПРАВЛЕНО: Використовуємо string()
                       errors.push_back(string("Suspiciously low value for <fullscreenRefresh>: '") + text + "'. Expected >= 50 typically.");
                  }
              } else {
                   // ВИПРАВЛЕНО: Використовуємо string()
                  errors.push_back(string("Invalid format for <fullscreenRefresh>: '") + text + "'. Expected an integer.");
              }
          } else {
                errors.push_back("Missing <fullscreenRefresh> tag in <devicePreferences>.");
          }
     } else {
         errors.push_back("Missing <devicePreferences> section.");
     }


      // 4. Перевірка графіки (прості значення)
      auto graphPrefs = root.child("graphicsPreferences");
       if(graphPrefs) {
           const vector<string> graphTags = {
               "colorGradingStrength", "brightnessDeferred", "contrastDeferred", "saturationDeferred"
           };
            for (const string& tagName : graphTags) {
                pugi::xml_node node = graphPrefs.child(tagName.c_str());
                 if (node) {
                    float value;
                    const char* text = node.text().as_string();
                    if (tryParseFloat(text, value)) {
                        if (value < 0.0f || value > 1.5f) {
                            // ВИПРАВЛЕНО: Використовуємо string()
                            errors.push_back(string("Suspicious value for <") + tagName + ">: '" + text + "'. Expected range typically around [0.0, 1.0].");
                        }
                    } else {
                         // ВИПРАВЛЕНО: Використовуємо string()
                         errors.push_back(string("Invalid format for <") + tagName + ">: '" + text + "'. Expected a number.");
                    }
                 }
            }
       } else {
            errors.push_back("Missing <graphicsPreferences> section.");
       }

    return errors;
}
