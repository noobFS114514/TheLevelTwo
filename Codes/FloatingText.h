#ifndef FLOATINGTEXT_H
#define FLOATINGTEXT_H

#include <QPointF>
#include <QString>
#include <QColor>
#include <QPainter>
#include <QFont>

class FloatingText
{
private:
    QPointF m_position;
    QString m_text;
    QColor m_color;
    qreal m_life;
    qreal m_maxLife;
    int m_fontSize;

public:
    FloatingText(const QPointF& position,
                 const QString& text,
                 const QColor& color,
                 int fontSize = 16)
        : m_position(position),
          m_text(text),
          m_color(color),
          m_life(1.0),
          m_maxLife(1.0),
          m_fontSize(fontSize)
    {
    }

    void update(qreal deltaTime)
    {
        m_life -= deltaTime;
        m_position.setY(m_position.y() - 45.0 * deltaTime);

        if (m_life < 0.0) {
            m_life = 0.0;
        }
    }

    bool isFinished() const
    {
        return m_life <= 0.0;
    }

    void draw(QPainter& painter) const
    {
        qreal ratio = m_life / m_maxLife;
        int alpha = static_cast<int>(255 * ratio);

        QColor drawColor = m_color;
        drawColor.setAlpha(alpha);

        painter.setPen(drawColor);
        painter.setFont(QFont("Microsoft YaHei", m_fontSize, QFont::Bold));

        QRectF textRect(m_position.x() - 80,
                        m_position.y() - 20,
                        160,
                        40);

        painter.drawText(textRect, Qt::AlignCenter, m_text);
    }
};

#endif // FLOATINGTEXT_H
