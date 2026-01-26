cmake_minimum_required(VERSION 3.16)

project(Galman LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Quick Concurrent Network ShaderTools)

qt_add_executable(galman
    src/main.cpp
        src/ScriptEngine.cpp
        src/ScriptManager.cpp
        src/LanguageManager.cpp
        src/ComfyClient.cpp
        src/ComfyWorkflowParser.cpp
        src/PlatformUtils.cpp
)

set_source_files_properties(qml/Theme.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)
set_source_files_properties(qml/Components/ImageAdjustmentsConstants.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(galman
    URI Galman
    VERSION 1.0
    SOURCES
        src/CopyWorker.h
        src/CopyWorker.cpp
        src/TrashWorker.h
        src/TrashWorker.cpp
        src/FolderBrowserModel.h
        src/FolderBrowserModel.cpp
        src/FolderCompareModel.h
        src/FolderCompareModel.cpp
        src/FolderCompareSideModel.h
        src/FolderCompareSideModel.cpp
        src/ImageAutoAdjuster.h
        src/ImageAutoAdjuster.cpp
        src/VolumeModel.h
        src/VolumeModel.cpp
        src/ScriptEngine.h
        src/ScriptManager.h
        src/LanguageManager.h
        src/ComfyClient.h
        src/ComfyWorkflowParser.h
        src/PlatformUtils.h
    QML_FILES
        qml/App/Main.qml
        qml/Theme.qml
        qml/Components/CommandPanel.qml
        qml/Components/FolderView.qml
        qml/Components/FolderItem.qml
        qml/Components/FolderViewGrid.qml
        qml/Components/FolderViewSortBar.qml
        qml/Components/FolderViewToolBar.qml
        qml/Components/FocusFrame.qml
        qml/Components/ImageDisplay.qml
        qml/Components/ImageCompare.qml
        qml/Components/ImagePreview.qml
        qml/Components/ImageAdjustmentsPanel.qml
        qml/Components/ImageAdjustmentsEffect.qml
        qml/Components/ImageAdjustmentsConstants.qml
        qml/Components/FocusRememberingScope.qml
        qml/Components/Placeholder.qml
        qml/Components/ConfirmDialog.qml
        qml/Components/RenameDialog.qml
        qml/Components/ShortcutsDialog.qml
        qml/Components/StatusBadge.qml
    RESOURCES
        qml/Assets/Galman.jpg
)

qt_add_shaders(galman "galman_shaders"
    PREFIX "/shaders"
    FILES
        qml/Shaders/ImageAdjustments.frag
        qml/Shaders/ImageBloomPass.frag
        qml/Shaders/ImageBloomBlur.frag
        qml/Shaders/ImageBloomComposite.frag
)

target_link_libraries(galman
    PRIVATE Qt6::Quick Qt6::Concurrent Qt6::Network
)

target_include_directories(galman PRIVATE src)

set(translation_files
    translations/galman_en.ts
    translations/galman_fr.ts
    translations/galman_es.ts
    translations/galman_it.ts
    translations/galman_ru.ts
    translations/galman_nl.ts
    translations/galman_zh.ts
)

find_program(LRELEASE_EXECUTABLE NAMES lrelease-qt6 lrelease)
if (LRELEASE_EXECUTABLE)
    set(qm_files)
    foreach(ts_file IN LISTS translation_files)
        get_filename_component(ts_name "${ts_file}" NAME_WE)
        set(qm_file "${CMAKE_CURRENT_BINARY_DIR}/translations/${ts_name}.qm")
        add_custom_command(
            OUTPUT "${qm_file}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/translations"
            COMMAND "${LRELEASE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/${ts_file}" -qm "${qm_file}"
            DEPENDS "${ts_file}"
            COMMENT "Generating ${qm_file}"
            VERBATIM
        )
        list(APPEND qm_files "${qm_file}")
    endforeach()

    add_custom_target(galman_translations ALL DEPENDS ${qm_files})
    add_dependencies(galman galman_translations)

    qt_add_resources(galman "translations"
        PREFIX "/i18n"
        BASE "${CMAKE_CURRENT_BINARY_DIR}/translations"
        FILES ${qm_files}
    )
else()
    message(WARNING "lrelease not found; translations will be skipped.")
endif()
