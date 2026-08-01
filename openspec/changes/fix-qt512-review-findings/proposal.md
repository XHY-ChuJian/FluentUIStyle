# 修复 Qt 5.12 适配 review 发现的问题

## Why

`support-qt-5-12` 变更实施中发现 3 个未被初始计划覆盖的问题，其中 1 个是 review 明确指出（lrelease 构建顺序），2 个是实施中暴露的连带问题（submodule 文档、README 门槛一致性）。这些问题不影响 5.12 编译本身，但会导致**干净 clone 无法构建**或**文档误导用户**，必须在提交前修复。

## What Changes

- 修复 Example/CMakeLists.txt 的 lrelease 构建顺序：`OBJECT_DEPENDS` 在 MSBuild 下不触发（首次构建 rcc 先于 lrelease 运行，`i18n_embed.qrc` 引用的 `.qm` 缺失导致 Example 构建失败）。改用 `add_custom_target(example-lrelease)` + `add_dependencies(Example example-lrelease)` 保证顺序，`.qm` 保持不入库。
- 更新 README 的 QWindowKit 文档：代码门槛已降 5.12，但 README 仍写 "Qt ≥ 5.15.2 时启用"、"Qt 5.14.2 可跳过子模块"——需改为 "Qt ≥ 5.12 时启用"，且明确 `git clone` 后须 `git submodule update --init --recursive`（含嵌套 syscmdline，5.12 起所有启用 QWK 的版本都需要）。
- （可选）在 README 记录 Qt 5.12 下 QWK 的验证状态与已知限制（若 3.3 门禁回退则记录"需手动开启"）。

## Capabilities

### New Capabilities

- `example-build-order`: Example 的翻译 .qm 生成与编译顺序可靠，首次构建（干净 clone）不因 rcc 先于 lrelease 失败。

### Modified Capabilities

<!-- 无既有 spec；support-qt-5-12 的 qwindowkit-qt5-12 已在原变更内 -->

## Impact

- **构建系统**：Example/CMakeLists.txt（lrelease 顺序，仅 WIN32 分支）。
- **文档**：README.md / README_EN.md 的 QWindowKit 版本描述与子模块说明（5.12 起启用、须 `--init --recursive`）。
- **不触及**：核心代码（守卫/清理已在前一变更完成）、qwindowkit 子模块。
