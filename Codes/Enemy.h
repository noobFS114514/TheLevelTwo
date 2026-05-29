#include "GameObject.h"
#include "Bullet.h"

class Enemy:public GameObject{
    // 敌对实体：分为小怪和boss
    // 属性：血量（由波数初始化） 
    // 接口：进攻 移动（来自GameObject类，从顶部向下移动接近玩家）
    protected:
        int health;
    public:
        void healthInit(const int flags); // 根据波数决定初始生命值 需要一个特定算法决定
        void attack(); // 检测碰撞箱是否和玩家有重合
        void getHurt(Bullet & b); // 受伤：检测子弹碰撞箱和自己是否有重合
        // 移动采用GameObject中的moveDown，自上而下匀速运动
        virtual void moveDown(qreal deltaTime);
        ~Enemy(); // 析构：需要移除碰撞箱
};
void Enemy::healthInit(const int flags){

}

void Enemy::attack(){

}

void Enemy::getHurt(Bullet & b){

}

void Enemy::moveDown(qreal deltaTime){

}

Enemy::~Enemy(){
    
}