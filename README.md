# TheLevelTwo

一个基于 Qt / C++ 实现的竖版射击小游戏。

## 运行方式

使用 Qt Creator 打开项目根目录下的 `CMakeLists.txt`，选择 Qt 6 MinGW Kit 后构建运行。

## 操作说明

- A / D 或 左右方向键：移动玩家
- Space：射击
- ESC：暂停游戏
- Enter / Space：菜单确认
- 鼠标：点击菜单按钮
- S：主菜单进入设置
- Q：退出游戏

## 已实现功能

### A 模块：游戏主框架与基础战斗

- `MainWindow` 主窗口
- `QTimer` 驱动游戏循环
- `QPainter` 绘制玩家、敌人、子弹、道具、Boss、UI
- 玩家移动与射击
- 子弹、敌人、玩家碰撞检测
- 分数、生命值、Game Over 流程

### B 模块：道具、武器升级与视觉反馈

- 宝箱掉落与拾取
- 武器升级系统
- 多发子弹、射速、穿透等升级效果
- 道具拾取
- 粒子特效
- 浮动文字反馈
- 简单音效反馈接口

### C 模块：高级内容

- 主菜单、暂停菜单、设置菜单、Game Over 菜单
- 游戏状态管理：Menu / Settings / Playing / Paused / GameOver
- Boss 敌人与 Boss 战
- Boss 扇形弹幕攻击
- 关卡递进系统
- 本地 JSON 存档
- 最高分保存
- 音量和全屏设置持久化

## 项目结构

```text
Codes/
  main.cpp
  mainwindow.h / mainwindow.cpp / mainwindow.ui
  GameObject.h
  Player.h
  Enemy.h
  Bullet.h
  EnemyBullet.h
  BossEnemy.h
  PowerUp.h
  Chest.h
  Weapon.h
  ParticleEffect.h
  FloatingText.h
  SaveManager.h
