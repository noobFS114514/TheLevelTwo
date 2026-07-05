#pragma once

#include "GameObject.h"
#include <QPainter>
#include <QColor>
#include <QPen>
#include <QFont>

enum class PowerUpType {
    Heal,
    WeaponUpgrade
};

class PowerUp : public GameObject {
private:
    PowerUpType m_type;

public:
    PowerUp(const QPointF& startPos, PowerUpType type = PowerUpType::Heal) {
        m_type = type;
        position = startPos;
        speed = 120.0;
        hitbox = QRectF(position.x(), position.y(), 28.0, 28.0);
    }

    PowerUpType getType() const {
        return m_type;
    }

    void moveDown(qreal deltaTime) override {
        GameObject::moveDown(deltaTime);
        hitbox.moveTo(position);
    }

    void draw(QPainter& painter) {
        painter.setRenderHint(QPainter::Antialiasing);

        if (m_type == PowerUpType::Heal) {
            painter.setPen(QPen(QColor(255, 255, 255), 2));
            painter.setBrush(QColor(80, 220, 120));
            painter.drawEllipse(hitbox);

            painter.setPen(QPen(QColor(255, 255, 255), 3));

            QPointF center = hitbox.center();

            painter.drawLine(
                QPointF(center.x() - 7, center.y()),
                QPointF(center.x() + 7, center.y())
            );

            painter.drawLine(
                QPointF(center.x(), center.y() - 7),
                QPointF(center.x(), center.y() + 7)
            );
        } else if (m_type == PowerUpType::WeaponUpgrade) {
            painter.setPen(QPen(QColor(120, 80, 20), 2));
            painter.setBrush(QColor(255, 215, 70));
            painter.drawRoundedRect(hitbox, 6.0, 6.0);

            painter.setPen(QColor(120, 80, 20));
            painter.setFont(QFont("Arial", 13, QFont::Bold));
            painter.drawText(hitbox, Qt::AlignCenter, "W");
        }
    }
};
