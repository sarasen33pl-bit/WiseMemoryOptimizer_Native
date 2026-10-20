# Contributing to Wise Memory Optimizer Native

Thank you for your interest in contributing to **Wise Memory Optimizer Native**! We welcome all contributions from bug fixes and documentation improvements to new features and translations.

---

## 🛠️ Development Setup

### Prerequisites
- Windows 10 or 11 (x64)
- Visual Studio 2022 with the **Desktop development with C++** workload (MSVC v143+, C++20 support).

### Building from Source
1. Clone the repository:
   ```cmd
   git clone https://github.com/sarasen33pl-bit/WiseMemoryOptimizer_Native.git
   cd WiseMemoryOptimizer_Native
   ```
2. Build the project:
   ```cmd
   build.bat
   ```
3. The executable `WiseMemoryOptimizer_Native.exe` will be compiled and copied to the root directory.

---

## 📐 Code Guidelines

- **Standard:** Modern C++20 strictly.
- **Dependencies:** 100% pure native Win32/GDI+ APIs. **No third-party runtime DLLs or heavy frameworks.**
- **Formatting:** Clean, readable, well-commented code following Windows API naming conventions.
- **Performance:** Keep animation loops double-buffered, zero-allocation in render ticks, and sub-millisecond execution.

---

## 🐛 Reporting Bugs & Suggestions

- Use GitHub Issues to submit bug reports with reproducible steps and OS version details.
- For feature requests, describe the use case and expected behavior.
