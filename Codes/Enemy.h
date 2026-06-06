#pragma once
#include "GameObject.h"
#include <QPainter>
#include <QColor>

class Enemy : public GameObject {
protected:
    int health;
    int m_scoreValue;

public:
    // 构造函数（直接在头文件内内联实现）
    Enemy(const QPointF& startPos, qreal enemySpeed, int wave) {
        position = startPos;
        speed = enemySpeed;
        hitbox = QRectF(position.x(), position.y(), 118.0, 56.0);
        healthInit(wave);
        m_scoreValue = 100 * wave; // 击杀得分
    }

    void healthInit(const int flags) {
        health = 1 + (flags / 3); // 基础血量算法
    }

    void attack() {}

    void getHurt(int dmg) {
        health -= dmg;
    }

    // 重写下落移动：向下移的同时必须挪动碰撞箱
    void moveDown(qreal deltaTime) override {
        GameObject::moveDown(deltaTime);
        hitbox.moveTo(position); // 碰撞箱位置紧跟实体坐标
    }

    // 绘图功能完全自治：敌人自己负责画自己
    void draw(QPainter &painter) {
        if (isDead()) return;

        painter.setRenderHint(QPainter::Antialiasing);

        QRectF body = hitbox;
        QRectF plank = body.adjusted(16, 7, -16, -9);
        QRectF leftBand(body.left() + 4, body.top() + 4, 18, body.height() - 8);
        QRectF rightBand(body.right() - 22, body.top() + 4, 18, body.height() - 8);

        painter.setPen(QPen(QColor(82, 54, 38), 2));
        painter.setBrush(QColor(139, 82, 44));
        painter.drawRoundedRect(body, 8.0, 8.0);

        painter.setBrush(QColor(212, 157, 91));
        painter.drawRoundedRect(plank, 6.0, 6.0);

        painter.setBrush(QColor(189, 132, 70));
        painter.drawRoundedRect(leftBand, 5.0, 5.0);
        painter.drawRoundedRect(rightBand, 5.0, 5.0);

        painter.setPen(QPen(QColor(55, 40, 32), 3));
        painter.setFont(QFont("Arial", 18, QFont::Black));
        painter.drawText(body, Qt::AlignCenter, QString::number(m_scoreValue / 100));
    }

    bool isDead() const { return health <= 0; }
    int getScoreValue() const { return m_scoreValue; }

    ~Enemy() noexcept override = default;
};