#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QList>
#include <QElapsedTimer>

#include "Player.h"
#include "Enemy.h"
#include "Bullet.h"
#include "PowerUp.h"
#include "ParticleEffect.h"
#include "Chest.h"
#include "SaveManager.h"
#include "BossEnemy.h"
#include "EnemyBullet.h"

class QPainter;
class QPaintEvent;
class QKeyEvent;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void gameLoop();
    void spawnWave();

private:
    enum class GameState {
        Menu,
        Playing,
        Paused,
        GameOver
    };

    Ui::MainWindow *ui;

    QTimer* m_gameTimer;
    QTimer* m_waveTimer;
    QElapsedTimer m_elapsedTimer;
    qreal m_lastFrameTime;

    Player m_player;
    QList<Enemy> m_enemies;
    QList<BossEnemy> m_bosses;
    QList<Bullet> m_bullets;
    QList<EnemyBullet> m_enemyBullets;
    QList<PowerUp> m_powerUps;
    QList<Chest> m_chests;
    QList<ParticleEffect> m_effects;

    int m_score;
    int m_highScore;
    int m_playerHp;
    int m_currentWave;
    bool m_isGameOver;
    bool m_bossActive;
    GameState m_gameState;
    bool m_moveLeft;
    bool m_moveRight;
    qreal m_shootCooldown;
    qreal m_timeSinceLastShot;
    qreal m_bossShootTimer;
    qreal m_hurtFlashTimer;

    const int m_screenWidth = 450;
    const int m_screenHeight = 800;

    void resetGame();
    void updateHighScore();
    void startBossBattle();
    void spawnBossBullets();
    void startNewGame();
    void pauseGame();
    void resumeGame();
    void returnToMenu();

    void checkCollisions();
    void shootBullet();

    void drawBackground(QPainter &painter);
    void drawPlayer(QPainter &painter);
    void drawHud(QPainter &painter);
    void drawMenu(QPainter &painter);
    void drawPauseOverlay(QPainter &painter);
    void drawGameOverOverlay(QPainter &painter);
};

#endif // MAINWINDOW_H
