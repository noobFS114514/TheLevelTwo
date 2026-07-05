#pragma once

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

class SaveManager {
public:
    struct Settings {
        int soundVolume = 70;
        int musicVolume = 60;
        bool fullscreen = false;
    };

    SaveManager() {
        const QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        m_filePath = dirPath.isEmpty()
            ? QStringLiteral("save.json")
            : dirPath + QStringLiteral("/save.json");
    }

    bool load() {
        QFile file(m_filePath);
        if (!file.exists()) {
            return save();
        }

        if (!file.open(QIODevice::ReadOnly)) {
            return false;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (!doc.isObject()) {
            return false;
        }

        const QJsonObject root = doc.object();
        m_highScore = root.value(QStringLiteral("highScore")).toInt(0);

        const QJsonObject settingsObj = root.value(QStringLiteral("settings")).toObject();
        m_settings.soundVolume = clampVolume(settingsObj.value(QStringLiteral("soundVolume")).toInt(70));
        m_settings.musicVolume = clampVolume(settingsObj.value(QStringLiteral("musicVolume")).toInt(60));
        m_settings.fullscreen = settingsObj.value(QStringLiteral("fullscreen")).toBool(false);

        return true;
    }

    bool save() const {
        const QFileInfo info(m_filePath);
        if (!QDir().mkpath(info.absolutePath())) {
            return false;
        }

        QFile file(m_filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            return false;
        }

        QJsonObject settingsObj;
        settingsObj.insert(QStringLiteral("soundVolume"), m_settings.soundVolume);
        settingsObj.insert(QStringLiteral("musicVolume"), m_settings.musicVolume);
        settingsObj.insert(QStringLiteral("fullscreen"), m_settings.fullscreen);

        QJsonObject root;
        root.insert(QStringLiteral("highScore"), m_highScore);
        root.insert(QStringLiteral("settings"), settingsObj);

        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        return true;
    }

    int highScore() const {
        return m_highScore;
    }

    bool updateHighScore(int score) {
        if (score <= m_highScore) {
            return false;
        }

        m_highScore = score;
        return true;
    }

    Settings settings() const {
        return m_settings;
    }

    void setSettings(const Settings& settings) {
        m_settings.soundVolume = clampVolume(settings.soundVolume);
        m_settings.musicVolume = clampVolume(settings.musicVolume);
        m_settings.fullscreen = settings.fullscreen;
    }

    QString filePath() const {
        return m_filePath;
    }

private:
    static int clampVolume(int value) {
        if (value < 0) return 0;
        if (value > 100) return 100;
        return value;
    }

    QString m_filePath;
    int m_highScore = 0;
    Settings m_settings;
};
