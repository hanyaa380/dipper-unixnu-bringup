# Xiaomi Mi 8 / dipper UniXNU bring-up
(文章由deepseek v4.1 flash编写)
这是一个面向 Xiaomi Mi 8（`dipper`、SDM845）同类设备协作的公开启动链诊断仓库。

仓库只保存可公开的源码、补丁、测试程序、构建说明和日志模板。它不是可直接刷写的
发行镜像，也不包含 Apple kernelcache、`kernel.macho`、ADT、iOS 根卷或用户数据。

## 当前问题

设备可以进入 Mu-Silicium UEFI，但在 BDS 阶段最终显示：

```text
No Operating System Found! Please make sure that an OS is Installed.
```

该字符串来自 Mu-Silicium：

```text
Silicon/Silicium/SiliciumPkg/Library/BootManagerLib/BootManager.c
PlatformBootManagerUnableToBoot()
```

这说明 UEFI 已经运行，但没有成功执行任何 EFI/Darwin 启动项。问题发生在
DarwinBoot 和 XNU 之前，因此当前不应继续排查 UFS、APFS、launchd 或用户态。

## 最新移植进度

本项目另有一条已经越过 UEFI 并进入 XNU IOKit 的调试线。当前真机证据显示：

```text
ArmCpu prepareGic returned
ArmCpu IOCPU::start returned
ArmCpu processor register returned
ArmCpu IRQ controller returned
ArmCpu initCPU enter
```

GIC 已经映射并完成 `prepareGic()`，但还没有证据证明 `enableGic()` 完成。最新探针
正在拆分 `initCPU()`、`enableGic()`、SGI probe、timer 和 `setCPUState()`：

- 设备调试提交：`cf6770b`
- 新构建：<https://github.com/hanyaa380/UniXNU-build/actions/runs/40170103530>

完整的已验证结果、镜像哈希、日志和下一步见 [PROGRESS.md](PROGRESS.md)。

## 第一个里程碑

先完成下面的链路：

```text
Mu-Silicium BDS
  -> 枚举 GPT/块设备
  -> 枚举 FAT/ESP
  -> 找到 \EFI\BOOT\BOOTAA64.EFI
  -> 手动执行 Hello EFI
  -> BDS 自动执行 Hello EFI
```

只有这个链路稳定后，才进入自定义 boot manager、DarwinBoot 和 XNU。

## 仓库内容

| 路径 | 用途 |
| --- | --- |
| `PROGRESS.md` | 真机进度、镜像哈希和当前卡点 |
| `docs/architecture.md` | 启动链和验收门 |
| `docs/no-os-found.md` | `No Operating System Found` 的分层排查 |
| `docs/uefi-smoke-test.md` | UEFI Shell、FAT/ESP 和 Hello EFI 测试 |
| `examples/HelloEfi` | 最小 AArch64 EFI 应用源码 |
| `examples/EfiProbe` | 枚举 FAT 卷并检查启动文件的 EFI 诊断应用 |
| `HANDOFF_PROMPT.md` | 可直接交给 GPT/其他 AI 的接手提示词 |
| `templates/uefi-report.md` | 真机结果回报模板 |

## 安全要求

- 优先使用 `fastboot boot` 临时启动测试镜像。
- 不把未经确认的 UEFI FD 直接写入设备。
- 不运行来源不明的“一键分区”或“一键刷写”脚本。
- 分区、格式化和刷写前必须确认具体型号、分区表和块设备路径。
- 本仓库不发布 Apple 二进制内容；协作者应使用自己的合法 IPSW 提取材料。

## 相关源码

- Mu-Silicium: <https://github.com/Project-Silicium/Mu-Silicium>
- XNU: <https://github.com/apple-oss-distributions/xnu>
- UniXNU 指南: <https://unixnu.com/guide/>
- 本项目的设备调试分支: <https://github.com/hanyaa380/UniXNU-dipper>


