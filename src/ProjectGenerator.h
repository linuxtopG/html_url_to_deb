#pragma once

#include <QString>
#include <QStringList>
#include <QDir>

struct GenerateOptions {
    QString appName;
    QString appVersion = "1.0.0";
    QString appOrg;
    QString targetUrl;
    QString sourceDir;
    QString iconPath;
    QString outputDir;
    bool buildAfterGen = false;
};

class ProjectGenerator
{
public:
    ProjectGenerator();

    bool generate(const GenerateOptions &options);
    QString lastError() const;

private:
    bool generateFromUrl(const GenerateOptions &options);
    bool generateFromDir(const GenerateOptions &options);
    bool buildGeneratedProject(const GenerateOptions &opts);
    bool copyDirectory(const QString &src, const QString &dst);
    QString buildQrcForDir(const QString &dir, const QString &iconFile);
    void collectQrcFiles(const QDir &dir, const QString &prefix, QStringList &out);
    QString defaultSvgIcon();

    bool writeFile(const QString &path, const QString &content);
    QString readFile(const QString &path);

    QString processTemplate(const QString &tmpl, const GenerateOptions &opts);
    QString val(const QString &key, const GenerateOptions &opts) const;

    QString templateCMakeLists();
    QString templateMainCpp();
    QString templateMainWindowHeader();
    QString templateMainWindowCpp();
    QString templateQrc();
    QString templateDesktopFile();
    QString templateBuildSh();
    QString templateBuildDeb();

    QString m_error;
    QDir m_appDir;
};
