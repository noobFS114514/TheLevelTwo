#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QKeyEvent>
#include <QPainter>
#include <QRandomGenerator>
#include <QDebug>

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

    resetGame();
}

void MainWindow::resetGame() {
    m_score = 0;
    m_playerHp = 3;
    m_currentWave = 1;
    m_isGameOver = false;
    m_moveLeft = false;
    m_moveRight = false;
    m_shootCooldown = 0.25;
    m_timeSinceLastShot = m_player.getFireCooldown();

    m_enemies.clear();
    m_bullets.clear();
    m_powerUps.clear();
    m_effects.clear();
    m_hurtFlashTimer = 0.0;


    m_player.spawnPlayer(730.0, 0.0, static_cast<qreal>(m_screenWidth));

    m_elapsedTimer.start();
    m_lastFrameTime = 0.0;

    m_gameTimer->start(16);   // 60FPS
    m_waveTimer->start(2500);  // 稍慢一些，更接近可玩的演示节奏
}

void MainWindow::spawnWave() {
    if (m_isGameOver) return;
    const int enemyWidth = 118;
    int enemyCount = QRandomGenerator::global()->bounded(3, 6); // 随机3~5个 [cite: 5]
    for (int i = 0; i < enemyCount; ++i) {
        qreal randomX = QRandomGenerator::global()->bounded(35, m_screenWidth - enemyWidth - 35);
        qreal startY = -70.0 - (i * 70.0);
        qreal enemySpeed = 80.0 + (m_currentWave * 3.0);
        m_enemies.append(Enemy(QPointF(randomX, startY), enemySpeed, m_currentWave));
    }
    m_currentWave++;
}

void MainWindow::gameLoop() {
    qreal currentTime = m_elapsedTimer.elapsed() / 1000.0;
    qreal deltaTime = currentTime - m_lastFrameTime;
    m_lastFrameTime = currentTime;

    if (m_isGameOver) { update(); return; }

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

    // 2. 驱动敌人物理（完全解耦，直接调用归位接口）
    for (auto it = m_enemies.begin(); it != m_enemies.end();) {
        it->moveDown(deltaTime);
        if (it->getPosition().y() > m_screenHeight) it = m_enemies.erase(it); // 越界清理
        else ++it;
    }

    for (auto it = m_powerUps.begin(); it != m_powerUps.end();) {
    it->moveDown(deltaTime);

        if (it->getPosition().y() > m_screenHeight) {
            it = m_powerUps.erase(it);
        } else {
            ++it;
        }
    }


    // 3. 碰撞仲裁
    checkCollisions();
    update();
}

void MainWindow::checkCollisions() {
    if (m_isGameOver) return;

    // 子弹 VS 敌人 [cite: 6]
    for (auto enemyIt = m_enemies.begin(); enemyIt != m_enemies.end(); ) {
        bool enemyDestroyed = false;
        for (auto bulletIt = m_bullets.begin(); bulletIt != m_bullets.end(); ) {
            if (enemyIt->getHitbox().intersects(bulletIt->getHitbox())) {
                enemyIt->getHurt(1);
                bulletIt = m_bullets.erase(bulletIt);
                if (enemyIt->isDead()) {
                    m_score += enemyIt->getScoreValue();
                    m_effects.append(
                        ParticleEffect::explosion(enemyIt->getHitbox().center(), QColor(255, 218, 92), 18)
                    );

                    QPointF dropPos = enemyIt->getPosition();

                    int chance = QRandomGenerator::global()->bounded(100);
                    if (chance < 30) {
                        m_powerUps.append(PowerUp(dropPos, PowerUpType::Heal));
                    }

                    enemyIt = m_enemies.erase(enemyIt);
                    enemyDestroyed = true;
                    break;
                }                

            } else { ++bulletIt; }
        }
        if (!enemyDestroyed) ++enemyIt;
    }

    // 玩家 VS 敌人 [cite: 6]
    QRectF playerBox = m_player.getHitbox();
    for (auto enemyIt = m_enemies.begin(); enemyIt != m_enemies.end(); ) {
        if (enemyIt->getHitbox().intersects(playerBox)) {
    m_hurtFlashTimer = 0.18;

    m_effects.append(
        ParticleEffect::explosion(enemyIt->getHitbox().center(), QColor(255, 80, 80), 14)
    );

    enemyIt = m_enemies.erase(enemyIt);
    m_playerHp--;

    if (m_playerHp <= 0) {
        m_isGameOver = true;
        m_waveTimer->stop();
    }
}
else { ++enemyIt; }
    }

    QRectF playerHitbox = m_player.getHitbox();

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

            qDebug() << "PowerUp picked: HP +1";
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

    for (auto& powerUp : m_powerUps) {
        powerUp.draw(painter);
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
        painter.drawText(rect(), Qt::AlignCenter, "GAME OVER\n\nPress R to Restart");
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
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

    // 临时测试武器升级，后续由宝箱触发
if (event->key() == Qt::Key_U && !m_isGameOver) {
    if (m_player.increaseWeaponLevel()) {
        qDebug() << "Weapon upgraded:"
                 << "level =" << m_player.getWeaponLevel()
                 << "bullet count =" << m_player.getWeapon().getBulletCount();
    } else {
        qDebug() << "Weapon already at max level";
    }

    return;
}

    if (event->key() == Qt::Key_Space && !m_isGameOver) {
        shootBullet();
        return;
    }

    QMainWindow::keyPressEvent(event);
}

MainWindow::~MainWindow()
{
    delete ui;
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
    painter.drawText(28, 39, QString("Score %1").arg(m_score));
    painter.drawText(178, 39, QString("Weapon Lv: %1").arg(m_player.getWeaponLevel()));

    QString hpText = "HP ";
    for (int i = 0; i < m_playerHp; ++i) {
        hpText += "+";
    }
    if (m_playerHp <= 0) {
        hpText += "DEAD";
    }
    painter.drawText(m_screenWidth - 118, 39, hpText);
}