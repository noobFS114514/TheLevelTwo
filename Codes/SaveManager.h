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

    static QJsonObject loadObject()
    {
        QFile file(saveFilePath());

        if (!file.exists()) {
            return QJsonObject();
        }

        if (!file.open(QIODevice::ReadOnly)) {
            return QJsonObject();
        }

        QByteArray data = file.readAll();
        file.close();

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);

        if (error.error != QJsonParseError::NoError || !doc.isObject()) {
            return QJsonObject();
        }

        return doc.object();
    }

    static void saveObject(const QJsonObject& obj)
    {
        QFile file(saveFilePath());

        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            return;
        }

        QJsonDocument doc(obj);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }

    static int loadHighScore()
    {
        QJsonObject obj = loadObject();
        return obj.value("highScore").toInt(0);
    }

    static void saveHighScore(int highScore)
    {
        QJsonObject obj = loadObject();
        obj["highScore"] = highScore;
        saveObject(obj);
    }

    static int loadSfxVolume()
    {
        QJsonObject obj = loadObject();
        return obj.value("sfxVolume").toInt(70);
    }

    static int loadMusicVolume()
    {
        QJsonObject obj = loadObject();
        return obj.value("musicVolume").toInt(60);
    }

    static bool loadFullscreen()
    {
        QJsonObject obj = loadObject();
        return obj.value("fullscreen").toBool(false);
    }

    static void saveSettings(int sfxVolume, int musicVolume, bool fullscreen)
    {
        QJsonObject obj = loadObject();

        obj["sfxVolume"] = sfxVolume;
        obj["musicVolume"] = musicVolume;
        obj["fullscreen"] = fullscreen;

        saveObject(obj);
    }
};

#endif // SAVEMANAGER_H
