# XTrader

XTrade 的桌面交易终端，原项目名 Mary。配合 XEngine 策略服务、XBroker 交易柜台和 HQMarket 行情服务使用。

打开 `XTrader.sln`，使用 Visual Studio 的 Qt/MSBuild、Qt 6.11.2 MSVC 64 位及 C++20 工具链构建。主工程为 `XTrader/XTrader.vcxproj`，输出程序为 `XTrader.exe`；运行目录仍为 `XTrader/exec/Debug` 或 `XTrader/exec/Release`。

改名保留已有本地数据与连接设置。登录选项继续读取原有 `QSettings("Mary", "Mary")` 命名空间；历史 `local.wind` 配置键及 `wind_simulated` 登录键保留兼容，不需要重新创建账户。模拟柜台实际由 XBroker 提供。

GitHub 仓库：https://github.com/fengwanggangx/XTrader
