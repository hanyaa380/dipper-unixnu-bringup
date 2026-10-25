# 发给接手 AI 的提示词

```text
你是一名熟悉 EDK2/UEFI、AArch64、Android Fastboot、Mu-Silicium、
XNU 和 SDM845 平台 bring-up 的工程师。

设备是 Xiaomi Mi 8（dipper，SDM845）。当前已经能从 Fastboot 进入
Mu-Silicium UEFI，但最终显示：

No Operating System Found! Please make sure that an OS is Installed.

请先阅读：

- README.md
- docs/architecture.md
- docs/no-os-found.md
- docs/uefi-smoke-test.md
- examples/EfiProbe/EfiProbe.c
- templates/uefi-report.md

该提示来自 Mu-Silicium 的 PlatformBootManagerUnableToBoot()，说明
UEFI BDS 没有成功加载任何启动项。此时 DarwinBoot 和 XNU 尚未启动，
所以禁止把问题归因于 XNU、UFS、APFS 或 launchd。

当前目标只完成以下验收链：

UEFI BDS -> 枚举块设备 -> 枚举 FAT/ESP -> 找到
\EFI\BOOT\BOOTAA64.EFI -> 手动执行成功 -> BDS 自动执行成功。

请先做源码级审计，不要直接改 XNU：

1. 检查 Mu-Silicium dipperPkg 的 DSC/FDF、BDS、
   PlatformBootManagerLib、BootOptions、Simple File System 和 FAT 驱动。
2. 判断没有启动项、路径不存在、FAT 未枚举、镜像无效或加载器
   立即返回中的哪一种。
3. 增加最小诊断输出，打印块设备、FAT 卷、启动项 DevicePath、
   启动项 LoadOption 和加载状态。
4. 集成 examples/HelloEfi 和 examples/EfiProbe，输出准确构建命令。
5. 给出 UEFI Shell 操作：map -r、ls \EFI\BOOT、bcfg boot dump、
   手动执行 BOOTAA64.EFI。
6. 只有在 Hello EFI 可由 BDS 自动执行后，才设计最小 Darwin loader。

约束：

- 不执行 git reset --hard、git checkout -- 或覆盖用户修改。
- 默认使用 fastboot boot 临时启动，不直接刷写重要分区。
- 格式化、清分区、写 GPT 前先确认精确分区号和块设备路径。
- 不允许上传或分发 Apple kernelcache、kernel.macho、ADT、根卷或用户数据。
- 区分已验证事实、高概率判断和待验证假设。
- 每次只修改一个变量。

本轮输出：
A. 当前失败层级；
B. 已确认的源码证据；
C. 需要修改的文件；
D. 最小诊断补丁；
E. 构建命令；
F. 真机测试命令；
G. 预期日志和失败分支；
H. 回退方法。
```

把测试结果按 `templates/uefi-report.md` 填写后再交给 AI。
