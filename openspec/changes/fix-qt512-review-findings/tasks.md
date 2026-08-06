# Tasks：修复 Qt 5.12 适配 review 发现的问题

## 1. lrelease 构建顺序修复

- [x] 1.1 修改 Example/CMakeLists.txt WIN32 分支：将 `add_custom_command(OUTPUT .qm)` + `set_source_files_properties(OBJECT_DEPENDS)` 改为 `add_custom_target(example-lrelease ALL)` + `add_dependencies(Example example-lrelease)`（参考 design D1），保留无 lrelease 时的 FATAL_ERROR 降级
- [x] 1.2 验证干净构建：删除 `Example/translations/Example_en_US.qm` 后执行 `cmake --build build-qt512-msvc2017 --config Release --target Example`，构建成功且 .qm 自动生成
- [x] 1.3 验证幂等：再次构建（.qm 已存在）不报错

## 2. README QWindowKit 门槛统一

- [x] 2.1 README.md 与 README_EN.md：将 QWindowKit 说明段（README.md:64/86-89/105 及英文对应段）的 "Qt ≥ 5.15.2 时启用" 改为 "Qt ≥ 5.12 时启用"，"Qt 5.14.2 可跳过子模块" 改为 "Qt 5.12 起启用 QWK 均需初始化子模块"
- [x] 2.2 兼容性表格：Qt5.14.2 行的窗口边框列改为 "QWK 无边框"（与代码门槛一致）；若 `support-qt-5-12` task 3.3 门禁已回退，Qt5.12 行按实际状态标注
- [x] 2.3 子模块段落明确 `git submodule update --init --recursive`（含嵌套 syscmdline 说明，参考 design D3）

## 3. 验证与收尾

- [x] 3.1 在 Qt 5.12.12 下完整重新配置 + 构建（Release），确认改动无回归
- [x] 3.2 grep 验证 README 不再出现 "Qt ≥ 5.15.2 时启用" / "5.14.2 可跳过子模块" 字样
- [x] 3.3 提交：按仓库提交规范提交本变更（与 `support-qt-5-12` 的 README 编辑协调，本变更先行）
