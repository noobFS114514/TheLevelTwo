#include "GameObject.h"
class Bullet:public GameObject{
    // 子弹类
    // public继承：位置 移动 碰撞箱
    // 子类属性：子弹伤害 子弹种类（与武器有关）
    // 对外接口：子弹攻击到目标实体
};

class bulletExample:public Bullet{};