# Design：修复 Qt 5.12 适配 review 发现的问题

## Context

`support-qt-5-12` 已使项目在 Qt 5.12.12 下编译通过（含 QWK）。实施与 review 暴露 3 个后续问题：

1. **lrelease 构建顺序**（review 明确）——Example/CMakeLists.txt:56-62 用 `add_custom_command(OUTPUT .qm)` + `set_source_files_properties(OBJECT_DEPENDS)`。实测 MSBuild 下 rcc 先于 lrelease 运行，`i18n_embed.qrc` 引用的 `.qm` 缺失 → 构建失败。已通过手动 lrelease 绕过，但干净 clone 必现。
2. **README 门槛不一致**——代码 QWK 门槛已降 5.12，README 仍写 "Qt ≥ 5.15.2 时启用"、"Qt 5.14.2 可跳过子模块"（README.md:64/75-76/86-89/105）。
3. **子模块初始化步骤**——qwindowkit 嵌套 syscmdline submodule（qmsetup/.gitmodules），README 只提 qwindowkit 一层，未明确 `--recursive` 的必要性。

## Goals / Non-Goals

**Goals:**

- 干净 clone 下 Qt 5.12 构建 Example 一次成功。
- README 文档与代码门槛一致，5.12 用户知道如何启用 QWK。
- 修复独立、可回退，不触碰核心样式代码。

**Non-Goals:**

- 不改 `support-qt-5-12` 已完成的代码守卫（它们已正确）。
- 不把 `.qm` 提交入库（保持 .gitignore 现状）。
- 不修改 qwindowkit/syscmdline 第三方内容。

## Decisions

### D1. lrelease 改用 custom target 保证顺序

Example/CMakeLists.txt WIN32 分支改为：

```cmake
if(EXAMPLE_LRELEASE)
    add_custom_target(example-lrelease ALL
        COMMAND "${EXAMPLE_LRELEASE}" "${_ex_ts}" -qm "${_ex_qm}"
        DEPENDS "${_ex_ts}"
        COMMENT "lrelease: Example_en_US.ts -> Example_en_US.qm"
        VERBATIM)
    add_dependencies(Example example-lrelease)
else()
    ... 原 FATAL_ERROR 保留 ...
endif()
```

- **理由**：`add_custom_target(ALL)` 使 lrelease 成为构建图显式节点，`add_dependencies(Example ...)` 保证 Example 链接/编译前 .qm 已生成；不依赖 MSBuild 对 `OBJECT_DEPENDS` 跨目录输出的跟踪。
- **取舍**：`ALL` 意味着即使不编译 Example（仅库/插件）也会跑 lrelease——开销极小（单 .ts 文件），换取顺序可靠性，可接受。
- **备选**：`.qm` 入库（改动 .gitignore + 提交二进制）——被拒，二进制产物入库不符合项目现状。
- **备选**：文档记录手动 lrelease——被拒，无法保证干净 clone 首次构建成功，正是 review 指出的缺陷。

### D2. README QWindowKit 段落统一门槛

将 README.md:64/86-89/105 与 README_EN.md 对应段的 "Qt ≥ 5.15.2" 改为 "Qt ≥ 5.12"，"Qt 5.14.2 可跳过子模块" 改为 "Qt 5.12 起启用 QWK，均需初始化子模块（含嵌套 syscmdline）"；兼容性表格 Qt5.14.2 行的窗口边框列改为 "QWK 无边框"（与代码门槛一致）。

- **理由**：文档是用户第一信息来源，门槛不一致会直接误导（5.12 用户以为不支持、5.14 用户以为可跳过子模块）。
- **注意**：`support-qt-5-12` 的 task 8.1/8.2 也涉及 README，两变更对 README 的编辑需协调——本变更聚焦 QWK 门槛段落，task 8 聚焦版本表格行。实施时先合并本变更，再跑 task 8，避免冲突。

### D3. 子模块说明加 `--recursive`

README 的子模块段落明确：`git submodule update --init --recursive`（qwindowkit 内含 qmsetup → syscmdline 嵌套 submodule，必须 `--recursive` 才能拉全）。

- **理由**：实施中实测 qmsetup/src/syscmdline 为空目录导致 CMake 配置失败，`--recursive` 是关键。
- **验证**：文档描述与 `git submodule status` 实测行为一致。

## Risks / Trade-offs

- [custom target ALL 在 Qt6 也生效，行为变化] → 无害：Qt6 下 lrelease 同样需要，顺序保证对 Qt6 也是改进。
- [README 与 task 8 的编辑冲突] → 本变更先合入，task 8 基于新文档继续（commit 顺序解决）。
- [无 lrelease 环境] → 保留 FATAL_ERROR 降级路径，spec 已有明确错误场景。

## Migration Plan

1. 本变更 3 个修复独立可回退，按常规 commit 合入。
2. 验证：删除 .qm → 干净构建 Example（Release）→ 通过；README 门槛文字 grep 一致。
3. 与 `support-qt-5-12` 的 task 8（README 表格）顺序执行：本变更先行。

## Open Questions

- 无（3 个修复方案均明确；QWK 5.12 交互验证结果由 `support-qt-5-12` task 3.3 门禁决定，本变更的 README 记录依据其结果）。
