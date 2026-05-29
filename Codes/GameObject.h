#pragma once
#include <QRectF>
#include <QPointF>

class GameObject{
    // 游戏内实体：如 玩家本人 小怪 boss 等
    // 属性：位置信息 移动信息 碰撞箱(Q) 等
    // 接口：生成实体
protected:
    QPointF position;   // 实体位置（通常使用窗口坐标系）
    QPointF velocity;   // 移动速度（x分量控制左右，y分量控制上下）
    QRectF hitbox;      // 碰撞箱（其位置随实体position更新）

public:
    GameObject() = default;
    virtual ~GameObject() = default;

    // 移动接口（仅有左右和上下两个方向）
    void moveLeft(qreal step);    // 向左移动 step 像素
    void moveRight(qreal step);   // 向右移动 step 像素
    void moveUp(qreal step);      // 向上移动 step 像素
    void moveDown(qreal step);    // 向下移动 step 像素

    // 位置与碰撞箱访问接口
    QPointF getPosition() const;              // 获取当前位置
    void setPosition(const QPointF& pos);     // 设置当前位置
    QRectF getHitbox() const;                 // 获取当前碰撞箱

    // 生成实体：在窗口的特定位置生成相关的实体
    virtual void spawnObject(const QPointF& pos);
};