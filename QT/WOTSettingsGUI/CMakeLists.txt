# Мінімальна необхідна версія CMake
cmake_minimum_required(VERSION 3.19)

# Назва проєкту та мови
project(WOTSettingsGUI LANGUAGES CXX)

# Пошук пакету Qt6 з необхідними компонентами
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets LinguistTools Network)

# Стандартні налаштування для проєктів Qt
qt_standard_project_setup()

# Визначення виконуваного файлу та ВСІХ його вихідних файлів
# Додаємо .cpp, .h, .hpp, .ui та згенерований .h
qt_add_executable(WOTSettingsGUI
    WIN32 MACOSX_BUNDLE # Стандартні параметри для Windows/macOS
    # Файли GUI
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui

    # Файли Бекенду (в корені проєкту)
    AppInitializer.cpp
    BackupManager.cpp
    ChangeTracker.cpp
    ConfigEditor.cpp
    ConfigManager.cpp
    FileValidator.cpp
    ProfileManager.cpp
    main.h             # Головний заголовок бекенду

    # Pugixml
    pugixml/pugixml.cpp
    pugixml/pugixml.hpp    # Додаємо, щоб CMake відстежував зміни
    pugixml/pugiconfig.hpp # Додаємо, щоб CMake відстежував зміни

    # Згенерований хедер з даними XML
    preferences_data.h
    configeditdialog.h configeditdialog.cpp configeditdialog.ui # Залишено дублікат, як у вас
    settingdelegate.h
    settingdelegate.cpp
    configeditdialog.h configeditdialog.cpp configeditdialog.ui # Залишено дублікат, як у вас
    helpdialog.h helpdialog.cpp helpdialog.ui # Залишено .ui, як у вас
    statsdialog.h statsdialog.cpp statsdialog.ui
    aichatdialog.h aichatdialog.cpp aichatdialog.ui
    resources.rc
    resources.qrc # Залишено .ui, як у вас

    # Важливо для AppInitializer.cpp
)

# Вказуємо папки, де шукати заголовкові файли
target_include_directories(WOTSettingsGUI PRIVATE
    .                  # Поточна директорія (корінь проєкту)
    pugixml            # Папка pugixml
)

# Налаштування перекладів (якщо використовуєш)
qt_add_translations(
    TARGETS WOTSettingsGUI
    TS_FILES WOTSettingsGUI_en_001.ts # Або твій файл перекладу
)

# Лінкування НЕОБХІДНИХ бібліотек Qt
# !!! ДОДАНО Qt::Network !!!
target_link_libraries(WOTSettingsGUI PRIVATE
    Qt::Core
    Qt::Widgets
    Qt::Network
)

# Налаштування встановлення (стандартні)
include(GNUInstallDirs)
install(TARGETS WOTSettingsGUI
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Генерація скрипту для розгортання (стандартна)
qt_generate_deploy_app_script(
    TARGET WOTSettingsGUI
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

# Додатково: Встановлення стандартної версії C++ (рекомендовано)
# Додай це перед qt_add_executable або після project()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Вимкнути розширення компілятора (добра практика)
