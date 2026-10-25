# 启动链与验收门

## 完整目标链

```text
Android Bootloader
  -> BootShim
  -> Mu-Silicium UEFI
  -> FAT/ESP
  -> EFI boot manager
  -> Darwin loader
  -> kernel.macho + ADT + boot_args
  -> ExitBootServices
  -> XNU
  -> IOKit platform expert
  -> GICv3 + timer + CPU
  -> UFS + GPT
  -> APFS root
  -> launchd
  -> Apple user space
```

## 验收门

任何后续门失败时，不得跳过前一个门。

| 门 | 目标 | 通过证据 |
| --- | --- | --- |
| G0 | Mu-Silicium 稳定启动 | UEFI 画面和输入稳定，能重复启动 |
| G1 | 枚举 FAT/ESP | UEFI Shell 的 `map -r` 能看到 FAT 卷 |
| G2 | 执行 EFI 应用 | 手动执行 `\EFI\BOOT\BOOTAA64.EFI` 成功 |
| G3 | BDS 自动启动 | 重启后无需手动 Shell 也能执行 EFI 应用 |
| G4 | 自定义 boot manager | 能读取配置并校验 kernel/ADT |
| G5 | DarwinBoot | 能构造 boot_args 并跳转 XNU 入口 |
| G6 | XNU 早期控制台 | XNU 日志稳定，不立即复位 |
| G7 | IOKit 平台专家 | ArmCpu、GICv3、timer 有明确进度 |
| G8 | 存储 | UFS 请求数增加，GPT 和块设备发布 |
| G9 | 根卷 | APFS 容器和根卷 UUID 可识别 |
| G10 | 用户态 | launchd 启动 |

当前 `No Operating System Found` 属于 G1/G2/G3，不属于 G6 以后。

## 设计边界

Mu-Silicium 和 XNU 使用不同的执行环境：

- UEFI 负责预启动、块设备、FAT、GOP、输入和 EFI 应用加载。
- UEFI 驱动不能替代 XNU 的 IOKit 驱动。
- XNU 运行后需要自己的 ArmCpu、GICv3、timer、UFS、存储和显示适配。
- Apple kernel、ADT、trust cache 和根卷应由各协作者从自己的合法 IPSW 提取。

## 禁止的捷径

- 不建议把不同设备、不同分区布局的完整 boot.img 直接混用。
- 不建议在 UEFI 尚未枚举 FAT 时修改 XNU。
- 不建议用“GIC 也不对、UFS 也不对、显示也不对”推断它们共享同一根因。
- 不建议把“UEFI 菜单显示成功”等同于 DarwinBoot 或 XNU 启动成功。
