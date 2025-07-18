#  OS 2024 Lab1: Shared Memory & Message Passing 作業紀錄
實作兩種常見的 **Inter-Process Communication (IPC)** 機制：
- **Message Passing**
- **Shared Memory**
透過 `sender` 與 `receiver` ，模擬訊息的傳遞與接收，並比較這兩種 IPC 方法在效能上的差異。

## 主要內容
| `sender.c` | 發送訊息給接收端，支援 Message Queue 或 Shared Memory |
| `receiver.c` | 接收訊息並輸出內容，直到接收到 Exit 訊息為止 |

### 編譯
```bash
make
```

### 執行
```bash
./receiver 1          # 啟動 receiver，使用 Message Passing 模式
./sender 1 message.txt # 啟動 sender，傳送 message.txt 的訊息
```
`1 = Message Passing`，`2 = Shared Memory`

