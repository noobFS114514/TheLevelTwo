#pragma once
#include <QRectF>
#include <QPointF>

class GameObject {
protected:
    QPointF position;   // 实体位置
    qreal speed = 0.0;  // 移动速率
    QRectF hitbox;      // 碰撞箱

public:
    GameObject() noexcept = default;

    // 【核心】必须保证虚析构函数有具体实现体，否则会产生未定义符号
    virtual ~GameObject() noexcept {}

    // 所有虚移动接口，必须带上完整的 {} 实现
    virtual void moveLeft(qreal deltaTime) { position.setX(position.x() - speed * deltaTime); }
    virtual void moveRight(qreal deltaTime) { position.setX(position.x() + speed * deltaTime); }
    virtual void moveUp(qreal deltaTime) { position.setY(position.y() - speed * deltaTime); }
    virtual void moveDown(qreal deltaTime) { position.setY(position.y() + speed * deltaTime); }

    // 访问接口
    QPointF getPosition() const { return position; }
    void setPosition(const QPointF& pos) {
    position = pos;
    hitbox.moveTo(pos);
}

QRectF getHitbox() const { return hitbox; }

void setSpeed(qreal speedValue) {
    speed = speedValue;
}


    virtual void spawnObject(const QPointF& pos) { position = pos; }
};