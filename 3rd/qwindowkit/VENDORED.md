# Vendored QWindowKit

This directory contains a locally maintained copy of QWindowKit and its
required build-time dependencies.

## Upstream revisions

- QWindowKit: <https://github.com/stdware/qwindowkit>, tag `1.5.0`, commit
  `35e88f3655720ed0537c7dd1dde243bf4a70c94c`
- qmsetup: <https://github.com/stdware/qmsetup>, commit
  `85c6c3c783be8af8d3f2fa492748a82da8ec9bad`
- syscmdline: <https://github.com/SineStriker/syscmdline>, commit
  `5a67673ff96acbfd894ea653fbaca872fded758a`

## Local changes

The bundled qmsetup has compatibility adjustments for the MinGW GCC 7.3
toolchain distributed with Qt 5.12.12. QWindowKit itself remains based on the
unmodified 1.5.0 release sources.

## Licenses

- QWindowKit is licensed under Apache License 2.0; see `LICENSE`.
- qmsetup is licensed under the MIT License; see `qmsetup/LICENSE`.
- syscmdline is licensed under the MIT License; see
  `qmsetup/src/syscmdline/LICENSE`.

Keep these license files and this revision record when redistributing or
updating the vendored sources.
