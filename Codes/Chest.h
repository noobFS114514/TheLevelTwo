#include "GameObject.h"

class Chest:public GameObject{
    // 奖励箱：击败以后可以获得武器和子弹的升级
    // 属性：生命值
    // 接口：弹出升级窗口（具体升级功能待写：可能有增加子弹数量，增加攻速，子弹效果改为群攻or穿透or分裂 等等）
    protected:
        int health;
    public:
        void setHealth();
        ~Chest(); // 析构时弹出升级选择窗口
}