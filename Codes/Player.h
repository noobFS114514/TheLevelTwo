#pragma once
#include "GameObject.h"
#include "Weapon.h"
#include "Enemy.h"
#include <memory>

class Player : public GameObject {
protected:
    std::unique_ptr<Weapon> weapon;
    int weaponLevel = 1;
    int health = 3;
    qreal boundaryLeft = 0.0;
    qreal boundaryRight = 600.0;

public:
    Player() {
    speed = 300.0;
    hitbox = QRectF(0, 0, 112.0, 72.0);
    weapon = std::make_unique<Gun>();
}

Player(const Player& other)
    : GameObject(other),
      weapon(other.weapon ? other.weapon->clone() : nullptr),
      weaponLevel(other.weaponLevel),
      health(other.health),
      boundaryLeft(other.boundaryLeft),
      boundaryRight(other.boundaryRight)
{
}

Player& operator=(const Player& other)
{
    if (this == &other) {
        return *this;
    }

    position = other.position;
    speed = other.speed;
    hitbox = other.hitbox;

    weapon = other.weapon ? other.weapon->clone() : nullptr;
    weaponLevel = other.weaponLevel;
    health = other.health;
    boundaryLeft = other.boundaryLeft;
    boundaryRight = other.boundaryRight;

    return *this;
}

Player(Player&& other) noexcept = default;
Player& operator=(Player&& other) noexcept = default;

Weapon& getWeapon() {
    return *weapon;
}

void recordWeaponUpgrade() {
    if (weaponLevel < 3) {
        ++weaponLevel;
    }
}

void setWeaponType(WeaponType type)
{
    switch (type) {
    case WeaponType::Gun:
        weapon = std::make_unique<Gun>();
        break;
    case WeaponType::GoldenCudgel:
        weapon = std::make_unique<GoldenCudgel>();
        break;
    case WeaponType::EmbroideryNeedle:
        weapon = std::make_unique<EmbroideryNeedle>();
        break;
    }

    weaponLevel = 1;
}


const Weapon& getWeapon() const {
    return *weapon;
}

int getWeaponLevel() const {
    return weaponLevel;
}

bool increaseWeaponLevel() {
    if (weaponLevel >= 3) {
        return false;
    }

    ++weaponLevel;

    if (weapon) {
        weapon->upgradeBulletCount(1);
        weapon->upgradeAttackSpeed(0.15);
    }

    return true;
}


qreal getFireCooldown() const {
    return weapon ? weapon->getAttackCooldown() : 0.25;
}

QRectF getCollisionBox() const
{
    return hitbox.adjusted(24.0, 18.0, -24.0, -12.0);
}


    // 【核心】给队友的空壳接口全部补齐空实现体 {}
    void setHealth(const int H) { health = H; }
    void setWeapon() {}
    void attack() {}
    void getHurt(Enemy& enemy) { (void)enemy; } // 压制未使用警告

    void spawnPlayer(qreal H, qreal L, qreal R) {
        boundaryLeft = L;
        boundaryRight = R;
        qreal centerX = (L + R) / 2.0;
        position = QPointF(centerX - hitbox.width() / 2.0, H - hitbox.height() / 2.0);
        hitbox.moveTo(position);
    }

    void moveLeft(qreal deltaTime) override {
        qreal newX = position.x() - speed * deltaTime;
        if (newX < boundaryLeft) newX = boundaryLeft;
        position.setX(newX);
        hitbox.moveTo(position);
    }

    void moveRight(qreal deltaTime) override {
        qreal newX = position.x() + speed * deltaTime;
        qreal maxX = boundaryRight - hitbox.width();
        if (newX > maxX) newX = maxX;
        position.setX(newX);
        hitbox.moveTo(position);
    }

    ~Player() noexcept override {}
};
