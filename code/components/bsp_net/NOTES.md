# lwIP Socket（UDP/TCP）学习笔记（第五十二~五十四章）

## 原理要点
- lwIP 提供 BSD Socket API（`lwip/sockets.h`），与标准 POSIX socket 兼容。
- 前提：先连接 WiFi（STA）拿到 IP，再做 socket 通信。
- UDP（SOCK_DGRAM）：`bind` + `recvfrom` + `sendto`，无连接。
- TCP Client（SOCK_STREAM）：`connect` + `send` + `recv`。
- TCP Server：`bind` + `listen` + `accept` + `recv`/`send` 回显。

## 配置
- 目标 IP/端口走 menuconfig（`SOCKET_REMOTE_IP`、`SOCKET_PORT`）。

## 验证结果

- ✅ UDP：`UDP listening on port 8080`（IP 192.168.1.22）
- ✅ TCP server：`TCP server listening on port 8080`
- 待验证：TCP client（需 PC 端跑 server）
