#pragma once
#include <QRectF>
#include <QPointF>

class GameObject{
    // 游戏内实体：如 玩家本人 小怪 boss 等
    // 属性：位置信息 移动信息 碰撞箱(Q) 等
    // 接口：生成实体
protected:
    QPointF position;   // 实体位置（通常使用窗口坐标系）
    qreal speed;        // 移动速率（每秒移动的像素数）
    QRectF hitbox;      // 碰撞箱（其位置随实体position更新）

public:
    GameObject() = default;
    virtual ~GameObject() = default;

    // 移动接口：按照speed速率随时间匀速移动，deltaTime 单位为秒
    void moveLeft(qreal deltaTime);    // 向左移动
    void moveRight(qreal deltaTime);   // 向右移动
    void moveUp(qreal deltaTime);      // 向上移动
    void moveDown(qreal deltaTime);    // 向下移动

    // 位置与碰撞箱访问接口
    QPointF getPosition() const;              // 获取当前位置
    void setPosition(const QPointF& pos);     // 设置当前位置
    QRectF getHitbox() const;                 // 获取当前碰撞箱

    // 修改实体移动速率的接口
    void setSpeed(qreal speedValue, GameObject* obj);

    // 生成实体：在窗口的特定位置生成相关的实体
    virtual void spawnObject(const QPointF& pos);
};

GameObject::GameObject(){

}

void GameObject::moveLeft(qreal deltaTime){
    position.setX(position.x() - speed * deltaTime);
}

void GameObject::moveRight(qreal deltaTime){
    position.setX(position.x() + speed * deltaTime);
}

void GameObject::moveUp(qreal deltaTime){
    position.setY(position.y() - speed * deltaTime);
}

void GameObject::moveDown(qreal deltaTime){
    position.setY(position.y() + speed * deltaTime);
}

void GameObject::setSpeed(qreal speedValue, GameObject* obj){
    obj->speed = speedValue;
}

void GameObject::setPosition(const QPointF& pos){
    position = pos;
}

QPointF GameObject::getPosition() const{
    return position;
}

QRectF GameObject::getHitbox() const{
    return hitbox;
}
