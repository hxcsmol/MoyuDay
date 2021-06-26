# MoyuDay

## 项目来源
这是源于煎蛋最近几天流行摸鱼拼图日历。
最近刚好在实验一些老技术，主要在寻找Qt之外的跨平台UI或图形框架，一些冷门的甚至Godot这样的游戏引擎都有尝试。
这天正好翻到SDL2，发现它依赖小，调用也简单，就顺手用SDL2写个玩玩。
因为平时开发以嵌入式为主，加上程序简单，所以懒得写类了，直接写了一个C文件搞定。

## 平台
目前试验了在VC下编译没问题，其它平台应该问题不大，自行安装SDL2库，做好头文件与库路径设置即可

## 依赖
依赖SDL2库，请自行安装。

## 发布
不想费劲编译，可直接下载[Release](https://github.com/hxcsmol/MoyuDay/releases/tag/Ver1.0.0)版本

## 操作
鼠标左键拖动即可，右键旋转，中键翻转

## 计划
因为仅仅是实验，所以花了半天实验了这个小游戏。下一步（如果有空的话）打算加个简单自动算法吧。毕竟摸鱼嘛，也要自动摸要爽些。
