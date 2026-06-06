---
pageType: home
title: RuView
titleSuffix: 'RuView — WiFi 感知平台'

hero:
  name: RuView
  text: |
    RuView WiFi 感知平台
    从无线电波到人体感知
  tagline: WiFi CSI 人体检测 · 呼吸心率监测 · 姿态估计 · ESP32-S3 边缘智能
  actions:
    - theme: brand
      text: 快速开始
      link: /ruview/guide/01-quick-start
    - theme: alt
      text: 项目架构
      link: /ruview/arch
    - theme: alt
      text: ESP32-S3 指南
      link: /ruview/guide-esp32-s3

features:
  - title: WiFi CSI 人体感知
    details: 利用 ESP32-S3 采集 WiFi CSI 信号，实现穿墙人体检测、呼吸率与心率监测、运动追踪
    link: /ruview/guide/09-vital-signs
  - title: 多节点 Mesh 组网
    details: 3-6 个 ESP32-S3 节点组成 mesh，20 Hz 完整 CSI 采集，总成本约 $54
    link: /ruview/guide/06-hardware-setup
  - title: 边缘智能模块
    details: 65 个 WASM 边缘智能模块，运行在 ESP32 传感器上，无需互联网、即时响应
    link: /ruview/guide/10-edge-intelligence
  - title: 自适应分类器
    details: 从标记录制数据学习环境特定 WiFi 信号模式，替代静态阈值分类，支持在线推理
    link: /ruview/guide/08-adaptive-classifier
  - title: 3D 点云融合
    details: 融合摄像头深度估计与 WiFi CSI 空间感知，生成实时 3D 点云
    link: /ruview/guide/12-pointcloud
  - title: Cognitum Seed
    details: Pi Zero 2 W 附件，为 ESP32-S3 添加持久向量存储、kNN 搜索、加密见证链
    link: /ruview/guide/11-cognitum-seed
---