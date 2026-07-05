#ifndef ENEMYBULLET_H
#define ENEMYBULLET_H

#include "GameObject.h"
#include <QPainter>
#include <QColor>
#include <QPen>
#include <QtMath>

class EnemyBullet : public GameObject
{
private:
    QPointF m_direction;

public:
    EnemyBullet(const QPointF& center, const QPointF& direction, qreal bulletSpeed = 220.0)
    {
        qreal length = qSqrt(direction.x() * direction.x() + direction.y() * direction.y());

        if (length <= 0.001) {
            m_direction = QPointF(0.0, 1.0);
        } else {
            m_direction = QPointF(direction.x() / length, direction.y() / length);
        }

        speed = bulletSpeed;

        hitbox = QRectF(0, 0, 12.0, 12.0);
        position = QPointF(center.x() - hitbox.width() / 2.0,
                           center.y() - hitbox.height() / 2.0);

        hitbox.moveTo(position);
    }

    void update(qreal deltaTime)
    {
        position.setX(position.x() + m_direction.x() * speed * deltaTime);
        position.setY(position.y() + m_direction.y() * speed * deltaTime);
        hitbox.moveTo(position);
    }

    bool isOutOfBounds(int screenWidth, int screenHeight) const
    {
        return position.x() < -40 ||
               position.x() > screenWidth + 40 ||
               position.y() < -40 ||
               position.y() > screenHeight + 40;
    }

    void draw(QPainter& painter) const
    {
        painter.setRenderHint(QPainter::Antialiasing);

        painter.setPen(QPen(QColor(120, 20, 30), 2));
        painter.setBrush(QColor(255, 90, 110));
        painter.drawEllipse(hitbox);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 180, 190));
        painter.drawEllipse(hitbox.adjusted(3, 3, -5, -5));
    }
};

#endif // ENEMYBULLET_H
