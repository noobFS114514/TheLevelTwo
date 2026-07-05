#pragma once

#include <QList>
#include <QPointF>
#include <QColor>
#include <QPainter>
#include <QRandomGenerator>
#include <QtMath>

struct Particle {
    QPointF position;
    QPointF velocity;
    qreal life;
    qreal maxLife;
    qreal radius;
    QColor color;
};

class ParticleEffect {
private:
    QList<Particle> m_particles;

public:
    ParticleEffect() = default;

    static ParticleEffect explosion(const QPointF& center, const QColor& color, int count = 16) {
        ParticleEffect effect;

        for (int i = 0; i < count; ++i) {
            qreal angle = QRandomGenerator::global()->bounded(360) * M_PI / 180.0;
            qreal speed = 70.0 + QRandomGenerator::global()->bounded(130);

            Particle p;
            p.position = center;
            p.velocity = QPointF(qCos(angle) * speed, qSin(angle) * speed);
            p.life = 0.45;
            p.maxLife = 0.45;
            p.radius = 3.0 + QRandomGenerator::global()->bounded(4);
            p.color = color;

            effect.m_particles.append(p);
        }

        return effect;
    }

    static ParticleEffect ring(const QPointF& center, const QColor& color, int count = 20) {
        ParticleEffect effect;

        for (int i = 0; i < count; ++i) {
            qreal angle = (360.0 / count) * i * M_PI / 180.0;
            qreal speed = 95.0;

            Particle p;
            p.position = center;
            p.velocity = QPointF(qCos(angle) * speed, qSin(angle) * speed);
            p.life = 0.35;
            p.maxLife = 0.35;
            p.radius = 2.8;
            p.color = color;

            effect.m_particles.append(p);
        }

        return effect;
    }

    void update(qreal deltaTime) {
        for (auto it = m_particles.begin(); it != m_particles.end();) {
            it->life -= deltaTime;

            it->position += QPointF(
                it->velocity.x() * deltaTime,
                it->velocity.y() * deltaTime
            );

            if (it->life <= 0.0) {
                it = m_particles.erase(it);
            } else {
                ++it;
            }
        }
    }

    void draw(QPainter& painter) const {
        painter.setPen(Qt::NoPen);

        for (const auto& p : m_particles) {
            qreal ratio = p.life / p.maxLife;
            if (ratio < 0.0) ratio = 0.0;
            if (ratio > 1.0) ratio = 1.0;

            QColor c = p.color;
            c.setAlpha(static_cast<int>(220 * ratio));

            painter.setBrush(c);
            painter.drawEllipse(p.position, p.radius * ratio, p.radius * ratio);
        }
    }

    bool isFinished() const {
        return m_particles.isEmpty();
    }
};
