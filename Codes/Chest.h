#pragma once
#include "GameObject.h"
#include "Weapon.h"
#include <QDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QColor>
#include <QPen>
#include <QRandomGenerator>
#include <QVector>
#include <QFont>
#include <QFrame>

// ============================================================================
// UpgradeOption — 单个升级选项的描述
// ============================================================================
// 由宝箱随机生成 3 个，填充到升级选择窗口中
//
// upgradeFunc 用一个整数 ID 标记升级类型，value 为升级幅度：
//   ID 0: upgradeDamage(int value)               — 伤害 +value
//   ID 1: upgradeBulletCount(int value)          — 弹数 +value
//   ID 2: upgradeAttackSpeed(qreal value)        — 攻速 +value%（value=0.10 → 10%）
//   ID 3: upgradePierce(int value)               — 穿透 +value
//   ID 4: upgradeCritRate(qreal value)           — 暴击率 +value（value=0.05 → 5%）
//   ID 5: upgradeDuration(qreal value)           — 金箍棒在场 +value 秒
//   ID 6: upgradeSize(qreal value)               — 金箍棒体积 +value%（value=0.20 → 20%）
//   ID 7: upgradeSplitCount(int value)           — 绣花针分裂 +value
//   ID 8: upgradeSplitTimes(int value)           — 绣花针分裂次数 +value
// ============================================================================
struct UpgradeOption {
    QString name;         // 升级名称（如「伤害提升」）
    QString desc;         // 详细描述（如「子弹伤害 +10」）
    int     upgradeId;    // 升级类型 ID（0~8）
    qreal   value;        // 升级数值
};

// ============================================================================
// 预声明
// ============================================================================
class Weapon;

// ============================================================================
// UpgradeDialog — 宝箱升级选择窗口
// ============================================================================
// 外观：
//   - 固定 300×400 模态窗口，深色主题与游戏风格统一
//   - 顶部标题「获得升级！」
//   - 3 个纵向排列的选项按钮，每个显示升级名称 + 描述
//   - 底部提示
//
// 交互流程：
//   1. 宝箱死亡 → 构造 UpgradeDialog，传入 Weapon* 和 wave
//   2. generateOptions() 随机生成 3 个升级选项
//   3. 玩家点击任一按钮 → applyUpgrade() → accept() 关闭窗口
//   4. 调用方通过 exec() 阻塞等待选择完成
//
// 选项生成算法（generateOptions）：
//   - 根据当前武器类型过滤可选升级池
//   - 高波次有概率出现更稀有的数值（wave≥5 时数值 ×1.5）
//   - 随机不重复抽取 3 个
// ============================================================================
class UpgradeDialog : public QDialog {
    Q_OBJECT

public:
    explicit UpgradeDialog(Weapon* weapon, int wave, QWidget* parent = nullptr)
        : QDialog(parent), m_weapon(weapon)
    {
        // ---- 窗口基本设置 ----
        setWindowTitle(QStringLiteral("获得升级！"));
        setFixedSize(300, 400);
        setModal(true);                       // 阻塞游戏操作
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        // 圆角 + 深色背景：由 paintEvent + 样式表共同实现
        setAttribute(Qt::WA_StyledBackground, true);
        setStyleSheet(
            "UpgradeDialog {"
            "  background-color: #1e1e2e;"
            "  border: 2px solid #c9a44b;"
            "  border-radius: 12px;"
            "}"
        );

        // ---- 主布局 ----
        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(20, 24, 20, 20);
        mainLayout->setSpacing(14);

        // 标题
        auto* titleLabel = new QLabel(QStringLiteral("获得升级！"));
        titleLabel->setAlignment(Qt::AlignCenter);
        titleLabel->setStyleSheet(
            "QLabel {"
            "  color: #ffd866;"
            "  font-size: 22px;"
            "  font-weight: bold;"
            "  font-family: \"Microsoft YaHei\", Arial;"
            "  background: transparent;"
            "  border: none;"
            "}"
        );
        mainLayout->addWidget(titleLabel);

        // 分隔线
        auto* line = new QFrame();
        line->setFrameShape(QFrame::HLine);
        line->setStyleSheet("QFrame { color: #c9a44b; background: transparent; }");
        mainLayout->addWidget(line);

        // 波次提示
        auto* waveLabel = new QLabel(QStringLiteral("第 %1 波").arg(wave));
        waveLabel->setAlignment(Qt::AlignCenter);
        waveLabel->setStyleSheet(
            "QLabel {"
            "  color: #8b949e;"
            "  font-size: 13px;"
            "  font-family: \"Microsoft YaHei\", Arial;"
            "  background: transparent;"
            "  border: none;"
            "}"
        );
        mainLayout->addWidget(waveLabel);

        // ---- 生成 3 个随机选项 ----
        generateOptions(wave);

        // ---- 选项按钮 ----
        // 每个按钮显示两行：第一行加粗名称，第二行灰色描述
        for (int i = 0; i < m_options.size(); ++i) {
            const auto& opt = m_options[i];

            QString btnText = opt.name + "\n" + opt.desc;

            auto* btn = new QPushButton(btnText);
            btn->setMinimumHeight(64);
            btn->setCursor(Qt::PointingHandCursor);
            btn->setStyleSheet(
                "QPushButton {"
                "  background-color: #2d2d3f;"
                "  color: #e0e0e0;"
                "  border: 1px solid #4a4a5a;"
                "  border-radius: 8px;"
                "  font-size: 14px;"
                "  font-family: \"Microsoft YaHei\", Arial;"
                "  text-align: left;"
                "  padding: 10px 14px;"
                "}"
                "QPushButton:hover {"
                "  background-color: #3d3d55;"
                "  border-color: #c9a44b;"
                "}"
                "QPushButton:pressed {"
                "  background-color: #4d4d65;"
                "}"
            );

            // 用 lambda 连接点击：应用升级、关闭窗口
            connect(btn, &QPushButton::clicked, this, [this, i]() {
                applyUpgrade(m_options[i]);
                accept();   // QDialog::accept() → exec() 返回 QDialog::Accepted
            });

            mainLayout->addWidget(btn);
        }

        // 底部提示
        mainLayout->addStretch();
        auto* hintLabel = new QLabel(QStringLiteral("点击一个选项以获得升级"));
        hintLabel->setAlignment(Qt::AlignCenter);
        hintLabel->setStyleSheet(
            "QLabel {"
            "  color: #6b6b7b;"
            "  font-size: 12px;"
            "  font-family: \"Microsoft YaHei\", Arial;"
            "  background: transparent;"
            "  border: none;"
            "}"
        );
        mainLayout->addWidget(hintLabel);
    }

private:
    Weapon* m_weapon;
    QVector<UpgradeOption> m_options;

    // ========================================================================
    // 应用升级到武器
    // 根据 UpgradeOption::upgradeId 调用 Weapon 对应的 upgradeXxx() 方法
    // ========================================================================
    void applyUpgrade(const UpgradeOption& opt) {
        if (!m_weapon) return;

        switch (opt.upgradeId) {
        case 0: // 伤害
            m_weapon->upgradeDamage(static_cast<int>(opt.value));
            break;
        case 1: // 子弹数量
            m_weapon->upgradeBulletCount(static_cast<int>(opt.value));
            break;
        case 2: // 攻速（value 直接传比例，如 0.10 = 冷却减10%）
            m_weapon->upgradeAttackSpeed(opt.value);
            break;
        case 3: // 穿透
            m_weapon->upgradePierce(static_cast<int>(opt.value));
            break;
        case 4: // 暴击率
            m_weapon->upgradeCritRate(opt.value);
            break;
        case 5: // 金箍棒在场时间
            m_weapon->upgradeDuration(opt.value);
            break;
        case 6: // 金箍棒体积
            m_weapon->upgradeSize(opt.value);
            break;
        case 7: // 绣花针分裂数量
            m_weapon->upgradeSplitCount(static_cast<int>(opt.value));
            break;
        case 8: // 绣花针分裂次数
            m_weapon->upgradeSplitTimes(static_cast<int>(opt.value));
            break;
        default:
            break;
        }
    }

    // ========================================================================
    // 随机生成 3 个不重复的升级选项
    //
    // 算法：
    //   1. 根据武器类型构建候选池（不同武器侧重不同升级）
    //   2. 高波次（wave≥5）稀有数值概率翻倍
    //   3. 从池中无放回随机抽取 3 个
    //
    // 候选池权重设计（参考宝箱血量 ~3~5，平衡早期难度）：
    //   普通（权重 6）：伤害+1, 弹数+1, 攻速+10%, 穿透+1, 暴击+5%
    //   稀有（权重 3）：伤害+2, 弹数+2, 攻速+20%, 穿透+2, 暴击+10%
    //   史诗（权重 1）：伤害+3, 弹数+3, 攻速+30%, 穿透+3, 暴击+20%
    // ========================================================================
    void generateOptions(int wave) {
        struct PoolEntry {
            QString   name;
            QString   descTpl;   // 描述模板，%1 会被替换为数值
            int       upgradeId;
            qreal     value;
            int       weight;    // 抽取权重
        };

        QVector<PoolEntry> pool;
        bool highWave = (wave >= 5);  // 高波次稀有度加权

        // ---- 通用升级（所有武器） ----
        // 伤害类（绝对数值 +1/+2/+3，基数 1）
        pool.append({QStringLiteral("伤害提升 I"),   QStringLiteral("子弹伤害 +1"),     0, 1.0,  6});
        pool.append({QStringLiteral("伤害提升 II"),  QStringLiteral("子弹伤害 +2"),     0, 2.0,  3});
        pool.append({QStringLiteral("伤害提升 III"), QStringLiteral("子弹伤害 +3"),     0, 3.0,  1});
        // 弹数类
        pool.append({QStringLiteral("弹匣扩容 I"),   QStringLiteral("子弹数量 +1"),     1, 1.0,  6});
        pool.append({QStringLiteral("弹匣扩容 II"),  QStringLiteral("子弹数量 +2"),     1, 2.0,  3});
        pool.append({QStringLiteral("弹匣扩容 III"), QStringLiteral("子弹数量 +3"),     1, 3.0,  1});
        // 攻速类
        pool.append({QStringLiteral("射速提升 I"),   QStringLiteral("攻击速度 +10%"),   2, 0.10, 6});
        pool.append({QStringLiteral("射速提升 II"),  QStringLiteral("攻击速度 +20%"),   2, 0.20, 3});
        pool.append({QStringLiteral("射速提升 III"), QStringLiteral("攻击速度 +30%"),   2, 0.30, 1});
        // 穿透类
        pool.append({QStringLiteral("穿刺 I"),       QStringLiteral("穿透数量 +1"),     3, 1.0,  6});
        pool.append({QStringLiteral("穿刺 II"),      QStringLiteral("穿透数量 +2"),     3, 2.0,  3});
        pool.append({QStringLiteral("穿刺 III"),     QStringLiteral("穿透数量 +3"),     3, 3.0,  1});
        // 暴击类
        pool.append({QStringLiteral("暴击强化 I"),   QStringLiteral("暴击率 +5%"),      4, 0.05, 6});
        pool.append({QStringLiteral("暴击强化 II"),  QStringLiteral("暴击率 +10%"),     4, 0.10, 3});
        pool.append({QStringLiteral("暴击强化 III"), QStringLiteral("暴击率 +20%"),     4, 0.20, 1});

        // ---- 金箍棒专用升级 ----
        WeaponType wt = m_weapon ? m_weapon->getType() : WeaponType::Gun;
        if (wt == WeaponType::GoldenCudgel) {
            pool.append({QStringLiteral("延长时间 I"),   QStringLiteral("在场时长 +2 秒"),    5, 2.0,  5});
            pool.append({QStringLiteral("延长时间 II"),  QStringLiteral("在场时长 +4 秒"),    5, 4.0,  2});
            pool.append({QStringLiteral("体积增大 I"),   QStringLiteral("金箍棒体积 +20%"),  6, 0.20, 5});
            pool.append({QStringLiteral("体积增大 II"),  QStringLiteral("金箍棒体积 +40%"),  6, 0.40, 2});
        }

        // ---- 绣花针专用升级 ----
        if (wt == WeaponType::EmbroideryNeedle) {
            pool.append({QStringLiteral("分裂增多 I"),   QStringLiteral("分裂数量 +2"),      7, 2.0,  5});
            pool.append({QStringLiteral("分裂增多 II"),  QStringLiteral("分裂数量 +4"),      7, 4.0,  2});
            pool.append({QStringLiteral("多次分裂 I"),   QStringLiteral("分裂次数 +1"),      8, 1.0,  4});
            pool.append({QStringLiteral("多次分裂 II"),  QStringLiteral("分裂次数 +2"),      8, 2.0,  1});
        }

        // ---- 无放回加权随机抽取 3 个 ----
        // 算法：累加权重 → 随机 [0, totalWeight) → 找到对应条目 → 移除 → 重复
        QVector<int> indices;
        for (int i = 0; i < pool.size(); ++i) indices.append(i);

        m_options.clear();
        QVector<int> selectedIndices;

        for (int pick = 0; pick < 3 && !indices.isEmpty(); ++pick) {
            // 计算剩余池的总权重
            int totalWeight = 0;
            for (int idx : indices) {
                int w = pool[idx].weight;
                // 高波次稀有加权：权重 ≤ 3 的条目额外 +1
                if (highWave && pool[idx].weight <= 3) w += 1;
                totalWeight += w;
            }

            // 按权重随机选择
            int roll = QRandomGenerator::global()->bounded(totalWeight);
            int cumulative = 0;
            int chosen = -1;
            int chosenLocalIdx = -1;

            for (int j = 0; j < indices.size(); ++j) {
                int idx = indices[j];
                int w = pool[idx].weight;
                if (highWave && pool[idx].weight <= 3) w += 1;
                cumulative += w;
                if (roll < cumulative) {
                    chosen = idx;
                    chosenLocalIdx = j;
                    break;
                }
            }

            if (chosen < 0) break;  // 理论上不会进入

            const auto& entry = pool[chosen];
            m_options.append({
                entry.name,
                entry.descTpl,
                entry.upgradeId,
                entry.value
            });

            // 从索引列表中移除（保证不重复）
            indices.removeAt(chosenLocalIdx);
        }
    }
};

// ============================================================================
// Chest — 宝箱实体
// ============================================================================
// 行为：
//   1. 从天而降，缓慢下落（speed 略低于敌人）
//   2. 有血量，被子弹击中扣血
//   3. 死亡时弹出 UpgradeDialog 供玩家选择升级
//   4. 外观为金色宝箱，易于与敌人区分
//
// 数值设计（参考敌人血量 = 1 + wave/3）：
//   wave=1: 宝箱血量 3（需 3 发子弹）
//   wave=3: 宝箱血量 4
//   wave=5: 宝箱血量 5
//   公式：chestHP = 2 + wave/2，保证比同波敌人耐打
//
// 与 MainWindow 的交互约定：
//   调用方在碰撞检测中发现宝箱死亡 → 暂停游戏计时器 →
//   调用 chest.showUpgradeWindow(weapon, wave, parent) →
//   窗口关闭后恢复计时器。这样设计是为了避免宝箱直接持有
//   游戏循环引用，保持 Chest 类的独立性。
// ============================================================================
class Chest : public GameObject {
protected:
    int m_health;       // 当前血量
    int m_wave;         // 宝箱出现的波次（影响升级选项稀有度）

public:
    Chest() noexcept {
        m_health = 3;
        m_wave   = 1;
        speed    = 80.0;                                        // 下落速度略慢于敌人
        hitbox   = QRectF(0, 0, 52.0, 48.0);                   // 宝箱碰撞箱
    }

    // 根据波次初始化血量
    // 公式：2 + wave/2，使得宝箱始终比同波敌人耐打
    void initByWave(int wave) {
        m_wave   = wave;
        m_health = 2 + wave / 2;
    }

    // ---- 属性访问 ----
    void setHealth(int h)    { m_health = h; }
    int  getHealth()  const  { return m_health; }
    int  getWave()    const  { return m_wave; }
    void setWave(int w)      { m_wave = w; }

    bool isDead() const { return m_health <= 0; }

    // ---- 受伤 ----
    // 返回值：true 表示宝箱刚刚死亡（可用于触发开箱逻辑）
    void takeDamage(int dmg) {
        m_health -= dmg;
        if (m_health < 0) m_health = 0;
    }

    // ---- 下落移动 ----
    void moveDown(qreal deltaTime) override {
        GameObject::moveDown(deltaTime);
        hitbox.moveTo(position);
    }

    // ---- 宝箱生成 ----
    void spawnChest(qreal x, qreal y, int wave) {
        position = QPointF(x, y);
        hitbox.moveTo(position);
        m_wave = wave;
        m_health = 2 + wave / 2;
    }

    // ========================================================================
    // 弹出升级选择窗口（模态阻塞）
    //
    // 参数：
    //   weapon — 玩家的武器指针，用于应用升级
    //   wave   — 当前波次，影响选项稀有度
    //   parent — 父窗口（通常是 MainWindow）
    //
    // 返回值：true 表示玩家选择了升级，false 表示异常（weapon 为空等）
    //
    // 调用示例（MainWindow 中）：
    //   if (chest.isDead()) {
    //       m_gameTimer->stop();                           // 暂停游戏
    //       chest.showUpgradeWindow(&playerWeapon, m_currentWave, this);
    //       m_gameTimer->start(16);                        // 恢复游戏
    //   }
    // ========================================================================
    bool showUpgradeWindow(Weapon* weapon, int wave, QWidget* parent = nullptr) {
        if (!weapon) return false;

        UpgradeDialog dialog(weapon, wave, parent);
        int result = dialog.exec();   // 模态阻塞，等待玩家选择

        return (result == QDialog::Accepted);
    }

    // ========================================================================
    // 绘制宝箱
    //
    // 外观设计（40×36 金色宝箱）：
    //   - 主体：圆角矩形，深金色
    //   - 盖板：浅金色，略向上偏移（模拟开盖感）
    //   - 锁扣：中心小圆，亮金色高光
    //   - HP 条：底部绿色→红色渐变条
    // ========================================================================
    void draw(QPainter& painter) {
        if (m_health <= 0) return;

        painter.setRenderHint(QPainter::Antialiasing);

        QRectF box = hitbox;

        // ---- 箱体 ----
        painter.setPen(QPen(QColor(120, 80, 20), 2));
        painter.setBrush(QColor(180, 130, 50));               // 深金色
        painter.drawRoundedRect(box.adjusted(2, 6, -2, 0), 6.0, 6.0);

        // ---- 箱盖 ----
        painter.setBrush(QColor(220, 170, 60));               // 亮金色
        painter.drawRoundedRect(box.adjusted(0, 0, 0, -box.height() * 0.55), 6.0, 6.0);

        // ---- 锁扣 ----
        QPointF center = box.center();
        painter.setPen(QPen(QColor(255, 215, 0), 2));         // 金色描边
        painter.setBrush(QColor(255, 240, 100));              // 亮金色填充
        painter.drawEllipse(center, 6, 5);

        // ---- 锁孔 ----
        painter.setPen(QPen(QColor(80, 50, 10), 1.5));
        painter.drawLine(QPointF(center.x(), center.y() - 2),
                         QPointF(center.x(), center.y() + 3));

        // ---- 装饰横带 ----
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(200, 150, 40));
        painter.drawRect(QRectF(box.left() + 4, center.y() - 1,
                                 box.width() - 8, 3));

        // ---- HP 条（底部） ----
        qreal barW = box.width() - 8;
        qreal barH = 5;
        qreal barX = box.left() + 4;
        qreal barY = box.bottom() - barH - 3;

        int maxHp = 2 + m_wave / 2;
        qreal ratio = (qreal)m_health / maxHp;
        if (ratio < 0.0) ratio = 0.0;
        if (ratio > 1.0) ratio = 1.0;

        // 背景（灰色）
        painter.setBrush(QColor(60, 60, 60));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(QRectF(barX, barY, barW, barH), 2.0, 2.0);

        // 前景（绿→黄→红渐变）
        QColor barColor;
        if (ratio > 0.6)
            barColor = QColor(80, 200, 80);    // 绿色
        else if (ratio > 0.3)
            barColor = QColor(220, 180, 40);   // 黄色
        else
            barColor = QColor(220, 70, 60);    // 红色

        painter.setBrush(barColor);
        painter.drawRoundedRect(QRectF(barX, barY, barW * ratio, barH), 2.0, 2.0);
    }

    ~Chest() noexcept override = default;
};
