cmake_minimum_required(VERSION 2.8.12)
include(WebKitCommon)
set_property(DIRECTORY . PROPERTY FOLDER "WebKit")

set(WebKit_SOURCES
    Storage/StorageAreaImpl.cpp
    Storage/StorageAreaSync.cpp
    Storage/StorageNamespaceImpl.cpp
    Storage/StorageSyncManager.cpp
    Storage/StorageThread.cpp
    Storage/StorageTracker.cpp
    Storage/WebDatabaseProvider.cpp
    Storage/WebStorageNamespaceProvider.cpp

    WebCoreSupport/WebResourceLoadScheduler.cpp
    WebCoreSupport/WebViewGroup.cpp
)

set(WebKit_INCLUDE_DIRECTORIES
    "${CMAKE_BINARY_DIR}"
    "${WEBKIT_DIR}/Storage"
    "${WEBKIT_DIR}/WebCoreSupport"
    "${FORWARDING_HEADERS_DIR}/JavaScriptCore"
    "${FORWARDING_HEADERS_DIR}/WebCore"
    "${FORWARDING_HEADERS_DIR}/WebKitLegacy"
)

set(WebKit_SYSTEM_INCLUDE_DIRECTORIES
    "${ICU_INCLUDE_DIRS}"
)

set(WebKit_LIBRARIES
    PRIVATE JavaScriptCore${DEBUG_SUFFIX}
    PRIVATE WebCore${DEBUG_SUFFIX}
    PRIVATE PAL${DEBUG_SUFFIX}
)

WEBKIT_INCLUDE_CONFIG_FILES_IF_EXISTS()

if (MSVC)
    ADD_PRECOMPILED_HEADER("WebKitPrefix.h" "win/WebKitPrefix.cpp" WebKit_SOURCES)
    list(APPEND WebKit_SOURCES win/WebKit.resources/WebKit.rc)
    add_definitions(/DWEBKIT_EXPORTS /DFRAMEWORK_NAME=WebKit)
endif ()

WEBKIT_FRAMEWORK(WebKit)

add_dependencies(WebKit WebCore)
if (TARGET WebKitGUID)
    add_dependencies(WebKit WebKitGUID)
endif ()

if (APPLE)
    set_target_properties(WebKit PROPERTIES LINK_FLAGS "-umbrella WebKit")
endif ()

if (MSVC)
    add_custom_command(
        TARGET WebKit
        POST_BUILD
        COMMAND ${PERL_EXECUTABLE} ${WEBKIT_LIBRARIES_DIR}/tools/scripts/version-stamp.pl ${DERIVED_SOURCES_WEBKIT_DIR} $<TARGET_FILE:WebKit>
        VERBATIM)
endif ()
