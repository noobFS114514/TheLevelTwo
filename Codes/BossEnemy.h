#ifndef BOSSENEMY_H
#define BOSSENEMY_H

#include "GameObject.h"
#include <QPainter>
#include <QColor>
#include <QPen>
#include <QFont>

class BossEnemy : public GameObject
{
private:
    int m_health;
    int m_maxHealth;
    int m_scoreValue;
    int m_direction;

public:
    BossEnemy(int wave, int screenWidth)
    {
        qreal width = 180.0;
        qreal height = 105.0;

        position = QPointF((screenWidth - width) / 2.0, -130.0);
        hitbox = QRectF(position.x(), position.y(), width, height);

        speed = 55.0;
        m_direction = 1;

        m_maxHealth = 10 + wave * 2;
        m_health = m_maxHealth;
        m_scoreValue = 1000 + wave * 200;
    }

    void update(qreal deltaTime, int screenWidth)
    {
        if (position.y() < 80.0) {
            position.setY(position.y() + speed * deltaTime);
        } else {
            position.setX(position.x() + m_direction * 120.0 * deltaTime);

            if (position.x() <= 36.0) {
                position.setX(36.0);
                m_direction = 1;
            }

            if (position.x() + hitbox.width() >= screenWidth - 36.0) {
                position.setX(screenWidth - 36.0 - hitbox.width());
                m_direction = -1;
            }

            position.setY(position.y() + 8.0 * deltaTime);
        }

        hitbox.moveTo(position);
    }

    void takeDamage(int damage)
    {
        m_health -= damage;
    }

    bool isDead() const
    {
        return m_health <= 0;
    }

    int getScoreValue() const
    {
        return m_scoreValue;
    }

    qreal healthRatio() const
    {
        if (m_maxHealth <= 0) {
            return 0.0;
        }

        qreal ratio = static_cast<qreal>(m_health) / static_cast<qreal>(m_maxHealth);

        if (ratio < 0.0) {
            ratio = 0.0;
        }

        if (ratio > 1.0) {
            ratio = 1.0;
        }

        return ratio;
    }

    void draw(QPainter& painter) const
    {
        if (isDead()) {
            return;
        }

        painter.setRenderHint(QPainter::Antialiasing);

        QRectF body = hitbox;
        QRectF eyeLeft(body.left() + 42, body.top() + 32, 26, 18);
        QRectF eyeRight(body.right() - 68, body.top() + 32, 26, 18);

        painter.setPen(QPen(QColor(80, 20, 25), 3));
        painter.setBrush(QColor(150, 45, 55));
        painter.drawRoundedRect(body, 18.0, 18.0);

        painter.setBrush(QColor(205, 70, 70));
        painter.drawRoundedRect(body.adjusted(18, 14, -18, -18), 14.0, 14.0);

        painter.setBrush(QColor(35, 35, 45));
        painter.drawRoundedRect(eyeLeft, 6.0, 6.0);
        painter.drawRoundedRect(eyeRight, 6.0, 6.0);

        painter.setPen(QPen(QColor(60, 20, 20), 5));
        painter.drawLine(QPointF(body.left() + 60, body.bottom() - 25),
                         QPointF(body.right() - 60, body.bottom() - 25));

        QRectF barBg(body.left(), body.top() - 18, body.width(), 10);
        QRectF barHp(barBg.left(), barBg.top(), barBg.width() * healthRatio(), barBg.height());

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(45, 45, 50));
        painter.drawRoundedRect(barBg, 5.0, 5.0);

        painter.setBrush(QColor(255, 80, 90));
        painter.drawRoundedRect(barHp, 5.0, 5.0);

        painter.setPen(QColor(255, 230, 120));
        painter.setFont(QFont("Arial", 16, QFont::Black));
        painter.drawText(body, Qt::AlignCenter, "BOSS");
    }
};

#endif // BOSSENEMY_H
