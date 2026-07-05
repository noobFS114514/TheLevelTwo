#pragma once
#include "Bullet.h"
#include <QList>
#include <QPointF>
#include <QRandomGenerator>
#include <QtMath>
#include <memory>


// ============================================================================
// 武器类型枚举
// ============================================================================
// Gun             — 枪：初始武器，远程水平多发射击
// GoldenCudgel    — 金箍棒：发射后停在碰撞点范围攻击
// EmbroideryNeedle — 绣花针：单发命中后扇形分裂
// ============================================================================
enum class WeaponType {
    Gun,
    GoldenCudgel,
    EmbroideryNeedle
};

// ============================================================================
// 武器基类 Weapon
// ============================================================================
// 设计思路：
//   - 武器由 Player 持有，本身无碰撞箱，只负责管理属性和生成子弹
//   - 所有可升级属性集中管理，由宝箱系统通过 upgradeXxx() 修改
//   - 不同武器类型通过子类重写 fire() 实现各自的弹幕模式
//   - 构造函数中的初始数值参考敌人血量 (1 + wave/3) 和宝箱血量 (~3~5)
//
// 算法细节：
//   1. 暴击判定：每发子弹独立随机 [0,99]，若 < critRate×100 则伤害×critMultiplier
//   2. 攻速叠加：(1-ratio1)×(1-ratio2)×... 乘法叠加保证冷却始终为正
//   3. 数值取舍：伤害/数量为 int（整数运算），比率/冷却为 qreal（浮点）
// ============================================================================

class Weapon {
protected:
    WeaponType m_type;          // 武器种类标识

    // ---- 通用攻击属性 ----
    int    m_damage;            // 单发基础伤害
    int    m_bulletCount;       // 每次射击子弹数（水平排列）
    qreal  m_attackCooldown;    // 攻击冷却（秒），两次射击的最小间隔
    qreal  m_bulletSpeed;       // 子弹飞行速度（像素/秒）

    // ---- 进阶属性 ----
    int    m_pierceCount;       // 穿透数：=1 命中即消失，>1 可穿透多个敌人后消失
    qreal  m_critRate;          // 暴击率 [0.0, 1.0]
    qreal  m_critMultiplier;    // 暴击倍率（默认 2.0 = 双倍伤害）

    // ---- 金箍棒专用 ----
    qreal  m_duration;          // 在场时长（秒），超时自动消失
    qreal  m_attackInterval;    // 伤害结算间隔（秒），每隔此时间对范围内敌人造成 1 次伤害
    qreal  m_sizeScale;         // 体积缩放因子，1.0 = 原始大小

    // ---- 绣花针专用 ----
    int    m_splitCount;        // 分裂数量：命中后扇形散开的小针数量
    qreal  m_splitAngle;        // 分裂扇形总夹角（度），小针在 [-angle/2, +angle/2] 内均匀分布
    int    m_splitTimes;        // 最大分裂次数：0=不分裂，每分裂一次计数-1

public:
    // ========================================================================
    // 构造函数
    // 初始数值参考：
    //   敌人血量 = 1 + wave/3          （wave=1 → 1血, wave=3 → 2血, wave=6 → 3血）
    //   宝箱血量 ≈ 3~5                （需 3~5 发子弹出宝箱）
    //   初始 DPS ≈ 伤害 × 频率 = 1 × 4 = 4/秒
    //   随着升级推进，DPS 逐步提升以应对高波次敌人
    // ========================================================================
    Weapon() noexcept {
        m_type           = WeaponType::Gun;
        m_damage         = 1;
        m_bulletCount    = 1;
        m_attackCooldown = 0.25;        // 每秒 4 发
        m_bulletSpeed    = 500.0;
        m_pierceCount    = 1;
        m_critRate       = 0.05;        // 5%
        m_critMultiplier = 2.0;
        m_duration       = 3.0;
        m_attackInterval = 1.0;
        m_sizeScale      = 1.0;
        m_splitCount     = 4;
        m_splitAngle     = 60.0;
        m_splitTimes     = 1;
    }

    virtual ~Weapon() noexcept = default;

    virtual std::unique_ptr<Weapon> clone() const {
    return std::make_unique<Weapon>(*this);
}


    // ========================================================================
    // 升级接口（由宝箱系统 Chest/PowerUp 调用）
    //
    // 升级幅度参考（由宝箱随机决定，此处提供修改入口）：
    //   普通（高概率 ~60%）：伤害+1, 弹数+1, 攻速+10%, 穿透+1, 暴击+5%
    //   稀有（低概率 ~25%）：伤害+2, 弹数+2, 攻速+20%, 穿透+2, 暴击+10%
    //   史诗（极低 ~10%）：伤害+3, 弹数+3, 攻速+30%, 穿透+3, 暴击+20%
    // ========================================================================

    // 伤害升级：直接累加，保证 DPS 线性增长
    virtual void upgradeDamage(int add) { m_damage += add; }

    // 子弹数量升级：每次 +1 或 +2
    virtual void upgradeBulletCount(int add) { m_bulletCount += add; }

    // 攻速提升：ratio = 0.10 表示冷却减少 10%
    // 算法：乘法叠加 (1-0.1)*(1-0.25)*... 避免冷却归零
    virtual void upgradeAttackSpeed(qreal ratio) {
        m_attackCooldown *= (1.0 - ratio);
    }

    // 穿透升级：子弹可穿透更多敌人
    virtual void upgradePierce(int add) { m_pierceCount += add; }

    // 暴击率升级：add = 0.05 表示绝对 +5%，上限 100%
    virtual void upgradeCritRate(qreal add) {
        m_critRate += add;
        if (m_critRate > 1.0) m_critRate = 1.0;
    }

    // 金箍棒：在场时间延长（秒）
    virtual void upgradeDuration(qreal add) { m_duration += add; }

    // 金箍棒：体积增大，ratio = 0.20 表示放大 20%
    virtual void upgradeSize(qreal ratio) { m_sizeScale *= (1.0 + ratio); }

    // 绣花针：分裂数量增加
    virtual void upgradeSplitCount(int add) { m_splitCount += add; }

    // 绣花针：分裂次数增加
    virtual void upgradeSplitTimes(int add) { m_splitTimes += add; }

    // ========================================================================
    // 访问接口
    // ========================================================================
    WeaponType getType()           const { return m_type; }
    int        getDamage()         const { return m_damage; }
    int        getBulletCount()    const { return m_bulletCount; }
    qreal      getAttackCooldown() const { return m_attackCooldown; }
    qreal      getBulletSpeed()    const { return m_bulletSpeed; }
    int        getPierceCount()    const { return m_pierceCount; }
    qreal      getCritRate()       const { return m_critRate; }
    qreal      getDuration()       const { return m_duration; }
    qreal      getAttackInterval() const { return m_attackInterval; }
    qreal      getSizeScale()      const { return m_sizeScale; }
    int        getSplitCount()     const { return m_splitCount; }
    qreal      getSplitAngle()     const { return m_splitAngle; }
    int        getSplitTimes()     const { return m_splitTimes; }

    // ========================================================================
    // 核心接口：开火 fire()
    // ========================================================================
    // 参数：
    //   origin  — 发射起点坐标（由调用方计算，通常是玩家炮口中心）
    //   bullets — 输出列表，新生成的子弹追加到此列表末尾
    //
    // 默认行为（Gun 枪）算法：
    //   Step 1. 计算水平排列总宽度 = (bulletCount - 1) × spacing
    //   Step 2. 以 origin 为中心，均匀生成 bulletCount 发子弹
    //   Step 3. 每发子弹独立暴击判定：
    //           生成 [0, 99] 随机整数 r
    //           若 r < critRate × 100   →  伤害 = damage × critMultiplier，标记 isCrit
    //           否则                     →  伤害 = damage
    //   Step 4. 设置穿透次数 = m_pierceCount
    // ========================================================================
    virtual void fire(const QPointF& origin, QList<Bullet>& bullets) {
        const qreal spacing = 10.0;  // 相邻子弹水平间距（像素）
        qreal totalWidth = (m_bulletCount - 1) * spacing;
        qreal startX = origin.x() - totalWidth / 2.0;

        for (int i = 0; i < m_bulletCount; ++i) {
            Bullet bullet;
            bullet.setPosition(QPointF(startX + i * spacing, origin.y()));
            bullet.setSpeed(m_bulletSpeed);

            // ---- 暴击独立判定 ----
            int  finalDmg = m_damage;
            bool crit     = (QRandomGenerator::global()->bounded(100)
                             < static_cast<int>(m_critRate * 100));
            if (crit) {
                finalDmg = static_cast<int>(m_damage * m_critMultiplier);
            }
            bullet.setDamage(finalDmg);
            bullet.setCrit(crit);
            bullet.setPierceCount(m_pierceCount);

            bullets.append(bullet);
        }
    }

    // 设置武器类型（宝箱解锁新武器时调用）
    virtual void setType(WeaponType type) { m_type = type; }
};


// ============================================================================
// Gun — 枪（初始武器）
// ============================================================================
// 特性：远程单发/多发射击，子弹水平排列向上飞行
// 升级方向：数量↑ 伤害↑ 暴击↑ 穿透↑ 攻速↑
//
// 大招·火力全开：
//   解锁后短时间内高速连射 n 发（由外部计时器 + fireBurst() 配合实现）
//   原理：临时增大 bulletCount，调用 fire() 后恢复原值
// ============================================================================
class Gun : public Weapon {
public:
    Gun() noexcept {
        m_type           = WeaponType::Gun;
        m_damage         = 1;
        m_bulletCount    = 1;
        m_attackCooldown = 0.25;
        m_bulletSpeed    = 500.0;
        m_pierceCount    = 1;
        m_critRate       = 0.05;
        m_critMultiplier = 2.0;
    }

    std::unique_ptr<Weapon> clone() const override {
    return std::make_unique<Gun>(*this);
}

    // 火力全开：在 burstCooldown 间隔内连续发射 count 发
    // 调用时机：大招计时器触发 → 连续多次调用 fireBurst()
    void fireBurst(const QPointF& origin, QList<Bullet>& bullets, int count) {
        int savedCount = m_bulletCount;
        m_bulletCount = count;
        fire(origin, bullets);
        m_bulletCount = savedCount;
    }
};


// ============================================================================
// GoldenCudgel — 金箍棒（范围攻击武器）
// ============================================================================
// 特性：
//   - 每 m_attackCooldown 秒发射一个金箍棒
//   - 金箍棒飞至与敌人碰撞处停下（由碰撞检测配合），持续旋转造成范围伤害
//   - 在场 m_duration 秒后自动消失
//   - 每隔 m_attackInterval 秒对范围内所有敌人结算 1 次 m_damage 伤害
//
// 升级方向：在场时长↑ 攻击间隔↓（攻速） 体积↑
// 亡语·掷地有声（解锁）：消失时产生 3× 范围爆炸
// 大招·救命毫毛（解锁）：命中后向四周分裂 4 个小金箍棒
//
// 数值设计：
//   范围伤害 3/跳（高于枪单发 1，但攻速慢、需要预判）
//   冷却 2s、持续 3s  →  可同时场上最多存在 2 个金箍棒
//   DPS = 3×3跳/2s冷却 ≈ 4.5/秒，两棒叠加 ≈ 9/秒
// ============================================================================
class GoldenCudgel : public Weapon {
public:
    GoldenCudgel() noexcept {
        m_type           = WeaponType::GoldenCudgel;
        m_damage         = 3;           // 每次范围伤害 3
        m_bulletCount    = 1;           // 每次发射 1 个
        m_attackCooldown = 2.0;         // 每 2 秒可发射一次
        m_bulletSpeed    = 350.0;       // 飞行速度略慢于子弹
        m_duration       = 3.0;         // 在场持续 3 秒
        m_attackInterval = 1.0;         // 每秒结算 1 次伤害
        m_sizeScale      = 1.0;
        m_critRate       = 0.05;
        m_critMultiplier = 2.0;
    }

    std::unique_ptr<Weapon> clone() const override {
    return std::make_unique<GoldenCudgel>(*this);
}


    // 金箍棒的 fire()：发射单个锚点子弹
    // 外部需要配合碰撞检测：子弹命中敌人后停止移动，由计时器驱动范围伤害
    void fire(const QPointF& origin, QList<Bullet>& bullets) override {
        Bullet bullet;
        bullet.setPosition(origin);
        bullet.setSpeed(m_bulletSpeed);
        bullet.setDamage(m_damage);
        bullet.setPierceCount(999);     // 不会被消耗（由 duration 控制消失）
        bullets.append(bullet);
    }
};


// ============================================================================
// EmbroideryNeedle — 绣花针（分裂型武器）
// ============================================================================
// 特性：
//   - 每次发射 1 根主针
//   - 主针命中敌人后：以命中点为起点，扇形散开 m_splitCount 根小针
//   - 小针伤害 = 主针伤害 × 0.5
//   - 每根小针也可继续分裂（受 m_splitTimes 限制）
//
// 升级方向：分裂数量↑ 分裂次数↑ 攻速↑
//
// 分裂算法（splitAt）：
//   Step 1. 以命中点为起点
//   Step 2. 扇形范围 = [baseDirection - splitAngle/2, baseDirection + splitAngle/2]
//           其中 baseDirection = 正上方（屏幕 Y 轴向下，故用 -90°）
//   Step 3. 均匀取 splitCount 个方向，相邻夹角 = splitAngle / (splitCount - 1)
//           例：splitCount=4, splitAngle=60° → 方向 = {-30°, -10°, +10°, +30°} 相对正上
//   Step 4. 小针速度 = bulletSpeed × 0.7（略低于主针）
//   Step 5. 小针分裂次数 = remainingTimes - 1
//
// 数值设计：
//   单发伤害 3（高伤害补偿低频率）
//   冷却 1.5s（频率低于枪）
//   初始 4 根小针 × 1 伤害 = 最高 3+4=7 范围伤害（需命中+分裂）
// ============================================================================
class EmbroideryNeedle : public Weapon {
public:
    EmbroideryNeedle() noexcept {
        m_type           = WeaponType::EmbroideryNeedle;
        m_damage         = 3;            // 单发伤害（命中后分裂）
        m_bulletCount    = 1;            // 每次发射 1 根主针
        m_attackCooldown = 1.5;          // 每 1.5 秒发射一次
        m_bulletSpeed    = 400.0;
        m_splitCount     = 4;            // 初始分裂 4 根小针
        m_splitAngle     = 60.0;         // 扇形总夹角 60°
        m_splitTimes     = 1;            // 初始可分裂 1 次
        m_critRate       = 0.05;
        m_critMultiplier = 2.0;
    }

    std::unique_ptr<Weapon> clone() const override {
    return std::make_unique<EmbroideryNeedle>(*this);
}


    // 绣花针的 fire()：单发主针
    void fire(const QPointF& origin, QList<Bullet>& bullets) override {
        Bullet bullet;
        bullet.setPosition(origin);
        bullet.setSpeed(m_bulletSpeed);

        int  finalDmg = m_damage;
        bool crit = (QRandomGenerator::global()->bounded(100)
                     < static_cast<int>(m_critRate * 100));
        if (crit) {
            finalDmg = static_cast<int>(m_damage * m_critMultiplier);
        }
        bullet.setDamage(finalDmg);
        bullet.setCrit(crit);
        bullet.setPierceCount(1);        // 主针命中即触发分裂

        bullets.append(bullet);
    }

    // ========================================================================
    // 分裂逻辑 splitAt()
    // ========================================================================
    // 在命中位置生成扇形小针阵列
    //
    // 参数：
    //   hitPos         — 命中位置，作为分裂起点
    //   baseDamage     — 主针造成的伤害，小针伤害 = baseDamage / 2
    //   remainingTimes — 剩余可分裂次数（-1 后传给小针）
    //   bullets        — 输出列表
    //
    // 使用时机：碰撞检测确认主针命中敌人 → 调用本方法生成小针
    // ========================================================================
    void splitAt(const QPointF& hitPos, int baseDamage, int remainingTimes,
                 QList<Bullet>& bullets)
    {
        if (remainingTimes <= 0) return;

        qreal halfAngle   = m_splitAngle / 2.0;
        qreal baseDir     = -90.0;       // 正上方（屏幕坐标下 Y 为正）

        for (int i = 0; i < m_splitCount; ++i) {
            // Step 2-3: 计算当前小针的角度
            // offset 范围：从 -halfAngle 到 +halfAngle 均匀分布
            qreal offset = (m_splitCount == 1)
                ? 0.0
                : -halfAngle + (i * m_splitAngle / (m_splitCount - 1));
            qreal angleDeg = baseDir + offset;
            qreal angleRad = qDegreesToRadians(angleDeg);

            Bullet needle;
            needle.setPosition(hitPos);

            // Step 4: 方向速度分解
            // 当前 Bullet 默认向上飞行，扇形小针需要自定义速度方向
            // 此处以 bulletSpeed × 0.7 作为速率，方向由角度决定
            qreal splitSpeed = m_bulletSpeed * 0.7f;
            qreal vx = splitSpeed * qCos(angleRad);
            qreal vy = splitSpeed * qSin(angleRad);

            // 注意：当前 Bullet::updateMovement() 仅支持向上移动
            //       后续需扩展 Bullet 子类（如 SplitBullet）以支持任意方向飞行
            //       此处暂存方向和速率，供后续扩展使用
            needle.setSpeed(splitSpeed);

            // Step 1 & 5: 伤害减半，分裂次数-1
            needle.setDamage(baseDamage / 2);
            needle.setPierceCount(1);
            // 小针的分裂次数在主针基础上 -1
            // needle.setSplitTimes(remainingTimes - 1);  // 待扩展

            bullets.append(needle);
        }
    }
};


// ============================================================================
// 保留旧占位类（向后兼容）
// ============================================================================
class weaponExample : public Gun {};
