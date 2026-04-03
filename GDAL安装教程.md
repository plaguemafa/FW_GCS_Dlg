# GDAL 开发环境安装教程（本仓库 / Windows + MSVC）

本仓库通过 **vcpkg 的清单模式（manifest）** 引入 `gdal`，用于 `DemReader.cpp` 读取 DEM（`#include <gdal.h>`）。  
在另一台电脑上，**不要**先单独折腾「系统级 GDAL」，除非你清楚如何与 MSVC / MFC 工程对齐；按下面流程走，可与当前工程保持一致。

---

## 1. 为什么 GDAL 经常装到怀疑人生

- **依赖极多**：PROJ、GEOS、TIFF、PNG、JSON、SQLite……vcpkg 会拉一整棵树；首次常是**源码编译**，不是秒下好的单个安装包。
- **时间很长**：机械硬盘或弱 CPU 上，**数小时到「隔夜」**都正常；你在这台机子上「两天两夜」级别的经历，多半是反复失败重试 + 全量重编译叠出来的。
- **磁盘占用大**：`vcpkg` 的 `buildtrees`、`packages`、`downloads` 加上已安装文件，**预留 30～80 GB** 更稳妥（视是否清理中间目录而定）。
- **网络与环境**：公司代理、杀软锁文件、路径过长、磁盘满，都会让 CMake/Ninja 报错，看起来像「GDAL 装坏了」。

理解：**一次成功的大头时间是编译，不是下载一个 exe。**

---

## 2. 本仓库推荐路径（与 `vcpkg.json` 一致）

工程目录：`FW_GCS_Dlg/vcpkg.json` 中已声明依赖 `gdal`。  
`FW_GCS_Dlg.vcxproj` 里已开启：

- `VcpkgEnableManifest` = true  
- x64 使用三元组 **`x64-windows`**

因此新电脑上的标准做法是：

1. 安装 **Visual Studio**（带 **使用 C++ 的桌面开发**、**MFC**、**Windows 10/11 SDK**）。
2. 安装并配置 **vcpkg**。
3. 用 **x64** 配置**第一次完整生成解决方案** —— VS 会自动按清单安装并编译 `gdal` 及其依赖。

> **平台说明**：README 写的主线是 **x64 + MSVC（v142）**。请在新电脑上以 **x64** 为主进行生成；Win32 会走 `x86-windows` 三元组，依赖也要单独装一套，一般不推荐 unless 你真要 32 位。

---

## 3. 前置检查清单（装 gdal 之前）

| 项目 | 说明 |
|------|------|
| Visual Studio | 2019 / 2022 均可；工作负载：**桌面 C++**，组件勾选 **MFC**、**Windows SDK**。 |
| Git | 克隆 vcpkg 必需。 |
| 磁盘空间 | 建议 **≥ 50 GB** 空闲（vcpkg + 中间文件）。 |
| 网络 | 能稳定访问 GitHub / 上游源码站；需代理时先配好 Git/系统代理。 |
| 权限 | 不要用需要管理员才能写的奇怪路径；路径尽量短，**避免中文空格路径**（减少历史坑）。 |

---

## 4. 安装 vcpkg

在 **PowerShell** 或 **x64 Native Tools Command Prompt** 中执行（路径请按你的习惯改，**全英文路径**最省心）：

```powershell
cd C:\dev
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
```

### 4.1 让 Visual Studio 能找到 vcpkg（二选一即可）

**方式 A（推荐）：环境变量**

- 新建系统或用户环境变量 **`VCPKG_ROOT`** = `C:\dev\vcpkg`（你的实际路径）。
- 重新打开 Visual Studio。

**方式 B：用户级集成**

```powershell
cd C:\dev\vcpkg
.\vcpkg integrate install
```

说明：

- `vcpkg integrate install` 主要面向“经典模式”（手动 `vcpkg install xxx` 的那种）。本仓库是 **manifest 模式**（`FW_GCS_Dlg/vcpkg.json`），是否能自动安装，**关键取决于 VS/MSBuild 能否定位到 vcpkg 根目录**（最稳妥就是设置 `VCPKG_ROOT` 并重启 VS）。
- 如果你已经设置了 `VCPKG_ROOT`，`integrate install` 做不做通常都不影响 manifest 自动装包。

之后在 VS 里打开本仓库解决方案，manifest 模式会走统一的工具链。

---

## 5. 打开本工程并触发 GDAL 安装

1. 打开 `FW_GCS_Dlg.sln`。  
2. 配置选 **Debug** 或 **Release**，平台选 **x64**。  
3. 对 `FW_GCS_Dlg` 项目执行 **生成**。  

首次会在 `FW_GCS_Dlg/vcpkg_installed/x64-windows/` 下安装头文件与库；控制台输出里会看到 vcpkg 在编译大量包 —— **请耐心等待，不要反复中断**。

当前机子上曾出现 **gdal 3.12.2 + x64-windows**；新电脑上具体版本以当时 vcpkg 基线为准，**API 与本仓库用法兼容即可**。

### 5.1 为什么“进 VS 重新编译”没有自动下载（按优先级排查）

1. **没重启 VS / `VCPKG_ROOT` 没生效**  
   - 设置或修改 `VCPKG_ROOT` 后，必须 **完全退出并重开 VS**。  
   - `VCPKG_ROOT` 要指向包含 `vcpkg.exe` 的目录（例如 `C:\dev\vcpkg`），不是指向某个 `installed/` 子目录。

2. **编译的平台不对（Win32 vs x64）**  
   - 本项目在 `FW_GCS_Dlg.vcxproj` 里把 x64 对应三元组设为 `x64-windows`。  
   - 如果你在 VS 顶部选的是 **Win32**，它会尝试走 `x86-windows`（等于要再装一套），而且你可能误以为“没自动下载/没反应”。请确保选择 **x64**。

3. **你构建的不是 `FW_GCS_Dlg` 项目**  
   - 解决方案里可能还有其它测试工程；请确认是对 `FW_GCS_Dlg` 右键“生成/重新生成”，或直接“生成解决方案”且该项目未被禁用。

4. **VS 缺 vcpkg/MSBuild 相关组件（较少见）**  
   - 通过 Visual Studio Installer 确认已安装“使用 C++ 的桌面开发”，并包含对应的 MSVC 工具集与 Windows SDK。缺组件时 vcpkg 可能不会被正确调用。

### 5.2 如何确认是否真的触发了 vcpkg

- 在 VS 的“输出”(Output) 窗口（显示“生成”日志）里，正常会看到类似 **`vcpkg install`** 的字样，以及大量第三方库的配置/编译信息。  
- 若输出里完全没有 vcpkg 相关行，通常就是 **`VCPKG_ROOT`/重启** 或 **平台选择** 导致的“未触发”。  

---

## 6. 时间预期（心里有个数）

| 场景 | 大致耗时 |
|------|----------|
| 首台机器、源码全编、中端 CPU + SSD |  often **1～3 小时** |
| 机械硬盘 / CPU 较弱 / 网络反复失败 | **半天～隔夜** 都有可能 |
| 已配置 vcpkg **二进制缓存**、且命中缓存 | 可缩短到 **几十分钟** 量级 |

---

## 7. 加速与「第二台电脑」复用（强烈建议了解）

### 7.1 vcpkg 二进制缓存（Binary Cache）

同一团队多台机器重复编译同一三元组时，可配置二进制缓存（文件共享或 Azure 等），避免每台都从源码烧一遍。  
官方文档关键词：`vcpkg binary caching`。适合作为后续优化，不是第一天必须。

### 7.2 可否直接拷贝 `vcpkg_installed`？

- **同一 VS 工具集、同一 vcpkg commit、同一三元组** 下，有时能省下编译，但 **ABI/路径** 敏感，**最稳妥**仍是新机上跑一遍清单安装。  
- 若你确要尝试整目录拷贝，务必同时记录 **vcpkg 的 git 版本** 与 **VS / Windows SDK 版本**，出问题优先回到「干净重编」。

---

## 8. 运行期说明（能编过但跑起来缺 DLL）

本工程是 **MFC 桌面程序**。若运行提示找不到 `gdal.dll` / `proj_*.dll` 等：

- 先看**生成输出目录**是否已有 vcpkg 拷入的运行库（视 VS/vcpkg 集成与项目设置而定）。  
- 若没有，可从本机安装树中把所需 DLL 拷到 exe 同目录，例如：  
  `FW_GCS_Dlg\vcpkg_installed\x64-windows\x64-windows\bin`（以你盘上实际 `bin` 为准）。

代码里已设置 `CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "YES");`，**DEM 路径尽量仍避免稀奇古怪编码**，减少 Windows 下路径问题。

---

## 9. 常见问题（排错顺序）

1. **生成失败：找不到 vcpkg / manifest**  
   - 确认 **`VCPKG_ROOT`** 或已执行 **`vcpkg integrate install`**，并重启 VS。

2. **链接错误或头文件找不到**  
   - 确认平台是 **x64**，与 `x64-windows` 一致；不要用混了 Win32 配置却期望 x64 依赖。

3. **磁盘爆满 / 写入失败**  
   - 清理 `vcpkg\buildtrees\`（确认不再编译时）、或换大盘；杀软排除构建目录有时能减少随机失败。

4. **下载超时 / SSL**  
   - 配代理、`git config`、或换网络；避免在不稳定网络下开多个并发编译。

5. **只想用 gdalinfo / 预处理栅格，不一定要编进 VS**  
   - 可另装 **OSGeo4W** 或 **conda-forge** 当**独立工具链**；但与本 **MSVC MFC + vcpkg** 工程不是同一条路，不要混成「系统 GDAL 目录 + 手工填 VS 包含库」 unless 你很熟。

---

## 10. 与本仓库 `README.md` 的对应关系

- **IDE / MFC / x64**：按 README 准备 Visual Studio。  
- **GDAL**：以本文 **vcpkg 清单**为准，无需在本机再装一份「官方 GDAL 安装程序」才能完成本工程链接。  
- **WebView / NuGet、QGIS / MBTiles / SQLite**：不在本文范围，按主自述与其它文档继续即可。

---

## 11. 最小命令备忘（给老员工复制）

```text
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg && bootstrap-vcpkg.bat
# 设置环境变量 VCPKG_ROOT=C:\...\vcpkg 后重启 VS
# 打开 FW_GCS_Dlg.sln，平台 x64，生成 FW_GCS_Dlg
```

---

若新电脑仍长时间卡在某一特定依赖（例如 PROJ、TIFF），把 **vcpkg 最后 50 行报错日志**贴出来比对，往往比盲目重装 GDAL 更有效。
