#include "GameObject.h"
#include "Weapon.h"

class Player:public GameObject{
    // 玩家类
    // public继承属性：位置 移动 碰撞箱
    // 子类属性：武器种类 生命值（？还未确定是碰撞即死还是血条机制） 使用的武器
    // 对外接口：发射子弹（根据武器） 受伤（？） 选择升级
};