#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QAction>
#include <QCheckBox>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMenuBar>
#include <QPainter>
#include <QRandomGenerator>
#include <QSlider>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setFixedSize(m_screenWidth, m_screenHeight);
    setFocusPolicy(Qt::StrongFocus);

    m_gameTimer = new QTimer(this);
    m_waveTimer = new QTimer(this);

    connect(m_gameTimer, &QTimer::timeout, this, &MainWindow::gameLoop);
    connect(m_waveTimer, &QTimer::timeout, this, &MainWindow::spawnWave);

    QAction* settingsAction = menuBar()->addAction("Settings");
    connect(settingsAction, &QAction::triggered, this, &MainWindow::showSettingsDialog);

    m_saveManager.load();
    resetGame();
    applySavedSettings();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::resetGame()
{
    m_score = 0;
    m_playerHp = 3;
    m_currentWave = 1;
    m_isGameOver = false;
    m_highScoreSaved = false;
    m_moveLeft = false;
    m_moveRight = false;
    m_shootCooldown = 0.25;
    m_timeSinceLastShot = m_player.getFireCooldown();

    m_enemies.clear();
    m_bullets.clear();
    m_powerUps.clear();
    m_effects.clear();
    m_chests.clear();
    m_hurtFlashTimer = 0.0;

    m_player = Player();
    m_player.spawnPlayer(730.0, 0.0, static_cast<qreal>(m_screenWidth));

    m_elapsedTimer.start();
    m_lastFrameTime = 0.0;

    m_gameTimer->start(16);
    m_waveTimer->start(2500);
}

void MainWindow::spawnWave()
{
    if (m_isGameOver) return;

    const int enemyWidth = 118;
    const int enemyCount = QRandomGenerator::global()->bounded(3, 6);

    for (int i = 0; i < enemyCount; ++i) {
        const qreal randomX = QRandomGenerator::global()->bounded(35, m_screenWidth - enemyWidth - 35);
        const qreal startY = -70.0 - (i * 70.0);
        const qreal enemySpeed = 80.0 + (m_currentWave * 3.0);
        m_enemies.append(Enemy(QPointF(randomX, startY), enemySpeed, m_currentWave));
    }

    if (m_currentWave >= 2 && QRandomGenerator::global()->bounded(100) < 35) {
        Chest chest;
        const qreal chestX = QRandomGenerator::global()->bounded(40, m_screenWidth - 90);
        chest.spawnChest(chestX, -80.0, m_currentWave);
        m_chests.append(chest);

        qDebug() << "Chest spawned at wave" << m_currentWave;
    }

    ++m_currentWave;
}

void MainWindow::gameLoop()
{
    const qreal currentTime = m_elapsedTimer.elapsed() / 1000.0;
    const qreal deltaTime = currentTime - m_lastFrameTime;
    m_lastFrameTime = currentTime;

    if (m_isGameOver) {
        update();
        return;
    }

    m_timeSinceLastShot += deltaTime;

    if (m_hurtFlashTimer > 0.0) {
        m_hurtFlashTimer -= deltaTime;
        if (m_hurtFlashTimer < 0.0) {
            m_hurtFlashTimer = 0.0;
        }
    }

    for (auto it = m_effects.begin(); it != m_effects.end();) {
        it->update(deltaTime);
        if (it->isFinished()) {
            it = m_effects.erase(it);
        } else {
            ++it;
        }
    }

    if (m_moveLeft) {
        m_player.moveLeft(deltaTime);
    }
    if (m_moveRight) {
        m_player.moveRight(deltaTime);
    }

    for (auto it = m_bullets.begin(); it != m_bullets.end();) {
        it->updateMovement(deltaTime);
        if (it->getPosition().y() < -20) {
            it = m_bullets.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = m_enemies.begin(); it != m_enemies.end();) {
        it->moveDown(deltaTime);
        if (it->getPosition().y() > m_screenHeight) {
            it = m_enemies.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = m_powerUps.begin(); it != m_powerUps.end();) {
        it->moveDown(deltaTime);
        if (it->getPosition().y() > m_screenHeight) {
            it = m_powerUps.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = m_chests.begin(); it != m_chests.end();) {
        it->moveDown(deltaTime);
        if (it->getPosition().y() > m_screenHeight) {
            it = m_chests.erase(it);
        } else {
            ++it;
        }
    }

    checkCollisions();
    update();
}

void MainWindow::checkCollisions()
{
    if (m_isGameOver) return;

    for (auto chestIt = m_chests.begin(); chestIt != m_chests.end();) {
        bool chestRemoved = false;

        for (auto bulletIt = m_bullets.begin(); bulletIt != m_bullets.end();) {
            if (!chestIt->getHitbox().intersects(bulletIt->getHitbox())) {
                ++bulletIt;
                continue;
            }

            chestIt->takeDamage(bulletIt->getDamage());

            if (bulletIt->consumePierce()) {
                bulletIt = m_bullets.erase(bulletIt);
            } else {
                ++bulletIt;
            }

            if (chestIt->isDead()) {
                m_effects.append(
                    ParticleEffect::explosion(chestIt->getHitbox().center(), QColor(255, 215, 70), 24)
                );

                m_gameTimer->stop();
                m_waveTimer->stop();

                const bool upgraded = chestIt->showUpgradeWindow(
                    &m_player.getWeapon(),
                    chestIt->getWave(),
                    this
                );

                if (upgraded) {
                    m_player.recordWeaponUpgrade();
                }

                m_lastFrameTime = m_elapsedTimer.elapsed() / 1000.0;

                if (!m_isGameOver) {
                    m_gameTimer->start(16);
                    m_waveTimer->start(2500);
                }

                chestIt = m_chests.erase(chestIt);
                chestRemoved = true;
                break;
            }
        }

        if (!chestRemoved) {
            ++chestIt;
        }
    }

    for (auto enemyIt = m_enemies.begin(); enemyIt != m_enemies.end();) {
        bool enemyDestroyed = false;

        for (auto bulletIt = m_bullets.begin(); bulletIt != m_bullets.end();) {
            if (!enemyIt->getHitbox().intersects(bulletIt->getHitbox())) {
                ++bulletIt;
                continue;
            }

            enemyIt->getHurt(bulletIt->getDamage());

            if (bulletIt->consumePierce()) {
                bulletIt = m_bullets.erase(bulletIt);
            } else {
                ++bulletIt;
            }

            if (enemyIt->isDead()) {
                m_score += enemyIt->getScoreValue();
                m_effects.append(
                    ParticleEffect::explosion(enemyIt->getHitbox().center(), QColor(255, 218, 92), 18)
                );

                const QPointF dropPos = enemyIt->getPosition();
                const int chance = QRandomGenerator::global()->bounded(100);

                if (chance < 22) {
                    m_powerUps.append(PowerUp(dropPos, PowerUpType::Heal));
                } else if (chance < 34) {
                    m_powerUps.append(PowerUp(dropPos, PowerUpType::WeaponUpgrade));
                }

                enemyIt = m_enemies.erase(enemyIt);
                enemyDestroyed = true;
                break;
            }
        }

        if (!enemyDestroyed) {
            ++enemyIt;
        }
    }

    const QRectF playerBox = m_player.getHitbox();
    for (auto enemyIt = m_enemies.begin(); enemyIt != m_enemies.end();) {
        if (enemyIt->getHitbox().intersects(playerBox)) {
            m_hurtFlashTimer = 0.18;
            m_effects.append(
                ParticleEffect::explosion(enemyIt->getHitbox().center(), QColor(255, 80, 80), 14)
            );

            enemyIt = m_enemies.erase(enemyIt);
            --m_playerHp;

            if (m_playerHp <= 0) {
                finishGame();
            }
        } else {
            ++enemyIt;
        }
    }

    const QRectF playerHitbox = m_player.getHitbox();
    for (auto powerIt = m_powerUps.begin(); powerIt != m_powerUps.end();) {
        if (!powerIt->getHitbox().intersects(playerHitbox)) {
            ++powerIt;
            continue;
        }

        if (powerIt->getType() == PowerUpType::Heal) {
            m_effects.append(
                ParticleEffect::ring(powerIt->getHitbox().center(), QColor(80, 220, 120), 20)
            );
            m_playerHp += 1;
            if (m_playerHp > 5) {
                m_playerHp = 5;
            }
        } else if (powerIt->getType() == PowerUpType::WeaponUpgrade) {
            m_effects.append(
                ParticleEffect::explosion(powerIt->getHitbox().center(), QColor(255, 215, 70), 24)
            );
            m_player.increaseWeaponLevel();
        }

        powerIt = m_powerUps.erase(powerIt);
    }
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    drawBackground(painter);

    for (auto& enemy : m_enemies) {
        enemy.draw(painter);
    }

    for (auto& powerUp : m_powerUps) {
        powerUp.draw(painter);
    }

    for (auto& chest : m_chests) {
        chest.draw(painter);
    }

    for (const auto& b : m_bullets) {
        const QColor bulletColor = b.getCrit() ? QColor(255, 96, 76) : QColor(255, 218, 92);
        painter.setPen(QPen(QColor(86, 61, 22), 2));
        painter.setBrush(bulletColor);
        painter.drawRoundedRect(b.getHitbox(), 3.0, 3.0);
        painter.setBrush(QColor(255, 245, 164));
        painter.drawRoundedRect(b.getHitbox().adjusted(2, 2, -3, -8), 2.0, 2.0);
    }

    for (const auto& effect : m_effects) {
        effect.draw(painter);
    }

    drawPlayer(painter);
    drawHud(painter);

    if (m_hurtFlashTimer > 0.0) {
        qreal ratio = m_hurtFlashTimer / 0.18;
        if (ratio > 1.0) ratio = 1.0;
        if (ratio < 0.0) ratio = 0.0;

        painter.fillRect(rect(), QColor(255, 0, 0, static_cast<int>(90 * ratio)));
    }

    if (m_isGameOver) {
        painter.fillRect(rect(), QColor(0, 0, 0, 160));
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 22, QFont::Bold));
        painter.drawText(rect(), Qt::AlignCenter,
                         QString("GAME OVER\nScore %1   Best %2\n\nPress R to Restart")
                         .arg(m_score)
                         .arg(m_saveManager.highScore()));
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) {
        QMainWindow::keyPressEvent(event);
        return;
    }

    if (m_isGameOver && event->key() == Qt::Key_R) {
        resetGame();
        return;
    }

    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_A) {
        m_moveLeft = true;
        return;
    }

    if (event->key() == Qt::Key_Right || event->key() == Qt::Key_D) {
        m_moveRight = true;
        return;
    }

    if (event->key() == Qt::Key_S && !m_isGameOver) {
        showSettingsDialog();
        return;
    }

    if (event->key() == Qt::Key_Space && !m_isGameOver) {
        shootBullet();
        return;
    }

    QMainWindow::keyPressEvent(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) {
        QMainWindow::keyReleaseEvent(event);
        return;
    }

    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_A) {
        m_moveLeft = false;
        return;
    }

    if (event->key() == Qt::Key_Right || event->key() == Qt::Key_D) {
        m_moveRight = false;
        return;
    }

    QMainWindow::keyReleaseEvent(event);
}

void MainWindow::shootBullet()
{
    const qreal cooldown = m_player.getFireCooldown();

    if (m_timeSinceLastShot < cooldown) {
        return;
    }

    const QRectF playerBox = m_player.getHitbox();
    const QPointF origin(playerBox.center().x() - 4.0, playerBox.top() - 15.0);

    m_player.getWeapon().fire(origin, m_bullets);
    m_timeSinceLastShot = 0.0;
}

void MainWindow::finishGame()
{
    if (m_highScoreSaved) {
        m_isGameOver = true;
        return;
    }

    m_isGameOver = true;
    m_waveTimer->stop();
    m_saveManager.updateHighScore(m_score);
    m_saveManager.save();
    m_highScoreSaved = true;
}

void MainWindow::showSettingsDialog()
{
    const bool gameWasRunning = !m_isGameOver && m_gameTimer->isActive();
    if (gameWasRunning) {
        m_gameTimer->stop();
        m_waveTimer->stop();
    }

    SaveManager::Settings settings = m_saveManager.settings();

    QDialog dialog(this);
    dialog.setWindowTitle("Settings");

    auto* layout = new QVBoxLayout(&dialog);
    auto* scoreLabel = new QLabel(QString("Best score: %1").arg(m_saveManager.highScore()), &dialog);
    scoreLabel->setAlignment(Qt::AlignCenter);

    auto* form = new QFormLayout();

    auto* soundSlider = new QSlider(Qt::Horizontal, &dialog);
    soundSlider->setRange(0, 100);
    soundSlider->setValue(settings.soundVolume);

    auto* musicSlider = new QSlider(Qt::Horizontal, &dialog);
    musicSlider->setRange(0, 100);
    musicSlider->setValue(settings.musicVolume);

    auto* fullscreenCheck = new QCheckBox("Fullscreen", &dialog);
    fullscreenCheck->setChecked(settings.fullscreen);

    form->addRow("Sound", soundSlider);
    form->addRow("Music", musicSlider);
    form->addRow("", fullscreenCheck);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    layout->addWidget(scoreLabel);
    layout->addLayout(form);
    layout->addWidget(buttons);

    if (dialog.exec() == QDialog::Accepted) {
        settings.soundVolume = soundSlider->value();
        settings.musicVolume = musicSlider->value();
        settings.fullscreen = fullscreenCheck->isChecked();
        m_saveManager.setSettings(settings);
        m_saveManager.save();
        applySavedSettings();
    }

    m_lastFrameTime = m_elapsedTimer.elapsed() / 1000.0;
    if (gameWasRunning && !m_isGameOver) {
        m_gameTimer->start(16);
        m_waveTimer->start(2500);
    }
}

void MainWindow::applySavedSettings()
{
    const SaveManager::Settings settings = m_saveManager.settings();

    if (settings.fullscreen) {
        setMinimumSize(0, 0);
        setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        showFullScreen();
    } else {
        showNormal();
        setFixedSize(m_screenWidth, m_screenHeight);
    }
}

void MainWindow::drawBackground(QPainter &painter)
{
    QLinearGradient road(0, 0, 0, m_screenHeight);
    road.setColorAt(0.0, QColor(72, 59, 62));
    road.setColorAt(0.55, QColor(88, 72, 74));
    road.setColorAt(1.0, QColor(101, 83, 78));
    painter.fillRect(rect(), road);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(38, 39, 44));
    painter.drawRect(0, 0, 34, m_screenHeight);
    painter.drawRect(m_screenWidth - 34, 0, 34, m_screenHeight);

    painter.setBrush(QColor(50, 52, 58));
    painter.drawRect(34, 0, 8, m_screenHeight);
    painter.drawRect(m_screenWidth - 42, 0, 8, m_screenHeight);

    painter.setPen(QPen(QColor(46, 45, 48, 95), 2));
    painter.drawLine(m_screenWidth / 2, 0, m_screenWidth / 2, m_screenHeight);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(70, 68, 60, 120));
    for (int i = 0; i < 10; ++i) {
        const int x = (i * 83 + 48) % (m_screenWidth - 80) + 24;
        const int y = (i * 137 + 70) % (m_screenHeight - 120) + 60;
        painter.drawEllipse(QPointF(x, y), 8 + (i % 3), 3 + (i % 2));
    }

    painter.setBrush(QColor(111, 96, 80));
    painter.drawRect(0, m_screenHeight - 92, m_screenWidth, 18);
    painter.setBrush(QColor(84, 68, 58));
    painter.drawRect(0, m_screenHeight - 74, m_screenWidth, 74);
}

void MainWindow::drawPlayer(QPainter &painter)
{
    QRectF box = m_player.getHitbox();
    QRectF body = box.adjusted(0, 16, 0, -6);
    QRectF cabin = box.adjusted(26, 0, -26, -32);

    painter.setPen(QPen(QColor(48, 48, 58), 3));
    painter.setBrush(QColor(154, 154, 170));
    painter.drawRoundedRect(body, 12.0, 12.0);

    painter.setBrush(QColor(197, 197, 212));
    painter.drawRoundedRect(cabin, 8.0, 8.0);

    painter.setBrush(QColor(44, 48, 58));
    painter.drawRoundedRect(box.adjusted(28, 36, -64, -20), 4.0, 4.0);
    painter.drawRoundedRect(box.adjusted(64, 36, -28, -20), 4.0, 4.0);

    painter.setBrush(QColor(40, 48, 58));
    painter.drawEllipse(QPointF(box.left() + 22, box.bottom() - 8), 14, 14);
    painter.drawEllipse(QPointF(box.right() - 22, box.bottom() - 8), 14, 14);
    painter.setBrush(QColor(76, 197, 220));
    painter.drawEllipse(QPointF(box.left() + 22, box.bottom() - 8), 6, 6);
    painter.drawEllipse(QPointF(box.right() - 22, box.bottom() - 8), 6, 6);

    QRectF cannon(box.center().x() - 8, box.top() - 12, 16, 30);
    painter.setBrush(QColor(90, 96, 112));
    painter.drawRoundedRect(cannon, 5.0, 5.0);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 159, 50));
    QPointF flame[] = {
        QPointF(box.center().x(), box.top() - 22),
        QPointF(box.center().x() - 6, box.top() - 8),
        QPointF(box.center().x() + 6, box.top() - 8)
    };
    painter.drawPolygon(flame, 3);
}

void MainWindow::drawHud(QPainter &painter)
{
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(32, 35, 42, 165));
    painter.drawRoundedRect(QRectF(12, 12, m_screenWidth - 24, 58), 10.0, 10.0);

    painter.setPen(QColor(255, 255, 255));
    painter.setFont(QFont("Arial", 13, QFont::Black));
    painter.drawText(28, 38, QString("Score %1").arg(m_score));
    painter.drawText(142, 38, QString("Best %1").arg(m_saveManager.highScore()));
    painter.drawText(244, 38, QString("Lv %1").arg(m_player.getWeaponLevel()));

    QString hpText = "HP ";
    for (int i = 0; i < m_playerHp; ++i) {
        hpText += "+";
    }
    if (m_playerHp <= 0) {
        hpText += "DEAD";
    }
    painter.drawText(m_screenWidth - 84, 38, hpText);

    painter.setFont(QFont("Arial", 11, QFont::Bold));
    painter.drawText(28, 62, QString("Wave %1").arg(m_currentWave));
}
