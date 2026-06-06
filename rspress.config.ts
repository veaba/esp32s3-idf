import { defineConfig } from "rspress/config";
import { pluginShiki } from "@rspress/plugin-shiki";

export default defineConfig({
  root: "docs",
  title: "RuView",
  description: "RuView — WiFi 感知平台文档",
  logo: "logo.svg",
  themeConfig: {
    socialLinks: [
      {
        icon: "github",
        mode: "link",
        content: "https://github.com/veaba/moto",
      },
    ],
    nav: [
      {
        text: "文档",
        link: "/ruview/",
        activeMatch: "^/ruview",
      },
      {
        text: "GitHub",
        link: "https://github.com/veaba/moto",
      },
    ],
    sidebar: {
      "/ruview/": [
        {
          text: "概览",
          link: "/ruview/",
        },
        {
          text: "项目架构",
          link: "/ruview/arch",
        },
        {
          text: "ESP32-S3 使用指南",
          link: "/ruview/guide-esp32-s3",
        },
        {
          text: "教程",
          collapsed: false,
          items: [
            {
              text: "01 - 快速入门",
              link: "/ruview/guide/01-quick-start",
            },
            {
              text: "02 - 安装与构建",
              link: "/ruview/guide/02-installation",
            },
            {
              text: "03 - 数据源配置",
              link: "/ruview/guide/03-data-sources",
            },
            {
              text: "04 - REST API 参考",
              link: "/ruview/guide/04-rest-api",
            },
            {
              text: "05 - WebSocket 实时流",
              link: "/ruview/guide/05-websocket",
            },
            {
              text: "06 - ESP32 硬件设置",
              link: "/ruview/guide/06-hardware-setup",
            },
            {
              text: "07 - 模型训练",
              link: "/ruview/guide/07-model-training",
            },
            {
              text: "08 - 自适应分类器",
              link: "/ruview/guide/08-adaptive-classifier",
            },
            {
              text: "09 - 生命体征检测",
              link: "/ruview/guide/09-vital-signs",
            },
            {
              text: "10 - 边缘智能模块",
              link: "/ruview/guide/10-edge-intelligence",
            },
            {
              text: "11 - Cognitum Seed 集成",
              link: "/ruview/guide/11-cognitum-seed",
            },
            {
              text: "12 - 3D 点云",
              link: "/ruview/guide/12-pointcloud",
            },
            {
              text: "13 - 故障排除",
              link: "/ruview/guide/13-troubleshooting",
            },
            {
              text: "14 - 常见问题解答",
              link: "/ruview/guide/14-faq",
            },
          ],
        },
      ],
    },
  },
  plugins: [pluginShiki()],
});
