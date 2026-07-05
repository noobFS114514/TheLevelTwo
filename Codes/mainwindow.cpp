#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QKeyEvent>
#include <QPainter>
#include <QRandomGenerator>
#include <QDebug>
#include <QApplication>
#include <QtMath>
#include <QMouseEvent>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setFixedSize(m_screenWidth, m_screenHeight); // 固定竖版
    setFocusPolicy(Qt::StrongFocus);

    m_gameTimer = new QTimer(this);
    m_waveTimer = new QTimer(this);

    connect(m_gameTimer, &QTimer::timeout, this, &MainWindow::gameLoop);
    connect(m_waveTimer, &QTimer::timeout, this, &MainWindow::spawnWave);

    m_highScore = SaveManager::loadHighScore();
    m_sfxVolume = SaveManager::loadSfxVolume();
    m_musicVolume = SaveManager::loadMusicVolume();
    m_fullscreenEnabled = SaveManager::loadFullscreen();
    m_selectedSetting = 0;

    resetGame();
    applyWindowMode();

    m_gameState = GameState::Menu;
    m_gameTimer->stop();
    m_waveTimer->stop();
    update();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::resetGame() {
    m_score = 0;
    m_playerHp = 3;
    m_currentWave = 1;
    m_currentLevel = 1;
    m_wavesClearedInLevel = 0;
    m_waitingForNextWave = false;
    m_nextWaveDelayTimer = 0.0;

    m_isGameOver = false;
    m_bossActive = false;

    m_moveLeft = false;
    m_moveRight = false;
    m_shootCooldown = 0.25;
    m_timeSinceLastShot = m_player.getFireCooldown();
    m_bossShootTimer = 0.0;

    m_enemies.clear();
    m_bosses.clear();
    m_bullets.clear();
    m_enemyBullets.clear();
    m_powerUps.clear();
    m_effects.clear();
    m_floatingTexts.clear();
    m_chests.clear();
    m_hurtFlashTimer = 0.0;

    m_player = Player();
    m_player.spawnPlayer(730.0, 0.0, static_cast<qreal>(m_screenWidth));

    m_elapsedTimer.start();
    m_lastFrameTime = 0.0;

    m_gameTimer->stop();
    m_waveTimer->stop();
}

void MainWindow::updateHighScore()
{
    if (m_score > m_highScore) {
        m_highScore = m_score;
        SaveManager::saveHighScore(m_highScore);

        qDebug() << "New high score saved:" << m_highScore;
    }
}

void MainWindow::saveSettings()
{
    SaveManager::saveSettings(m_sfxVolume, m_musicVolume, m_fullscreenEnabled);
}

void MainWindow::applyWindowMode()
{
    if (m_fullscreenEnabled) {
        showFullScreen();
    } else {
        showNormal();
        setFixedSize(m_screenWidth, m_screenHeight);
    }

    update();
}

void MainWindow::playSfx()
{
    if (m_sfxVolume <= 0) {
        return;
    }

    QApplication::beep();
}

void MainWindow::addFloatingText(const QPointF& position,
                                 const QString& text,
                                 const QColor& color,
                                 int fontSize)
{
    m_floatingTexts.append(FloatingText(position, text, color, fontSize));
}


void MainWindow::startBossBattle()
{
    m_bossActive = true;
    m_bossShootTimer = 0.0;

    m_waveTimer->stop();

    m_enemies.clear();
    m_chests.clear();

    m_bosses.append(BossEnemy(m_currentWave + m_currentLevel * 2, m_screenWidth));

    m_effects.append(
        ParticleEffect::ring(QPointF(m_screenWidth / 2.0, 120.0),
                             QColor(255, 80, 90),
                             36)
    );

    addFloatingText(QPointF(m_screenWidth / 2.0, 180.0),
                "WARNING!",
                QColor(255, 80, 90),
                26);

playSfx();

    qDebug() << "Boss battle started at wave" << m_currentWave;
}

void MainWindow::spawnBossBullets()
{
    if (m_bosses.isEmpty()) {
        return;
    }

    QRectF bossBox = m_bosses.first().getHitbox();
    QPointF origin(bossBox.center().x(), bossBox.bottom() - 4.0);

    const int bulletCount = 7;
    const qreal startAngle = 55.0;
    const qreal endAngle = 125.0;
    const qreal step = (endAngle - startAngle) / (bulletCount - 1);

    for (int i = 0; i < bulletCount; ++i) {
        qreal angleDegree = startAngle + step * i;
        qreal angleRad = qDegreesToRadians(angleDegree);

        QPointF direction(qCos(angleRad), qSin(angleRad));
        m_enemyBullets.append(EnemyBullet(origin, direction, 230.0));
    }

    m_effects.append(
        ParticleEffect::ring(origin, QColor(255, 90, 110), 18)
    );

    qDebug() << "Boss fired" << bulletCount << "bullets";
}


void MainWindow::startNewGame()
{
    resetGame();
    m_gameState = GameState::Playing;
    m_isGameOver = false;

    m_lastFrameTime = m_elapsedTimer.elapsed() / 1000.0;

    m_gameTimer->start(16);
    m_waveTimer->stop();

    spawnWave();

    setFocus();
    update();
}

void MainWindow::pauseGame()
{
    if (m_gameState != GameState::Playing) {
        return;
    }

    m_gameState = GameState::Paused;
    m_gameTimer->stop();
    m_waveTimer->stop();

    m_moveLeft = false;
    m_moveRight = false;

    update();
}

void MainWindow::resumeGame()
{
    if (m_gameState != GameState::Paused) {
        return;
    }

    m_gameState = GameState::Playing;

    m_lastFrameTime = m_elapsedTimer.elapsed() / 1000.0;

    m_gameTimer->start(16);
    m_waveTimer->stop();

    setFocus();
    update();
}

void MainWindow::returnToMenu()
{
    resetGame();
    m_gameState = GameState::Menu;
    m_isGameOver = false;

    m_moveLeft = false;
    m_moveRight = false;

    m_gameTimer->stop();
    m_waveTimer->stop();

    update();
}

void MainWindow::spawnWave()
{
    if (m_gameState != GameState::Playing) {
        return;
    }

    if (m_bossActive) {
        return;
    }

    if (!m_enemies.isEmpty()) {
        return;
    }

    if (m_wavesClearedInLevel >= 3) {
        startBossBattle();
        return;
    }

    const int enemyWidth = 118;

    int enemyCount = QRandomGenerator::global()->bounded(
        3 + m_currentLevel,
        6 + m_currentLevel
    );

    if (enemyCount > 9) {
        enemyCount = 9;
    }

    qreal baseSpeed = 80.0 + m_currentLevel * 15.0 + m_currentWave * 2.0;

    for (int i = 0; i < enemyCount; ++i) {
        qreal randomX = QRandomGenerator::global()->bounded(
            35,
            m_screenWidth - enemyWidth - 35
        );

        qreal startY = -70.0 - (i * 70.0);
        qreal enemySpeed = baseSpeed + QRandomGenerator::global()->bounded(0, 30);

        m_enemies.append(
            Enemy(QPointF(randomX, startY), enemySpeed, m_currentWave)
        );
    }

    if (m_currentWave >= 2 &&
        QRandomGenerator::global()->bounded(100) < 35) {
        Chest chest;

        qreal chestX = QRandomGenerator::global()->bounded(
            40,
            m_screenWidth - 90
        );

        chest.spawnChest(chestX, -80.0, m_currentWave);
        m_chests.append(chest);

        qDebug() << "Chest spawned at wave" << m_currentWave;
    }

    addFloatingText(QPointF(m_screenWidth / 2.0, 165.0),
                    QString("LEVEL %1 - WAVE %2")
                        .arg(m_currentLevel)
                        .arg(m_wavesClearedInLevel + 1),
                    QColor(255, 218, 92),
                    20);

    qDebug() << "Wave spawned:"
             << "level =" << m_currentLevel
             << "wave =" << m_currentWave
             << "count =" << enemyCount;

    m_currentWave++;
}

void MainWindow::gameLoop() {
    qreal currentTime = m_elapsedTimer.elapsed() / 1000.0;
    qreal deltaTime = currentTime - m_lastFrameTime;
    m_lastFrameTime = currentTime;

    if (m_gameState != GameState::Playing) {
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

for (auto it = m_floatingTexts.begin(); it != m_floatingTexts.end();) {
    it->update(deltaTime);

    if (it->isFinished()) {
        it = m_floatingTexts.erase(it);
    } else {
        ++it;
    }
}


    // A2：根据按键状态持续移动玩家
    if (m_moveLeft) {
        m_player.moveLeft(deltaTime);
    }
    if (m_moveRight) {
        m_player.moveRight(deltaTime);
    }

    // 1. 驱动子弹物理
    for (auto it = m_bullets.begin(); it != m_bullets.end();) {
        it->updateMovement(deltaTime);
        if (it->getPosition().y() < -20) it = m_bullets.erase(it);
        else ++it;
    }

    for (auto it = m_enemyBullets.begin(); it != m_enemyBullets.end();) {
    it->update(deltaTime);

    if (it->isOutOfBounds(m_screenWidth, m_screenHeight)) {
        it = m_enemyBullets.erase(it);
    } else {
        ++it;
    }
}


    // 2. 驱动敌人物理（完全解耦，直接调用归位接口）
    for (auto it = m_enemies.begin(); it != m_enemies.end();) {
        it->moveDown(deltaTime);
        if (it->getPosition().y() > m_screenHeight) it = m_enemies.erase(it); // 越界清理
        else ++it;
    }

    for (auto it = m_bosses.begin(); it != m_bosses.end();) {
    it->update(deltaTime, m_screenWidth);

if (it->getPosition().y() > m_screenHeight) {
    it = m_bosses.erase(it);
    m_bossActive = false;

    m_currentLevel++;
    m_wavesClearedInLevel = 0;
    m_currentWave++;

    m_waitingForNextWave = true;
    m_nextWaveDelayTimer = 1.5;
} else {
    ++it;
}

}

if (m_bossActive && !m_bosses.isEmpty()) {
    m_bossShootTimer += deltaTime;

    if (m_bossShootTimer >= 1.4) {
        m_bossShootTimer = 0.0;
        spawnBossBullets();
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


    // 3. 碰撞仲裁
    // 3. 碰撞仲裁
checkCollisions();

if (m_gameState == GameState::Playing &&
    !m_bossActive &&
    m_bosses.isEmpty() &&
    m_enemies.isEmpty() &&
    m_chests.isEmpty() &&
    !m_waitingForNextWave) {

    m_wavesClearedInLevel++;
    m_waitingForNextWave = true;
    m_nextWaveDelayTimer = 1.4;

    qDebug() << "Wave cleared. Cleared in level ="
             << m_wavesClearedInLevel;
}

if (m_waitingForNextWave) {
    m_nextWaveDelayTimer -= deltaTime;

    if (m_nextWaveDelayTimer <= 0.0) {
        m_waitingForNextWave = false;
        m_nextWaveDelayTimer = 0.0;
        spawnWave();
    }
}

update();

}

void MainWindow::checkCollisions() {
    if (m_gameState != GameState::Playing) return;

    // 子弹 VS 宝箱
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
            qDebug() << "Chest opened at wave" << chestIt->getWave();

            m_gameTimer->stop();
            m_waveTimer->stop();

            bool upgraded = chestIt->showUpgradeWindow(
                &m_player.getWeapon(),
                chestIt->getWave(),
                this
            );

            if (upgraded) {
                m_player.recordWeaponUpgrade();

                qDebug() << "Chest upgrade accepted:"
                         << "level =" << m_player.getWeaponLevel()
                         << "damage =" << m_player.getWeapon().getDamage()
                         << "bullet count =" << m_player.getWeapon().getBulletCount()
                         << "cooldown =" << m_player.getWeapon().getAttackCooldown()
                         << "pierce =" << m_player.getWeapon().getPierceCount();
            }

            m_lastFrameTime = m_elapsedTimer.elapsed() / 1000.0;

            if (m_gameState == GameState::Playing) {
                m_gameTimer->start(16);
                m_waveTimer->stop();
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
    
    // 子弹 VS 敌人：伤害、暴击、穿透全部生效
for (auto enemyIt = m_enemies.begin(); enemyIt != m_enemies.end(); ) {
    bool enemyDestroyed = false;

    for (auto bulletIt = m_bullets.begin(); bulletIt != m_bullets.end(); ) {
        if (!enemyIt->getHitbox().intersects(bulletIt->getHitbox())) {
            ++bulletIt;
            continue;
        }

        QPointF hitCenter = enemyIt->getHitbox().center();
        QPointF dropPos = enemyIt->getPosition();

        enemyIt->getHurt(bulletIt->getDamage());

        if (bulletIt->getCrit()) {
            m_effects.append(
                ParticleEffect::explosion(hitCenter, QColor(255, 80, 80), 10)
            );
        }

        bool shouldRemoveBullet = bulletIt->consumePierce();

        if (enemyIt->isDead()) {
            int gainedScore = enemyIt->getScoreValue();
            m_score += gainedScore;

            addFloatingText(hitCenter,
                            QString("+%1").arg(gainedScore),
                            QColor(255, 218, 92),
                            15);

            playSfx();

            m_effects.append(
                ParticleEffect::explosion(hitCenter, QColor(255, 218, 92), 18)
            );

            int chance = QRandomGenerator::global()->bounded(100);
            if (chance < 30) {
                m_powerUps.append(PowerUp(dropPos, PowerUpType::Heal));
            }

            enemyIt = m_enemies.erase(enemyIt);
            enemyDestroyed = true;
        }

        if (shouldRemoveBullet) {
            bulletIt = m_bullets.erase(bulletIt);
        } else {
            ++bulletIt;
        }

        if (enemyDestroyed) {
            break;
        }
    }

    if (!enemyDestroyed) {
        ++enemyIt;
    }
}

// 子弹 VS Boss
for (auto bossIt = m_bosses.begin(); bossIt != m_bosses.end();) {
    bool bossDestroyed = false;

    for (auto bulletIt = m_bullets.begin(); bulletIt != m_bullets.end();) {
        if (!bossIt->getHitbox().intersects(bulletIt->getHitbox())) {
            ++bulletIt;
            continue;
        }

        QPointF hitCenter = bulletIt->getHitbox().center();

        bossIt->takeDamage(bulletIt->getDamage());

        if (bulletIt->getCrit()) {
            m_effects.append(
                ParticleEffect::explosion(hitCenter, QColor(255, 80, 80), 12)
            );
        }

        bool shouldRemoveBullet = bulletIt->consumePierce();

        if (bossIt->isDead()) {
            QPointF dropCenter = bossIt->getHitbox().center();

            int gainedScore = bossIt->getScoreValue();
            m_score += gainedScore;

            addFloatingText(dropCenter,
                            QString("BOSS CLEAR  +%1").arg(gainedScore),
                            QColor(255, 218, 92),
                            20);

            playSfx();

            m_effects.append(
                ParticleEffect::explosion(dropCenter, QColor(255, 80, 90), 48)
            );

            for (int i = 0; i < 5; ++i) {
                QPointF dropPos(
                    dropCenter.x() - 70.0 + i * 35.0,
                    dropCenter.y()
                );

                PowerUpType type = (i % 2 == 0)
                    ? PowerUpType::Heal
                    : PowerUpType::WeaponUpgrade;

                m_powerUps.append(PowerUp(dropPos, type));
            }

            bossIt = m_bosses.erase(bossIt);
            bossDestroyed = true;
            m_bossActive = false;

            m_currentLevel++;
            m_wavesClearedInLevel = 0;

            m_waitingForNextWave = true;
            m_nextWaveDelayTimer = 2.0;

            addFloatingText(QPointF(m_screenWidth / 2.0, 230.0),
                            QString("LEVEL %1").arg(m_currentLevel),
                            QColor(255, 218, 92),
                            24);

            qDebug() << "Boss defeated. Score =" << m_score
                    << "Next level =" << m_currentLevel;

        }

        if (shouldRemoveBullet) {
            bulletIt = m_bullets.erase(bulletIt);
        } else {
            ++bulletIt;
        }

        if (bossDestroyed) {
            break;
        }
    }

    if (!bossDestroyed) {
        ++bossIt;
    }
}

    // 玩家 VS 敌人 [cite: 6]
    QRectF playerBox = m_player.getHitbox();
    QRectF playerHitbox = playerBox;

    for (auto enemyIt = m_enemies.begin(); enemyIt != m_enemies.end(); ) {
        if (enemyIt->getHitbox().intersects(playerBox)) {
    m_hurtFlashTimer = 0.18;

    m_effects.append(
        ParticleEffect::explosion(enemyIt->getHitbox().center(), QColor(255, 80, 80), 14)
    );

    enemyIt = m_enemies.erase(enemyIt);
    m_playerHp--;

    addFloatingText(m_player.getHitbox().center(),
                "-1 HP",
                QColor(255, 80, 90),
                16);

playSfx();

   if (m_playerHp <= 0) {
    m_isGameOver = true;
    m_gameState = GameState::GameOver;

    updateHighScore();

    m_gameTimer->stop();
    m_waveTimer->stop();

    m_moveLeft = false;
    m_moveRight = false;
   }

}
else { ++enemyIt; }
    }

    // 玩家 VS Boss
for (auto bossIt = m_bosses.begin(); bossIt != m_bosses.end(); ++bossIt) {
    if (bossIt->getHitbox().intersects(playerBox) && m_hurtFlashTimer <= 0.0) {
        m_hurtFlashTimer = 0.35;

        m_effects.append(
            ParticleEffect::explosion(m_player.getHitbox().center(),
                                      QColor(255, 80, 80),
                                      18)
        );

        m_playerHp--;

        addFloatingText(playerHitbox.center(),
                "-1 HP",
                QColor(255, 80, 90),
                16);

playSfx();


        if (m_playerHp <= 0) {
            m_isGameOver = true;
            m_gameState = GameState::GameOver;

            updateHighScore();

            m_gameTimer->stop();
            m_waveTimer->stop();

            m_moveLeft = false;
            m_moveRight = false;
        }
    }
}

  for (auto bulletIt = m_enemyBullets.begin(); bulletIt != m_enemyBullets.end();) {
    if (bulletIt->getHitbox().intersects(playerHitbox)) {
        bulletIt = m_enemyBullets.erase(bulletIt);

        if (m_hurtFlashTimer <= 0.0) {
            m_hurtFlashTimer = 0.25;

            m_effects.append(
                ParticleEffect::explosion(playerHitbox.center(),
                                          QColor(255, 80, 90),
                                          16)
            );

            m_playerHp--;

            addFloatingText(playerHitbox.center(),
                "-1 HP",
                QColor(255, 80, 90),
                16);

playSfx();


            qDebug() << "Player hit by boss bullet. HP =" << m_playerHp;

            if (m_playerHp <= 0) {
                m_isGameOver = true;
                m_gameState = GameState::GameOver;

                updateHighScore();

                m_gameTimer->stop();
                m_waveTimer->stop();

                m_moveLeft = false;
                m_moveRight = false;

                break;
            }
        }
    } else {
        ++bulletIt;
    }
}

for (auto powerIt = m_powerUps.begin(); powerIt != m_powerUps.end();) {
    if (powerIt->getHitbox().intersects(playerHitbox)) {
        m_effects.append(
            ParticleEffect::ring(powerIt->getHitbox().center(), QColor(80, 220, 120), 20)
        );

        if (powerIt->getType() == PowerUpType::Heal) {
    m_playerHp += 1;

    if (m_playerHp > 5) {
        m_playerHp = 5;
    }

    addFloatingText(playerHitbox.center(),
                "+1 HP",
                QColor(80, 220, 120),
                15);

playSfx();


    qDebug() << "PowerUp picked: HP +1";
} else if (powerIt->getType() == PowerUpType::WeaponUpgrade) {
    if (m_player.increaseWeaponLevel()) {
        addFloatingText(playerHitbox.center(),
                QString("Weapon Lv %1").arg(m_player.getWeaponLevel()),
                QColor(255, 218, 92),
                15);

playSfx();

        qDebug() << "PowerUp picked: weapon level ="
                 << m_player.getWeaponLevel();
    } else {
        qDebug() << "PowerUp picked: weapon already max";
    }
}


        powerIt = m_powerUps.erase(powerIt);
    } else {
        ++powerIt;
    }
}

}

void MainWindow::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    drawBackground(painter);

    for (auto& enemy : m_enemies) {
        enemy.draw(painter);
    }

    for (auto& boss : m_bosses) {
    boss.draw(painter);
}

    for (auto& powerUp : m_powerUps) {
        powerUp.draw(painter);
    }

    for (auto& chest : m_chests) {
    chest.draw(painter);
}

    painter.setPen(QPen(QColor(86, 61, 22), 2));
    painter.setBrush(QColor(255, 218, 92));
    for (const auto& b : m_bullets) {
        painter.drawRoundedRect(b.getHitbox(), 3.0, 3.0);
        painter.setBrush(QColor(255, 245, 164));
        QRectF shine = b.getHitbox().adjusted(2, 2, -3, -8);
        painter.drawRoundedRect(shine, 2.0, 2.0);
        painter.setBrush(QColor(255, 218, 92));
    }

    for (const auto& enemyBullet : m_enemyBullets) {
    enemyBullet.draw(painter);
}

    for (const auto& effect : m_effects) {
    effect.draw(painter);
}

for (const auto& text : m_floatingTexts) {
    text.draw(painter);
}

    drawPlayer(painter);
    drawHud(painter);

    if (m_hurtFlashTimer > 0.0) {
    qreal ratio = m_hurtFlashTimer / 0.18;
    if (ratio > 1.0) ratio = 1.0;
    if (ratio < 0.0) ratio = 0.0;

    painter.fillRect(rect(), QColor(255, 0, 0, static_cast<int>(90 * ratio)));
}


    if (m_gameState == GameState::Menu) {
    drawMenu(painter);
} else if (m_gameState == GameState::Settings) {
    drawSettingsMenu(painter);
} else if (m_gameState == GameState::Paused) {
    drawPauseOverlay(painter);
} else if (m_gameState == GameState::GameOver) {
    drawGameOverOverlay(painter);
}


}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) {
        QMainWindow::keyPressEvent(event);
        return;
    }

    // 主菜单
if (m_gameState == GameState::Menu) {
    if (event->key() == Qt::Key_Return ||
        event->key() == Qt::Key_Enter ||
        event->key() == Qt::Key_Space) {
        startNewGame();
        return;
    }

    if (event->key() == Qt::Key_S) {
        m_gameState = GameState::Settings;
        m_selectedSetting = 0;
        update();
        return;
    }

    if (event->key() == Qt::Key_Q) {
        close();
        return;
    }

    return;
}

if (m_gameState == GameState::Settings) {
    if (event->key() == Qt::Key_Escape ||
        event->key() == Qt::Key_M) {
        m_gameState = GameState::Menu;
        update();
        return;
    }

    if (event->key() == Qt::Key_Up || event->key() == Qt::Key_W) {
        m_selectedSetting--;

        if (m_selectedSetting < 0) {
            m_selectedSetting = 3;
        }

        update();
        return;
    }

    if (event->key() == Qt::Key_Down || event->key() == Qt::Key_S) {
        m_selectedSetting++;

        if (m_selectedSetting > 3) {
            m_selectedSetting = 0;
        }

        update();
        return;
    }

    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_A) {
        if (m_selectedSetting == 0) {
            m_sfxVolume -= 10;
            if (m_sfxVolume < 0) {
                m_sfxVolume = 0;
            }
            saveSettings();
        } else if (m_selectedSetting == 1) {
            m_musicVolume -= 10;
            if (m_musicVolume < 0) {
                m_musicVolume = 0;
            }
            saveSettings();
        }

        update();
        return;
    }

    if (event->key() == Qt::Key_Right || event->key() == Qt::Key_D) {
        if (m_selectedSetting == 0) {
            m_sfxVolume += 10;
            if (m_sfxVolume > 100) {
                m_sfxVolume = 100;
            }
            saveSettings();
        } else if (m_selectedSetting == 1) {
            m_musicVolume += 10;
            if (m_musicVolume > 100) {
                m_musicVolume = 100;
            }
            saveSettings();
        }

        update();
        return;
    }

    if (event->key() == Qt::Key_F ||
        event->key() == Qt::Key_Return ||
        event->key() == Qt::Key_Enter ||
        event->key() == Qt::Key_Space) {
        if (m_selectedSetting == 2) {
            m_fullscreenEnabled = !m_fullscreenEnabled;
            saveSettings();
            applyWindowMode();
            return;
        }

        if (m_selectedSetting == 3) {
            m_gameState = GameState::Menu;
            update();
            return;
        }
    }

    return;
}

    // 游戏中按 ESC 暂停
    if (m_gameState == GameState::Playing &&
        event->key() == Qt::Key_Escape) {
        pauseGame();
        return;
    }

    // 暂停菜单
    if (m_gameState == GameState::Paused) {
        if (event->key() == Qt::Key_Escape ||
            event->key() == Qt::Key_Return ||
            event->key() == Qt::Key_Enter) {
            resumeGame();
            return;
        }

        if (event->key() == Qt::Key_R) {
            startNewGame();
            return;
        }

        if (event->key() == Qt::Key_M) {
            returnToMenu();
            return;
        }

        if (event->key() == Qt::Key_Q) {
            close();
            return;
        }

        return;
    }

    // Game Over 菜单
    if (m_gameState == GameState::GameOver) {
        if (event->key() == Qt::Key_R ||
            event->key() == Qt::Key_Return ||
            event->key() == Qt::Key_Enter) {
            startNewGame();
            return;
        }

        if (event->key() == Qt::Key_M) {
            returnToMenu();
            return;
        }

        if (event->key() == Qt::Key_Q) {
            close();
            return;
        }

        return;
    }

    // 下面只处理 Playing 状态
    if (m_gameState != GameState::Playing) {
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

    // 临时测试武器升级，后续可以删掉
    if (event->key() == Qt::Key_U) {
        if (m_player.increaseWeaponLevel()) {
            qDebug() << "Weapon upgraded:"
                     << "level =" << m_player.getWeaponLevel()
                     << "bullet count =" << m_player.getWeapon().getBulletCount();
        } else {
            qDebug() << "Weapon already at max level";
        }

        return;
    }

    if (event->key() == Qt::Key_Space) {
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

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    QPointF pos = event->position();

    if (m_gameState == GameState::Menu) {
        QRectF startRect(70, 240, m_screenWidth - 140, 40);
        QRectF settingsRect(70, 295, m_screenWidth - 140, 40);
        QRectF quitRect(70, 350, m_screenWidth - 140, 40);

        if (startRect.contains(pos)) {
            startNewGame();
            return;
        }

        if (settingsRect.contains(pos)) {
            m_gameState = GameState::Settings;
            m_selectedSetting = 0;
            update();
            return;
        }

        if (quitRect.contains(pos)) {
            close();
            return;
        }
    }

    if (m_gameState == GameState::Settings) {
        QRectF sfxRect(70, 230, m_screenWidth - 140, 40);
        QRectF musicRect(70, 285, m_screenWidth - 140, 40);
        QRectF fullscreenRect(70, 340, m_screenWidth - 140, 40);
        QRectF backRect(70, 420, m_screenWidth - 140, 40);

        if (sfxRect.contains(pos)) {
            m_selectedSetting = 0;
            update();
            return;
        }

        if (musicRect.contains(pos)) {
            m_selectedSetting = 1;
            update();
            return;
        }

        if (fullscreenRect.contains(pos)) {
            m_selectedSetting = 2;
            m_fullscreenEnabled = !m_fullscreenEnabled;
            saveSettings();
            applyWindowMode();
            return;
        }

        if (backRect.contains(pos)) {
            m_gameState = GameState::Menu;
            update();
            return;
        }
    }

    if (m_gameState == GameState::Paused) {
        QRectF resumeRect(70, 310, m_screenWidth - 140, 40);
        QRectF restartRect(70, 360, m_screenWidth - 140, 40);
        QRectF menuRect(70, 410, m_screenWidth - 140, 40);

        if (resumeRect.contains(pos)) {
            resumeGame();
            return;
        }

        if (restartRect.contains(pos)) {
            startNewGame();
            return;
        }

        if (menuRect.contains(pos)) {
            returnToMenu();
            return;
        }
    }

    if (m_gameState == GameState::GameOver) {
        QRectF restartRect(70, 390, m_screenWidth - 140, 40);
        QRectF menuRect(70, 440, m_screenWidth - 140, 40);

        if (restartRect.contains(pos)) {
            startNewGame();
            return;
        }

        if (menuRect.contains(pos)) {
            returnToMenu();
            return;
        }
    }

    QMainWindow::mousePressEvent(event);
}


void MainWindow::shootBullet()
{

    if (m_gameState != GameState::Playing) {
        return;
    }

    const qreal cooldown = m_player.getFireCooldown();

    if (m_timeSinceLastShot < cooldown) {
        return;
    }

    const QRectF playerBox = m_player.getHitbox();

    const QPointF origin(
        playerBox.center().x() - 4.0,
        playerBox.top() - 15.0
    );

    m_player.getWeapon().fire(origin, m_bullets);

    m_timeSinceLastShot = 0.0;

    qDebug() << "Space pressed:"
             << m_player.getWeapon().getBulletCount()
             << "bullet(s) fired";
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
        int x = (i * 83 + 48) % (m_screenWidth - 80) + 24;
        int y = (i * 137 + 70) % (m_screenHeight - 120) + 60;
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
    painter.drawRoundedRect(QRectF(12, 12, m_screenWidth - 24, 42), 10.0, 10.0);

    painter.setPen(QColor(255, 255, 255));
    painter.setFont(QFont("Arial", 14, QFont::Black));
    painter.drawText(24, 39, QString("Score %1").arg(m_score));
    painter.drawText(142, 39, QString("Lv %1").arg(m_currentLevel));
    painter.drawText(205, 39, QString("Weapon %1").arg(m_player.getWeaponLevel()));

    QString hpText = "HP ";
    for (int i = 0; i < m_playerHp; ++i) {
        hpText += "+";
    }
    if (m_playerHp <= 0) {
        hpText += "DEAD";
    }
    painter.drawText(m_screenWidth - 95, 39, hpText);

}


void MainWindow::drawMenu(QPainter &painter)
{
    painter.fillRect(rect(), QColor(0, 0, 0, 175));

    painter.setPen(QColor(255, 218, 92));
    painter.setFont(QFont("Arial", 30, QFont::Black));
    painter.drawText(QRectF(0, 110, m_screenWidth, 60),
                     Qt::AlignCenter,
                     "The Level Two");

    painter.setPen(QPen(QColor(255, 218, 92, 120), 2));
    painter.setBrush(QColor(30, 30, 35, 160));
    painter.drawRoundedRect(QRectF(70, 240, m_screenWidth - 140, 40), 10, 10);
    painter.drawRoundedRect(QRectF(70, 295, m_screenWidth - 140, 40), 10, 10);
    painter.drawRoundedRect(QRectF(70, 350, m_screenWidth - 140, 40), 10, 10);

    painter.setPen(QColor(230, 230, 230));
    painter.setFont(QFont("Microsoft YaHei", 16, QFont::Bold));

    painter.drawText(QRectF(0, 240, m_screenWidth, 40),
                     Qt::AlignCenter,
                     "按 Enter / Space 开始游戏");

    painter.drawText(QRectF(0, 295, m_screenWidth, 40),
                     Qt::AlignCenter,
                     "按 S 设置");

    painter.drawText(QRectF(0, 350, m_screenWidth, 40),
                     Qt::AlignCenter,
                     "按 Q 退出");

    painter.setPen(QColor(255, 218, 92));
    painter.setFont(QFont("Microsoft YaHei", 14, QFont::Bold));
    painter.drawText(QRectF(0, 410, m_screenWidth, 40),
                     Qt::AlignCenter,
                     QString("最高分：%1").arg(m_highScore));

    painter.setPen(QColor(160, 160, 160));
    painter.setFont(QFont("Microsoft YaHei", 11));
    painter.drawText(QRectF(0, 470, m_screenWidth, 80),
                     Qt::AlignCenter,
                     "操作：A/D 或方向键移动，Space 射击，ESC 暂停");
}

void MainWindow::drawSettingsMenu(QPainter &painter)
{
    painter.fillRect(rect(), QColor(0, 0, 0, 185));

    painter.setPen(QColor(255, 218, 92));
    painter.setFont(QFont("Arial", 28, QFont::Black));
    painter.drawText(QRectF(0, 100, m_screenWidth, 60),
                     Qt::AlignCenter,
                     "SETTINGS");

    painter.setFont(QFont("Microsoft YaHei", 15, QFont::Bold));

    painter.setPen(QPen(QColor(255, 218, 92, 110), 2));
    painter.setBrush(QColor(30, 30, 35, 160));
    painter.drawRoundedRect(QRectF(70, 230, m_screenWidth - 140, 40), 10, 10);
    painter.drawRoundedRect(QRectF(70, 285, m_screenWidth - 140, 40), 10, 10);
    painter.drawRoundedRect(QRectF(70, 340, m_screenWidth - 140, 40), 10, 10);
    painter.drawRoundedRect(QRectF(70, 420, m_screenWidth - 140, 40), 10, 10);

    QString sfxPrefix = (m_selectedSetting == 0) ? "> " : "  ";
    QString musicPrefix = (m_selectedSetting == 1) ? "> " : "  ";
    QString fullscreenPrefix = (m_selectedSetting == 2) ? "> " : "  ";
    QString backPrefix = (m_selectedSetting == 3) ? "> " : "  ";

    painter.setPen(m_selectedSetting == 0 ? QColor(255, 218, 92) : QColor(230, 230, 230));
    painter.drawText(QRectF(0, 230, m_screenWidth, 40),
                     Qt::AlignCenter,
                     QString("%1音效音量：%2").arg(sfxPrefix).arg(m_sfxVolume));

    painter.setPen(m_selectedSetting == 1 ? QColor(255, 218, 92) : QColor(230, 230, 230));
    painter.drawText(QRectF(0, 285, m_screenWidth, 40),
                     Qt::AlignCenter,
                     QString("%1音乐音量：%2").arg(musicPrefix).arg(m_musicVolume));

    painter.setPen(m_selectedSetting == 2 ? QColor(255, 218, 92) : QColor(230, 230, 230));
    painter.drawText(QRectF(0, 340, m_screenWidth, 40),
                     Qt::AlignCenter,
                     QString("%1全屏：%2")
                         .arg(fullscreenPrefix)
                         .arg(m_fullscreenEnabled ? "开" : "关"));

    painter.setPen(m_selectedSetting == 3 ? QColor(255, 218, 92) : QColor(230, 230, 230));
    painter.drawText(QRectF(0, 420, m_screenWidth, 40),
                     Qt::AlignCenter,
                     QString("%1返回主菜单").arg(backPrefix));

    painter.setPen(QColor(160, 160, 160));
    painter.setFont(QFont("Microsoft YaHei", 11));
    painter.drawText(QRectF(0, 520, m_screenWidth, 100),
                     Qt::AlignCenter,
                     "↑/↓ 选择，←/→ 调整音量，Enter/F 切换全屏，ESC 返回");
}


void MainWindow::drawPauseOverlay(QPainter &painter)
{
    painter.fillRect(rect(), QColor(0, 0, 0, 150));

    painter.setPen(QColor(255, 255, 255));
    painter.setFont(QFont("Arial", 28, QFont::Black));
    painter.drawText(QRectF(0, 210, m_screenWidth, 60),
                     Qt::AlignCenter,
                     "PAUSED");

    painter.setPen(QPen(QColor(255, 255, 255, 110), 2));
painter.setBrush(QColor(30, 30, 35, 170));
painter.drawRoundedRect(QRectF(70, 310, m_screenWidth - 140, 40), 10, 10);
painter.drawRoundedRect(QRectF(70, 360, m_screenWidth - 140, 40), 10, 10);
painter.drawRoundedRect(QRectF(70, 410, m_screenWidth - 140, 40), 10, 10);

    painter.setPen(QColor(255, 255, 255));
    painter.setFont(QFont("Microsoft YaHei", 15, QFont::Bold));
    painter.drawText(QRectF(0, 310, m_screenWidth, 40),
                     Qt::AlignCenter,
                     "按 ESC / Enter 继续");

    painter.drawText(QRectF(0, 360, m_screenWidth, 40),
                     Qt::AlignCenter,
                     "按 R 重新开始");

    painter.drawText(QRectF(0, 410, m_screenWidth, 40),
                     Qt::AlignCenter,
                     "按 M 返回主菜单");
}

void MainWindow::drawGameOverOverlay(QPainter &painter)
{
    painter.fillRect(rect(), QColor(0, 0, 0, 175));

    painter.setPen(QColor(255, 90, 90));
    painter.setFont(QFont("Arial", 30, QFont::Black));
    painter.drawText(QRectF(0, 170, m_screenWidth, 70),
                     Qt::AlignCenter,
                     "GAME OVER");

    painter.setPen(QColor(255, 255, 255));
    painter.setFont(QFont("Microsoft YaHei", 17, QFont::Bold));
    painter.drawText(QRectF(0, 270, m_screenWidth, 45),
                     Qt::AlignCenter,
                     QString("最终分数：%1").arg(m_score));

    painter.setPen(QColor(255, 218, 92));
painter.drawText(QRectF(0, 315, m_screenWidth, 45),
                 Qt::AlignCenter,
                 QString("最高分：%1").arg(m_highScore));

    painter.setPen(QPen(QColor(255, 255, 255, 110), 2));
painter.setBrush(QColor(30, 30, 35, 170));
painter.drawRoundedRect(QRectF(70, 390, m_screenWidth - 140, 40), 10, 10);
painter.drawRoundedRect(QRectF(70, 440, m_screenWidth - 140, 40), 10, 10);

    painter.setPen(QColor(255, 255, 255));
    painter.setFont(QFont("Microsoft YaHei", 14, QFont::Bold));
    painter.setPen(QColor(255, 255, 255));
painter.drawText(QRectF(0, 390, m_screenWidth, 40),
                 Qt::AlignCenter,
                 "按 R / Enter 再来一局");

painter.drawText(QRectF(0, 440, m_screenWidth, 40),
                 Qt::AlignCenter,
                 "按 M 返回主菜单");

}
