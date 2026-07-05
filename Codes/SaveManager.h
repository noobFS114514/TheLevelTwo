#ifndef SAVEMANAGER_H
#define SAVEMANAGER_H

#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QString>

class SaveManager
{
public:
    static QString saveFilePath()
    {
        QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

        if (dirPath.isEmpty()) {
            dirPath = QDir::currentPath();
        }

        QDir dir(dirPath);
        if (!dir.exists()) {
            dir.mkpath(".");
        }

        return dir.filePath("save.json");
    }

    static int loadHighScore()
    {
        QFile file(saveFilePath());

        if (!file.exists()) {
            return 0;
        }

        if (!file.open(QIODevice::ReadOnly)) {
            return 0;
        }

        QByteArray data = file.readAll();
        file.close();

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);

        if (error.error != QJsonParseError::NoError || !doc.isObject()) {
            return 0;
        }

        QJsonObject obj = doc.object();
        return obj.value("highScore").toInt(0);
    }

    static void saveHighScore(int highScore)
    {
        QJsonObject obj;
        obj["highScore"] = highScore;

        QJsonDocument doc(obj);

        QFile file(saveFilePath());
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            return;
        }

        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }
};

#endif // SAVEMANAGER_H
