#include "ProjectGenerator.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDirIterator>
#include <QProcess>
#include <QThread>
#include <QRegularExpression>
#include <QDebug>

ProjectGenerator::ProjectGenerator() {}

QString ProjectGenerator::lastError() const { return m_error; }

static QString appExecName(const QString &name)
{
    return name.toLower().replace(QRegularExpression("[^a-z0-9]"), "-");
}

bool ProjectGenerator::generate(const GenerateOptions &opts)
{
    if (opts.appName.isEmpty()) { m_error = "App name is required"; return false; }
    if (opts.targetUrl.isEmpty() && opts.sourceDir.isEmpty()) {
        m_error = "Either --url or --dir is required"; return false;
    }

    QString outDir = opts.outputDir.isEmpty() ? opts.appName : opts.outputDir;
    m_appDir = QDir(outDir);

    if (m_appDir.exists()) {
        m_error = "Output directory already exists: " + outDir;
        return false;
    }
    m_appDir.mkpath(".");
    m_appDir.mkpath("resources");

    bool ok = false;
    if (!opts.targetUrl.isEmpty())
        ok = generateFromUrl(opts);
    else
        ok = generateFromDir(opts);

    if (ok && opts.buildAfterGen)
        ok = buildGeneratedProject(opts);

    return ok;
}

bool ProjectGenerator::generateFromUrl(const GenerateOptions &opts)
{
    GenerateOptions o = opts;
    writeFile(m_appDir.filePath("CMakeLists.txt"), processTemplate(templateCMakeLists(), o));
    writeFile(m_appDir.filePath("main.cpp"), processTemplate(templateMainCpp(), o));
    writeFile(m_appDir.filePath("MainWindow.h"), processTemplate(templateMainWindowHeader(), o));
    writeFile(m_appDir.filePath("MainWindow.cpp"), processTemplate(templateMainWindowCpp(), o));
    writeFile(m_appDir.filePath("resources/app.qrc"), processTemplate(templateQrc(), o));
    writeFile(m_appDir.filePath(appExecName(o.appName) + ".desktop"), processTemplate(templateDesktopFile(), o));
    writeFile(m_appDir.filePath("build.sh"), processTemplate(templateBuildSh(), o));
    writeFile(m_appDir.filePath("build-deb.sh"), processTemplate(templateBuildDeb(), o));

    QString iconFile = o.iconPath.isEmpty() ? "app.svg" : QFileInfo(o.iconPath).fileName();
    if (!o.iconPath.isEmpty())
        QFile::copy(o.iconPath, m_appDir.filePath("resources/" + iconFile));
    else
        writeFile(m_appDir.filePath("resources/app.svg"), defaultSvgIcon());

    QFile::setPermissions(m_appDir.filePath("build.sh"),
        QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    QFile::setPermissions(m_appDir.filePath("build-deb.sh"),
        QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);

    return true;
}

bool ProjectGenerator::generateFromDir(const GenerateOptions &opts)
{
    GenerateOptions o = opts;
    o.targetUrl = "qrc:///web/index.html";

    QDir srcDir(opts.sourceDir);
    if (!srcDir.exists()) { m_error = "Source directory not found"; return false; }

    m_appDir.mkpath("resources");
    if (!copyDirectory(opts.sourceDir, m_appDir.filePath("resources"))) return false;

    QString iconFile = o.iconPath.isEmpty() ? "app.svg" : QFileInfo(o.iconPath).fileName();

    writeFile(m_appDir.filePath("CMakeLists.txt"), processTemplate(templateCMakeLists(), o));
    writeFile(m_appDir.filePath("main.cpp"), processTemplate(templateMainCpp(), o));
    writeFile(m_appDir.filePath("MainWindow.h"), processTemplate(templateMainWindowHeader(), o));
    writeFile(m_appDir.filePath("MainWindow.cpp"), processTemplate(templateMainWindowCpp(), o));
    writeFile(m_appDir.filePath("resources/app.qrc"), buildQrcForDir(m_appDir.filePath("resources"), iconFile));
    writeFile(m_appDir.filePath(appExecName(o.appName) + ".desktop"), processTemplate(templateDesktopFile(), o));
    writeFile(m_appDir.filePath("build.sh"), processTemplate(templateBuildSh(), o));
    writeFile(m_appDir.filePath("build-deb.sh"), processTemplate(templateBuildDeb(), o));

    if (!o.iconPath.isEmpty())
        QFile::copy(o.iconPath, m_appDir.filePath("resources/" + iconFile));
    else
        writeFile(m_appDir.filePath("resources/app.svg"), defaultSvgIcon());

    QFile::setPermissions(m_appDir.filePath("build.sh"),
        QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    QFile::setPermissions(m_appDir.filePath("build-deb.sh"),
        QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);

    return true;
}

bool ProjectGenerator::buildGeneratedProject(const GenerateOptions &opts)
{
    Q_UNUSED(opts);
    QString buildDir = m_appDir.filePath("build");
    QDir().mkpath(buildDir);

    QProcess cmake;
    cmake.setWorkingDirectory(buildDir);
    cmake.start("cmake", {m_appDir.absolutePath(), "-DCMAKE_BUILD_TYPE=Release"});
    cmake.waitForFinished();
    if (cmake.exitCode() != 0) {
        m_error = "CMake failed: " + cmake.readAllStandardError();
        return false;
    }

    QProcess build;
    build.setWorkingDirectory(buildDir);
    build.start("cmake", {"--build", ".", "--parallel", QString::number(QThread::idealThreadCount())});
    build.waitForFinished();
    if (build.exitCode() != 0) {
        m_error = "Build failed: " + build.readAllStandardError();
        return false;
    }
    return true;
}

QString ProjectGenerator::buildQrcForDir(const QString &dirPath, const QString &iconFile)
{
    QDir dir(dirPath);
    QStringList files;
    collectQrcFiles(dir, "", files);

    QString qrc;
    qrc += "<RCC>\n";
    qrc += "    <qresource prefix=\"/\">\n";
    qrc += "        <file>" + iconFile + "</file>\n";
    qrc += "    </qresource>\n";
    if (!files.isEmpty()) {
        qrc += "    <qresource prefix=\"/web\">\n";
        for (const QString &f : files)
            qrc += "        <file>" + f + "</file>\n";
        qrc += "    </qresource>\n";
    }
    qrc += "</RCC>\n";
    return qrc;
}

void ProjectGenerator::collectQrcFiles(const QDir &dir, const QString &prefix, QStringList &out)
{
    QStringList entries = dir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &entry : entries) {
        QString relPath = prefix.isEmpty() ? entry : prefix + "/" + entry;
        if (QFileInfo(dir.absoluteFilePath(entry)).isDir())
            collectQrcFiles(QDir(dir.absoluteFilePath(entry)), relPath, out);
        else
            out.append(relPath);
    }
}

bool ProjectGenerator::copyDirectory(const QString &src, const QString &dst)
{
    QDir srcDir(src);
    QDir dstDir(dst);
    if (!dstDir.exists()) dstDir.mkpath(".");

    QStringList entries = srcDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &entry : entries) {
        QString srcPath = srcDir.absoluteFilePath(entry);
        QString dstPath = dstDir.absoluteFilePath(entry);
        if (QFileInfo(srcPath).isDir()) {
            if (!copyDirectory(srcPath, dstPath)) return false;
        } else {
            if (!QFile::copy(srcPath, dstPath)) return false;
        }
    }
    return true;
}

bool ProjectGenerator::writeFile(const QString &path, const QString &content)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_error = "Failed to write: " + path;
        return false;
    }
    QTextStream s(&file);
    s << content;
    file.close();
    return true;
}

QString ProjectGenerator::processTemplate(const QString &tmpl, const GenerateOptions &opts)
{
    QString result = tmpl;
    result.replace("{{APP_NAME}}", opts.appName);
    result.replace("{{APP_NAME_EXEC}}", appExecName(opts.appName));
    result.replace("{{APP_VERSION}}", opts.appVersion);
    result.replace("{{APP_ORG}}", opts.appOrg.isEmpty() ? opts.appName : opts.appOrg);
    result.replace("{{APP_URL}}", opts.targetUrl);
    QString iconFile = opts.iconPath.isEmpty() ? "app.svg" : QFileInfo(opts.iconPath).fileName();
    result.replace("{{APP_ICON}}", iconFile);
    result.replace("{{WINDOW_WIDTH}}", "1024");
    result.replace("{{WINDOW_HEIGHT}}", "768");
    return result;
}

// ============ TEMPLATES (safe raw string literals with ~~ delimiter) ============

QString ProjectGenerator::templateCMakeLists()
{
    return R"~~(
cmake_minimum_required(VERSION 3.21)
project({{APP_NAME_EXEC}} VERSION {{APP_VERSION}} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets WebEngineWidgets PrintSupport)

qt_add_executable({{APP_NAME_EXEC}}
    main.cpp
    MainWindow.h MainWindow.cpp
    resources/app.qrc
)

target_link_libraries({{APP_NAME_EXEC}} PRIVATE
    Qt6::Widgets Qt6::WebEngineWidgets Qt6::PrintSupport
)

install(TARGETS {{APP_NAME_EXEC}} RUNTIME DESTINATION bin)
install(FILES resources/{{APP_ICON}} DESTINATION share/icons/hicolor/scalable/apps)
install(FILES {{APP_NAME_EXEC}}.desktop DESTINATION share/applications)

set(CPACK_GENERATOR DEB)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "{{APP_ORG}}")
set(CPACK_DEBIAN_PACKAGE_SECTION web)
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "{{APP_NAME_EXEC}}-${PROJECT_VERSION}-linux-amd64")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64)
set(CPACK_STRIP_FILES TRUE)
include(CPack)
)~~";
}

QString ProjectGenerator::templateMainCpp()
{
    return R"~~(
#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("{{APP_NAME}}");
    app.setApplicationVersion("{{APP_VERSION}}");
    app.setOrganizationName("{{APP_ORG}}");
    app.setOrganizationDomain("{{APP_NAME_EXEC}}.local");

    QString dataDir = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation) + "/storage";
    QDir().mkpath(dataDir);

    QWebEngineProfile *profile = new QWebEngineProfile("{{APP_NAME_EXEC}}-profile", &app);
    profile->setPersistentStoragePath(dataDir);
    profile->setPersistentCookiesPolicy(
        QWebEngineProfile::ForcePersistentCookies);
    profile->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
    profile->setHttpCacheMaximumSize(100 * 1024 * 1024);

    QWebEngineSettings *s = profile->settings();
    s->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    s->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    s->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    s->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    s->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    s->setAttribute(QWebEngineSettings::AutoLoadImages, true);
    s->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    s->setAttribute(QWebEngineSettings::ErrorPageEnabled, true);

    MainWindow window;
    QWebEnginePage *page = new QWebEnginePage(profile, &window);
    window.setPage(page);
    window.load("{{APP_URL}}");
    window.show();

    return app.exec();
}
)~~";
}

QString ProjectGenerator::templateMainWindowHeader()
{
    return R"~~(
#pragma once

#include <QMainWindow>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QLabel>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    void setPage(QWebEnginePage *page);
    void load(const QString &url);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onTitleChanged(const QString &title);

private:
    QWebEngineView *m_view;
    QWebEnginePage *m_page;
    QLabel *m_statusLabel;
};
)~~";
}

QString ProjectGenerator::templateMainWindowCpp()
{
    return R"~~(
#include "MainWindow.h"

#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineFullScreenRequest>
#include <QCloseEvent>
#include <QSettings>
#include <QAction>
#include <QFileDialog>
#include <QStatusBar>
#include <QDir>
#include <QApplication>
#include <QWebEngineCookieStore>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_view(nullptr)
    , m_page(nullptr)
    , m_statusLabel(nullptr)
{
    setWindowTitle("{{APP_NAME}}");
    setMinimumSize(800, 600);
    resize({{WINDOW_WIDTH}}, {{WINDOW_HEIGHT}});

    m_view = new QWebEngineView(this);
    setCentralWidget(m_view);

    m_statusLabel = new QLabel(tr("Ready"));
    statusBar()->addPermanentWidget(m_statusLabel);

    connect(m_view, &QWebEngineView::titleChanged,
            this, &MainWindow::onTitleChanged);
    connect(m_view, &QWebEngineView::loadStarted, this, [this]() {
        m_statusLabel->setText(tr("Loading..."));
    });
    connect(m_view, &QWebEngineView::loadFinished, this, [this](bool ok) {
        m_statusLabel->setText(ok ? tr("Done") : tr("Failed"));
    });
}

void MainWindow::setPage(QWebEnginePage *page)
{
    m_page = page;
    m_view->setPage(page);

    connect(page, &QWebEnginePage::fullScreenRequested, this,
        [this](QWebEngineFullScreenRequest req) {
            req.toggleOn() ? showFullScreen() : showNormal();
            req.accept();
        });

    QAction *actFullscreen = new QAction(tr("Fullscreen"), this);
    actFullscreen->setShortcut(QKeySequence(Qt::Key_F11));
    connect(actFullscreen, &QAction::triggered, this, [this]() {
        isFullScreen() ? showNormal() : showFullScreen();
    });
    addAction(actFullscreen);

    QAction *actPrint = new QAction(tr("Print / PDF"), this);
    actPrint->setShortcut(QKeySequence::Print);
    connect(actPrint, &QAction::triggered, this, [this]() {
        QString path = QFileDialog::getSaveFileName(
            this, tr("Save PDF"),
            QDir::homePath() + "/{{APP_NAME_EXEC}}.pdf",
            tr("PDF Files (*.pdf)"));
        if (!path.isEmpty()) {
            m_view->page()->printToPdf(path);
        }
    });
    addAction(actPrint);

    QAction *actReload = new QAction(tr("Reload"), this);
    actReload->setShortcut(QKeySequence::Refresh);
    connect(actReload, &QAction::triggered,
            m_view, &QWebEngineView::reload);
    addAction(actReload);

    QAction *actBack = new QAction(tr("Back"), this);
    actBack->setShortcut(QKeySequence::Back);
    connect(actBack, &QAction::triggered,
            m_view, &QWebEngineView::back);
    addAction(actBack);

    QAction *actForward = new QAction(tr("Forward"), this);
    actForward->setShortcut(QKeySequence::Forward);
    connect(actForward, &QAction::triggered,
            m_view, &QWebEngineView::forward);
    addAction(actForward);

    QSettings settings;
    restoreGeometry(settings.value("geometry").toByteArray());
}

void MainWindow::load(const QString &url)
{
    m_view->load(QUrl(url));
}

void MainWindow::onTitleChanged(const QString &title)
{
    if (!title.isEmpty())
        setWindowTitle(title + " - {{APP_NAME}}");
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings;
    settings.setValue("geometry", saveGeometry());
    QMainWindow::closeEvent(event);
}
)~~";
}

QString ProjectGenerator::templateQrc()
{
    return R"~~(
<RCC>
    <qresource prefix="/">
        <file>{{APP_ICON}}</file>
    </qresource>
    <qresource prefix="/web">
    </qresource>
</RCC>
)~~";
}

QString ProjectGenerator::templateDesktopFile()
{
    return R"~~(
[Desktop Entry]
Type=Application
Name={{APP_NAME}}
Comment=Web application converted with WebApp Converter
Exec={{APP_NAME_EXEC}}
Icon={{APP_ICON}}
Categories=Network;Web;Utility;
Terminal=false
StartupNotify=true
)~~";
}

QString ProjectGenerator::templateBuildSh()
{
    return R"~~(
#!/bin/bash
set -e
BUILD_DIR="build"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build . --parallel "$(nproc)"
echo ""
echo "Build complete!"
echo "Run: ./$BUILD_DIR/{{APP_NAME_EXEC}}"
)~~";
}

QString ProjectGenerator::templateBuildDeb()
{
    return R"~~(
#!/bin/bash
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build-deb"

echo "Building {{APP_NAME}} .deb package..."
sudo apt-get install -y build-essential cmake qt6-base-dev qt6-webengine-dev 2>/dev/null || true

rm -rf "$BUILD_DIR" && mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake "$SCRIPT_DIR" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build . --parallel "$(nproc)"
cpack -G DEB

echo "Package: $(ls *.deb)"
)~~";
}

QString ProjectGenerator::defaultSvgIcon()
{
    return R"~~(
<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512">
  <defs>
    <linearGradient id="bg" x1="0%" y1="0%" x2="100%" y2="100%">
      <stop offset="0%" style="stop-color:#1a1a2e"/>
      <stop offset="100%" style="stop-color:#0f3460"/>
    </linearGradient>
    <linearGradient id="glow" x1="0%" y1="0%" x2="100%" y2="100%">
      <stop offset="0%" style="stop-color:#00d2ff"/>
      <stop offset="100%" style="stop-color:#3a7bd5"/>
    </linearGradient>
  </defs>
  <rect width="512" height="512" rx="80" fill="url(#bg)"/>
  <g transform="translate(256,240)">
    <rect x="-70" y="-30" width="140" height="100" rx="12" fill="none" stroke="url(#glow)" stroke-width="6"/>
    <path d="M-50,10 L-20,40 L20,10" fill="none" stroke="url(#glow)" stroke-width="5"/>
    <line x1="0" y1="40" x2="0" y2="60" stroke="url(#glow)" stroke-width="5"/>
    <line x1="-20" y1="75" x2="20" y2="75" stroke="url(#glow)" stroke-width="4"/>
  </g>
  <text x="256" y="380" text-anchor="middle" fill="#888" font-family="sans-serif" font-size="28">WebApp</text>
</svg>
)~~";
}
