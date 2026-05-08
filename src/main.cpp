#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QDebug>
#include <iostream>

#include "ProjectGenerator.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("WebApp Converter");
    app.setApplicationVersion("1.0.0");

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Convert web content (URLs or HTML/CSS/JS files) into standalone Debian desktop applications.\n"
        "The generated app uses Qt WebEngine (Chromium) with its own browser engine.\n\n"
        "Examples:\n"
        "  webapp-converter --url https://example.com --name MyApp\n"
        "  webapp-converter --dir ./my-web-app/ --name MyApp --icon icon.png\n"
        "  webapp-converter --url https://example.com --name MyApp --build");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOption(QCommandLineOption("url", "URL of the web application to convert", "url"));
    parser.addOption(QCommandLineOption("dir", "Local directory with HTML/CSS/JS files", "dir"));
    parser.addOption(QCommandLineOption("name", "Application name (required)", "name"));
    parser.addOption(QCommandLineOption("icon", "Path to application icon (PNG/SVG)", "icon"));
    parser.addOption(QCommandLineOption("output", "Output directory (default: ./<name>)", "dir"));
    parser.addOption(QCommandLineOption("app-version", "Application version (default: 1.0.0)", "app-ver"));
    parser.addOption(QCommandLineOption("build", "Build the generated project after creation"));

    parser.process(app);

    if (!parser.isSet("name")) {
        std::cerr << "Error: --name is required\n";
        parser.showHelp(1);
        return 1;
    }

    if (!parser.isSet("url") && !parser.isSet("dir")) {
        std::cerr << "Error: Either --url or --dir is required\n";
        parser.showHelp(1);
        return 1;
    }

    GenerateOptions opts;
    opts.appName = parser.value("name");
    opts.appVersion = parser.value("app-version");
    if (opts.appVersion.isEmpty()) opts.appVersion = "1.0.0";
    opts.appOrg = opts.appName;
    opts.targetUrl = parser.value("url");
    opts.sourceDir = parser.value("dir");
    opts.iconPath = parser.value("icon");
    opts.outputDir = parser.value("output");
    opts.buildAfterGen = parser.isSet("build");

    if (!opts.iconPath.isEmpty() && !QFileInfo::exists(opts.iconPath)) {
        std::cerr << "Error: Icon file not found: " << opts.iconPath.toStdString() << "\n";
        return 1;
    }
    if (!opts.sourceDir.isEmpty() && !QDir(opts.sourceDir).exists()) {
        std::cerr << "Error: Source directory not found: " << opts.sourceDir.toStdString() << "\n";
        return 1;
    }

    std::cout << "WebApp Converter v" << app.applicationVersion().toStdString() << "\n";
    std::cout << "----------------------------------------\n";

    if (!opts.targetUrl.isEmpty())
        std::cout << "URL:    " << opts.targetUrl.toStdString() << "\n";
    if (!opts.sourceDir.isEmpty())
        std::cout << "Source: " << opts.sourceDir.toStdString() << "\n";
    std::cout << "Name:   " << opts.appName.toStdString() << "\n";
    std::cout << "Output: " << (opts.outputDir.isEmpty() ? opts.appName : opts.outputDir).toStdString() << "\n";
    std::cout << "----------------------------------------\n";

    ProjectGenerator gen;
    if (!gen.generate(opts)) {
        std::cerr << "Error: " << gen.lastError().toStdString() << "\n";
        return 1;
    }

    QString outPath = opts.outputDir.isEmpty() ? opts.appName : opts.outputDir;
    QString absOut = QDir(outPath).absolutePath();
    QString execName = opts.appName.toLower().replace(QRegularExpression("[^a-z0-9]"), "-");
    std::cout << "Project generated successfully!\n";
    std::cout << "Location: " << absOut.toStdString() << "\n";
    std::cout << "\n";
    std::cout << "To build:\n";
    std::cout << "  cd \"" << absOut.toStdString() << "\" && ./build.sh\n";
    std::cout << "To build .deb package:\n";
    std::cout << "  cd \"" << absOut.toStdString() << "\" && ./build-deb.sh\n";
    std::cout << "\n";
    std::cout << "To run the generated app:\n";
    std::cout << "  cd \"" << absOut.toStdString() << "/build\" && ./" << execName.toStdString() << "\n";

    return 0;
}
