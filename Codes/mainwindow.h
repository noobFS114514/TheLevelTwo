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
    // A1 板块需要的绘图事件和键盘事件
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void gameLoop();    // 核心游戏循环驱动（约 60 FPS）
    void spawnWave();   // A3核心：波次生成器

private:
    Ui::MainWindow *ui;

    // 游戏核心循环驱动
    QTimer* m_gameTimer;
    QTimer* m_waveTimer;
    QElapsedTimer m_elapsedTimer; // 用于精准计算 deltaTime
    qreal m_lastFrameTime;

    // 游戏实体管理容器
    Player m_player;
    QList<Enemy> m_enemies;
    QList<Bullet> m_bullets; // 用于接收来自 Player 发射的子弹
    QList<PowerUp> m_powerUps;

    // A3 计分与状态变量
    int m_score;
    int m_playerHp;
    int m_currentWave; // 当前波次
    bool m_isGameOver;
    bool m_moveLeft;
    bool m_moveRight;
    qreal m_shootCooldown;
    qreal m_timeSinceLastShot;

    // 竖版屏幕尺寸常数
    const int m_screenWidth = 450;
    const int m_screenHeight = 800;

    void resetGame();       // 游戏初始化与重置
    void checkCollisions(); // A3核心：轴对齐矩形碰撞检测
    void shootBullet();     // A2核心：单发子弹生成
    void drawBackground(QPainter &painter);
    void drawPlayer(QPainter &painter);
    void drawHud(QPainter &painter);
};

#endif // MAINWINDOW_H
