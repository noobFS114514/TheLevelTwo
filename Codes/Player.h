#include "GameObject.h"
#include "Weapon.h"
#include "Enemy.h"

class Player:public GameObject{
    // 玩家类
    // public继承属性：位置 移动 碰撞箱
    // 子类属性：生命值（？还未确定是碰撞即死还是血条机制） 使用的武器
    // 对外接口：发射子弹（根据武器） 受伤（？） 选择升级
    protected:
        Weapon weapon;
        int health;
        qreal boundaryLeft;   // 可移动区域的左边界
        qreal boundaryRight;  // 可移动区域的右边界
    public:
        void setHealth(const int H);
        void setWeapon();
        void attack(); // 根据武器进行发射子弹
        void getHurt(Enemy& enemy); // 根据敌人决定受伤程度
        // 重写生成对象：碰撞箱中心位于窗口下方H像素处、左右边框[L,R]
        void spawnPlayer(qreal H, qreal L, qreal R);
        void moveLeft(qreal deltaTime);
        void moveRight(qreal deltaTime);
};

Player::Player() {

}

void Player::setHealth(const int H){
    health = H;
}

void Player::setWeapon(){
    
}

void Player::attack() {

}

void Player::getHurt(Enemy& enemy) {

}
void Player::spawnPlayer(qreal H, qreal L, qreal R) {
    // 记录左右边界
    boundaryLeft = L;
    boundaryRight = R;
    // 碰撞箱中心置于 (L+R)/2, H
    qreal centerX = (L + R) / 2.0;
    position.setX(centerX - hitbox.width() / 2.0);
    position.setY(H - hitbox.height() / 2.0);
}

void Player::moveLeft(qreal deltaTime) {
    // 按住左方向键时每秒向左移动 speed 像素
    qreal newX = position.x() - speed * deltaTime;
    // 检测碰撞箱左边缘是否超出左边界
    if (newX < boundaryLeft) {
        newX = boundaryLeft;
    }
    position.setX(newX);
}

void Player::moveRight(qreal deltaTime) {
    // 按住右方向键时每秒向右移动 speed 像素
    qreal newX = position.x() + speed * deltaTime;
    // 检测碰撞箱右边缘是否超出右边界
    qreal maxX = boundaryRight - hitbox.width();
    if (newX > maxX) {
        newX = maxX;
    }
    position.setX(newX);
}
