#pragma once
#include "GameObject.h"

class Bullet : public GameObject {
protected:
    int damage;
    int pierceCount;    // 穿透剩余次数：>1 表示命中后不消失
    bool isCrit;        // 是否暴击（用于绘制区分）

public:
    Bullet() noexcept {
        damage      = 1;
        pierceCount = 1;
        isCrit      = false;
        speed       = 500.0;
        hitbox      = QRectF(0, 0, 8.0, 15.0); // 默认子弹大小
    }

    // ---- 属性设置 ----
    void setDamage(const int D)        { damage = D; }
    void setPierceCount(int p)         { pierceCount = p; }
    void setCrit(bool c)               { isCrit = c; }

    // ---- 属性访问 ----
    int  getDamage()     const { return damage; }
    int  getPierceCount() const { return pierceCount; }
    bool getCrit()       const { return isCrit; }

    // 穿透结算：返回 true 表示子弹应被移除
    bool consumePierce() {
        pierceCount--;
        return (pierceCount <= 0);
    }

    void attack() {}

    // 子弹更新：向上飞并同步自身碰撞箱
    void updateMovement(qreal deltaTime) {
        moveUp(deltaTime);
        hitbox.moveTo(position);
    }

    ~Bullet() noexcept override = default;
};


class bulletExample : public Bullet {};