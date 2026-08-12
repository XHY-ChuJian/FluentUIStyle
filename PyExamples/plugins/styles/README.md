请将编译出来的 C++ 插件 `fluentui3styleplugin.dll` (Windows) 或对应的 .so (Linux) / .dylib (macOS) 复制到这个目录下。
这样上一级的 `main.py` 才能通过 `QCoreApplication.addLibraryPath` 找到并加载它。
