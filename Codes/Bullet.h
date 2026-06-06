#pragma once
#include "GameObject.h"
class Bullet : public GameObject {
protected:
    int damage;
public:
    Bullet() noexcept {
        damage = 1;
        speed = 500.0;
        hitbox = QRectF(0, 0, 8.0, 15.0); // 默认子弹大小
    }

    void setDamage(const int D) { damage = D; }
    void attack() {}

    // 子弹更新：向上飞并同步自身碰撞箱
    void updateMovement(qreal deltaTime) {
        moveUp(deltaTime);
        hitbox.moveTo(position);
    }

    ~Bullet() noexcept override = default;
};


class bulletExample:public Bullet{};