# /autoplan Review — support-qt-5-12

Branch: master | Commit: 5ddbdff | Date: 2026-08-01
Mode: SELECTIVE EXPANSION | Codex: [codex-unavailable] (binary missing) → Claude subagent only [subagent-only]

## Phase 1: CEO Review

### Premise Challenge (0A)

见主评审对话。P1-P5 全部成立，基于用户明确指令与实测证据。

### 0C-bis Implementation Alternatives

- **APPROACH A** 最小局部适配（现状）：Effort S / Risk Low — **RECOMMENDED**（P5 explicit, P3 pragmatic）
- **APPROACH B** 统一版本宏头文件：Effort M / Risk Med — 拒绝（60+ 处既有守卫迁移纯重构，风格冲突）
- **APPROACH C** 分叉 Qt5 分支：Effort XL / Risk High — 拒绝（同步成本噩梦）

### 0D Expansion Candidates

| # | 候选 | 决策 | 分类 |
|---|------|------|------|
| E1 | CI 多版本构建矩阵（GitHub Actions + aqtinstall） | 待 gate | **TASTE** |
| E2 | 一键本地构建脚本 | SKIP | — |
| E3 | qmake 集成 QWK | SKIP（Non-Goals） | — |
| E4 | CI 冒烟（并入 E1） | 待 gate | — |

### Claude Subagent Findings (CEO voice)

1. **[HIGH] QWK 模拟路径未验证即默认 ON** — 建议先实机验证再定默认。评估：tasks 3.1→3.2 顺序执行即隐式验证，design.md D2 有回退方案，spec 场景是验收标准非承诺。→ 仍列入 gate 作为 taste decision。
2. **[MED] 无 CI 的多版本手工矩阵 6 个月后腐化** — 与 0D E1 独立吻合。→ 强信号，列入 gate。
3. **[MED] 替代方案记录不足** — D2 备选只一句驳回。→ 小改进：补充备选记录。
4. **[LOW] 验证矩阵砍 5.14.2** — **拒绝**（P1 completeness）：5.14.2 是 README 已声明支持版本，HighDPI PassThrough 边界（5.14 引入）正好需要 5.14 验证；成本低。
5. **[MED] Qt 5.12 EOL（2021 OSS / 2024 LTS）** — 事实但用户指令即需求，尊重用户判断。
6. **[MED] 竞争视角** — Qt 5.12 支持对 ElaWidgetTools（5.15 起）是差异化优势。→ 文档论证补充。

### CEO Dual Voices Consensus

```
CEO DUAL VOICES — CONSENSUS TABLE:
═══════════════════════════════════════════════════════════════
  Dimension                           Claude  Codex    Consensus
  ──────────────────────────────────── ─────── ──────── ─────────
  1. Premises valid?                   ✅      N/A      CONFIRMED
  2. Right problem to solve?           ✅      N/A      CONFIRMED (EOL flagged)
  3. Scope calibration correct?        ✅      N/A      DISAGREE (QWK 默认 ON 顺序)
  4. Alternatives sufficiently explored?✅      N/A      DISAGREE (备选记录)
  5. Competitive/market risks covered? N/A     N/A      N/A
  6. 6-month trajectory sound?         ⚠️      N/A      CONFIRMED (CI 缺失, 双模型独立指出)
═══════════════════════════════════════════════════════════════
CONFIRMED = both agree (Claude voice + subagent). Missing voice = N/A.
```

### Sections 1-11（自动决定）

**S1 Architecture** — 无新组件。依赖变化：QWK 从 5.15.2+ 扩展到 5.12+。耦合无变化（QWK 仅在 Example/frameless 使用，fluentui3style 零 QWK 依赖）。架构图：

```
改动前                              改动后 (Qt 5.12+)
Example ──► FluentUI3Style          Example ──► FluentUI3Style
    │            │                      │            │
    ├─► ExWidgets ├─► QWK (5.15.2+)     ├─► ExWidgets ├─► QWK (5.12+)
    └─► Qt5/6 (守卫)                     └─► Qt5/6 (守卫, +1 处)
AudiomaticMini (Qt6-only) 不变        AudiomaticMini (Qt6-only) 不变
```

**S2 Error & Rescue** — 构建期失败全部显式（编译错误可见），无静默失败路径：

```
CODEPATH                | 失败模式             | 检测
------------------------|----------------------|------------------
CMake 配置 (5.12)       | 找不到 Qt5Config     | 显式报错 → 检查 CMAKE_PREFIX_PATH
QWK 编译 (5.12)         | qmsetup 不兼容点     | 显式编译错误 → tasks 3.2
qmake shadow build      | Makefile 生成失败    | 显式报错 → tasks 6.1
回归 (5.14.2/5.15.2/6.6)| 构建/运行失败        | 显式 → tasks 7.x
QWK 交互 (5.12)         | 拖动/缩放体验异常    | 手动验证 → tasks 5.4
```

**S3 Security** — 无新攻击面：构建配置 + 示例守卫 + 文档。无凭据/输入/网络面（CI 若加入用标准 GitHub token）。No issues。

**S4 Data Flow & Interaction** — 唯一运行时行为变化：HighDPI 策略在 5.12 走默认 rounding（无 PassThrough），与 5.14+ 有像素级细微差异——可接受（README 已声明版本间差异）。QWK 交互三项为显式验证项。无未处理边界。

**S5 Code Quality** — 无新增业务逻辑代码。setCodec 守卫复用 mainwindow.cpp:1412 既有模式（DRY 合规）。No issues。

**S6 Test** — 项目无单测基础设施（Qt 样式库现状）。验证矩阵即测试：4 版本 × (CMake+qmake) × (构建+运行+交互)。覆盖全部新 codepath（守卫/门槛/清理）。无 LLM 相关。

**S7 Performance** — 构建期改动，零运行时影响。No issues。

**S8 Observability** — CMake status message 已有（QWK enabled/disabled 提示）。构建失败信息充分。No issues。

**S9 Deploy** — 无部署。发布 = 合入 PR；回滚 = git revert（D1/D2 改动彼此独立可分）。验证矩阵即 post-merge 检查。

**S10 Trajectory** — 可逆性 5/5。零新增技术债（无新抽象、无新依赖）。与项目"多版本支持"定位一致。

**S11 Design & UX** — UI scope 弱（无新 UI 设计）。QWK 无边框交互（拖动/缩放/按钮）是主要关切 → tasks 5.4 覆盖。Example 冒烟路径已列。

**NOT in scope**：CI 集成（E1，待 gate）；qmake 集成 QWK；AudiomaticMini Qt5；预编译二进制发布；统一版本宏层。
**What already exists**：60+ 处 QT_VERSION 守卫；qwindowkit 5.12 兼容分支；CMake Qt5/Qt6 双分支；mainwindow.cpp setCodec 先例；.gitignore build-* 覆盖。
**Dream state delta**：本方案交付"5.12 可编译可运行 + QWK 可用 + 双路径验证"。距 12 月理想（CI 全矩阵防回归）缺 CI 自动化（E1）。
**Failure Modes Registry**：

```
CODEPATH        | FAILURE MODE        | RESCUED? | TEST? | USER SEES? | LOGGED?
----------------|---------------------|----------|-------|------------|--------
QWK 5.12 编译   | qmsetup 不兼容      | N/A(显式) | ✅3.2 | 编译错误   | ✅
QWK 5.12 交互   | 模拟路径体验差      | ✅D2回退 | ✅5.4 | 可见异常   | ✅
回归遗漏        | 高版本构建破坏      | ✅7.x   | ✅7.x | 显式       | ✅
```
CRITICAL GAPS: 0

**Completion Summary (CEO)**:

```
+====================================================================+
| Mode: SELECTIVE_EXPANSION | System Audit: 干净, 无 stash/TODO 相关   |
| S1 Arch: 0 | S2 Errors: 5 mapped, 0 GAPS | S3 Security: 0           |
| S4 Data/UX: 1 noted(HighDPI) | S5 Quality: 0 | S6 Tests: 矩阵覆盖   |
| S7 Perf: 0 | S8 Observ: 0 | S9 Deploy: 0 | S10 Future: 可逆 5/5    |
| S11 Design: 弱 scope, QWK 交互验证覆盖                              |
| NOT in scope: 4 | What exists: 5 | Dream delta: CI 自动化(E1)       |
| Failure modes: 3 rows, 0 CRITICAL | Scope: E1 待 gate (TASTE)       |
| Outside voice: Claude subagent [codex-unavailable]                  |
+====================================================================+
```

## Decision Audit Trail

<!-- AUTONOMOUS DECISION LOG -->
## Decision Audit Trail

| # | Phase | Decision | Classification | Principle | Rationale | Rejected |
|---|-------|----------|-----------|-----------|----------|
| 1 | CEO 0C-bis | Approach A（最小局部适配） | Mechanical | P5/P3 | 与 60+ 处守卫风格一致，最小可逆 | B（宏层）、C（分叉） |
| 2 | CEO 0D | E2 构建脚本 SKIP | Mechanical | P3 | README 记录命令即可 | — |
| 3 | CEO 0D | E3 qmake+QWK SKIP | Mechanical | Non-Goals | README 声明 qmake 遗留 | — |
| 4 | CEO S6 | 拒绝砍 5.14.2 验证 | Mechanical | P1 | 已声明支持版本需回归；PassThrough 边界需验证 | subagent 建议砍 |
| 5 | CEO S10 | 可逆性评估 5/5 | Mechanical | — | 守卫改动完全可逆 | — |
| 6 | CEO 0D | E1 CI 矩阵 → gate | Taste | P2 | blast radius 内但新增工作流文件 | — |
| 7 | CEO S2 | QWK 5.12 编译风险 → 实机验证 | Taste | P1 | qmsetup 兼容性未实测，tasks 3.2 覆盖 | — |
