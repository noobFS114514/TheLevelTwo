#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QSoundEffect>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

class AudioManager : public QObject
{
public:
    explicit AudioManager(QObject* parent = nullptr)
        : QObject(parent)
    {
        m_musicPlayer = new QMediaPlayer(this);
        m_musicOutput = new QAudioOutput(this);
        m_musicPlayer->setAudioOutput(m_musicOutput);

        m_shootEffect = new QSoundEffect(this);
        m_hitEffect = new QSoundEffect(this);
        m_pickupEffect = new QSoundEffect(this);
        m_bossEffect = new QSoundEffect(this);
        m_chestEffect = new QSoundEffect(this);

        connect(m_musicPlayer, &QMediaPlayer::mediaStatusChanged,
                this,
                [this](QMediaPlayer::MediaStatus status) {
                    if (status == QMediaPlayer::EndOfMedia && m_hasMusic) {
                        m_musicPlayer->play();
                    }
                });
    }

    void load(const QString& audioDir)
    {
        QDir dir(audioDir);

        loadMusic(dir.filePath("bgm.wav"));

        loadEffect(m_shootEffect, dir.filePath("shoot.wav"));
        loadEffect(m_hitEffect, dir.filePath("hit.wav"));
        loadEffect(m_pickupEffect, dir.filePath("pickup.wav"));
        loadEffect(m_bossEffect, dir.filePath("boss.wav"));
        loadEffect(m_chestEffect, dir.filePath("chest.wav"));
    }

    void setVolumes(int sfxVolume, int musicVolume)
    {
        m_sfxVolume = qBound(0, sfxVolume, 100);
        m_musicVolume = qBound(0, musicVolume, 100);

        qreal sfx = static_cast<qreal>(m_sfxVolume) / 100.0;
        qreal music = static_cast<qreal>(m_musicVolume) / 100.0;

        m_musicOutput->setVolume(music);

        m_shootEffect->setVolume(sfx);
        m_hitEffect->setVolume(sfx);
        m_pickupEffect->setVolume(sfx);
        m_bossEffect->setVolume(sfx);
        m_chestEffect->setVolume(sfx);
    }

    void playMusic()
    {
        if (!m_hasMusic) {
            return;
        }

        if (m_musicPlayer->playbackState() != QMediaPlayer::PlayingState) {
            m_musicPlayer->play();
        }
    }

    void stopMusic()
    {
        m_musicPlayer->stop();
    }

    void playShoot()
    {
        playEffect(m_shootEffect);
    }

    void playHit()
    {
        playEffect(m_hitEffect);
    }

    void playPickup()
    {
        playEffect(m_pickupEffect);
    }

    void playBoss()
    {
        playEffect(m_bossEffect);
    }

    void playChest()
    {
        playEffect(m_chestEffect);
    }

private:
    QMediaPlayer* m_musicPlayer = nullptr;
    QAudioOutput* m_musicOutput = nullptr;

    QSoundEffect* m_shootEffect = nullptr;
    QSoundEffect* m_hitEffect = nullptr;
    QSoundEffect* m_pickupEffect = nullptr;
    QSoundEffect* m_bossEffect = nullptr;
    QSoundEffect* m_chestEffect = nullptr;

    bool m_hasMusic = false;
    int m_sfxVolume = 70;
    int m_musicVolume = 60;

    void loadMusic(const QString& filePath)
    {
        if (!QFileInfo::exists(filePath)) {
            qDebug() << "Missing music file:" << filePath;
            m_hasMusic = false;
            return;
        }

        m_musicPlayer->setSource(QUrl::fromLocalFile(filePath));
        m_hasMusic = true;
    }

    void loadEffect(QSoundEffect* effect, const QString& filePath)
    {
        if (!effect) {
            return;
        }

        if (!QFileInfo::exists(filePath)) {
            qDebug() << "Missing sound file:" << filePath;
            return;
        }

        effect->setSource(QUrl::fromLocalFile(filePath));
    }

    void playEffect(QSoundEffect* effect)
    {
        if (!effect || m_sfxVolume <= 0 || effect->source().isEmpty()) {
            return;
        }

        effect->stop();
        effect->play();
    }
};

#endif // AUDIOMANAGER_H
