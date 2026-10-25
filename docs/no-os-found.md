# No Operating System Found

## 含义

完整提示：

```text
No Operating System Found! Please make sure that an OS is Installed.
```

它来自 Mu-Silicium 的 `PlatformBootManagerUnableToBoot()`。BDS 在尝试完
启动项后进入该回调，绘制无系统图标；如果图标资源加载失败，还会打印上述文本。

这不是 XNU panic，也不代表 UFS、APFS 或 iOS 根卷损坏。此时内核尚未被加载。

## 立即停止做的事情

- 不要继续修改 XNU 的 ArmCpu、GIC 或 UFS 驱动。
- 不要重新创建 iOS 根卷。
- 不要根据这条消息判断 APFS 或 launchd 有问题。
- 不要在没有确认路径时格式化分区。

## 分支判断

### 分支 A：手动 `Return to firmware` 后出现

这是当前固件没有其他可启动项时的正常回退。重点检查 EFI 启动项、
FAT/ESP 和标准回退路径。

### 分支 B：倒计时自动启动后出现

默认启动项不可用，或默认项就是固件菜单。检查：

- 默认启动项的设备路径；
- `\EFI\BOOT\BOOTAA64.EFI` 是否存在；
- 文件是否为 AArch64 PE32+；
- FAT 驱动是否加载；
- 分区是否被枚举。

### 分支 C：曾经能启动，修改分区后消失

优先检查 GPT 分区类型、ESP 标志、文件系统类型和卷 GUID。
修改分区后重新执行 `map -r`。

## UEFI Shell 检查

```text
map -r
fs0:
ls
ls \EFI\BOOT
bcfg boot dump
```

如果 `fs0:` 不是目标文件系统，继续尝试 `fs1:`、`fs2:`。

手动执行：

```text
\EFI\BOOT\BOOTAA64.EFI
```

## 结果判读

| 现象 | 含义 | 下一步 |
| --- | --- | --- |
| 没有任何 FAT 卷 | 块设备或 FAT 驱动缺失 | 检查分区、GPT、Simple File System、FAT 驱动 |
| 有 FAT 卷但看不到启动文件 | 路径或镜像写入错误 | 检查目录、大小写和实际文件路径 |
| 文件存在但不能执行 | 非 AArch64 PE32+ 或文件损坏 | 检查 `file` 输出和 SHA-256 |
| 手动执行成功但 BDS 不启动 | 启动项或 BDS 枚举问题 | 检查 `bcfg boot dump` 和默认项 |
| BDS 自动执行成功 | G1-G3 通过 | 开始自定义 loader |

## 建立启动项

在确认 UEFI Shell 支持 `bcfg` 后，可使用：

```text
bcfg boot add 0 FS0:\EFI\BOOT\BOOTAA64.EFI "Dipper Test"
bcfg boot dump
```

不要照抄 `FS0`；必须先通过 `map -r` 确认目标卷。
