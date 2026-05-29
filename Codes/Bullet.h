#include "GameObject.h"
class Bullet:public GameObject{
    // 子弹类
    // public继承：位置 移动 碰撞箱
    // 子类属性：子弹伤害 子弹种类（与武器有关，单独放到不同子类中）
    // 对外接口：子弹攻击到目标实体
    protected:
        int damage;
    public:
        void setDamage(const int D);
        void attack();
        ~Bullet(); // 析构：需要移除实体碰撞箱
};
void Bullet::setDamage(const int D){
    damage = D;
}

Bullet::~Bullet(){

}
class bulletExample:public Bullet{};