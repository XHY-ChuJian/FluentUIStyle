# Third-Party Notices

This repository includes the following vendored third-party components under
`3rd/qwindowkit`:

| Component | Upstream revision | License |
| --- | --- | --- |
| QWindowKit | 1.5.0 (`35e88f3655720ed0537c7dd1dde243bf4a70c94c`) | Apache License 2.0 (`3rd/qwindowkit/LICENSE`) |
| qmsetup | `85c6c3c783be8af8d3f2fa492748a82da8ec9bad` | MIT License (`3rd/qwindowkit/qmsetup/LICENSE`) |
| syscmdline | `5a67673ff96acbfd894ea653fbaca872fded758a` | MIT License (`3rd/qwindowkit/qmsetup/src/syscmdline/LICENSE`) |

QWindowKit remains based on its upstream 1.5.0 release. The bundled qmsetup
and syscmdline contain local compatibility changes for the MinGW GCC 7.3
toolchain shipped with Qt 5.12.12. See `3rd/qwindowkit/VENDORED.md` for the
vendoring record.
