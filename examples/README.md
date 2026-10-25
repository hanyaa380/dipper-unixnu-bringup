# EFI examples

这些程序用于验证 Mu-Silicium 到 FAT/ESP 的加载链，不包含 Apple 代码。

## HelloEfi

输出固定文本，并等待 15 秒。把它编译为 `BOOTAA64.EFI`，放入：

```text
\EFI\BOOT\BOOTAA64.EFI
```

如果此程序能运行，说明 UEFI 已成功执行外部 PE32+ AArch64 EFI 应用。

## EfiProbe

枚举 `EFI_SIMPLE_FILE_SYSTEM_PROTOCOL` 句柄，并检查：

```text
\EFI\BOOT\BOOTAA64.EFI
\EFI\UniXNU\unixnu.toml
\EFI\UniXNU\kernel.macho
\EFI\UniXNU\kernel.macho.adt
```

它只读取路径，不执行、不修改、不删除文件。

## 构建

使用 Mu-Silicium 或 EDK2 的 AArch64 工具链构建 UEFI Application。
具体包路径和输出目录取决于 Mu-Silicium 当前 EDK2 布局。

需要确认：

- 目标架构为 AArch64；
- 输出是 PE32+ EFI Application；
- 文件名最终为 `BOOTAA64.EFI` 或 `EfiProbe.efi`；
- 固件包含必要的 FAT 和 Simple File System 驱动。

建议让接手 AI 先检查 Mu-Silicium 的 `.dsc`、`.fdf` 和
`Include/APRIORI.inc`，再给出准确构建命令，不要直接复制其他平台的路径。
