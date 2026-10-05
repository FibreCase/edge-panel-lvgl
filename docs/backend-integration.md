# 现有 Python 后端与 LVGL 接入

依据 `refer/edge-panel-python` 中的本地源码进行静态分析；没有启动服务或访问真实天气 API。
参考目录只用于研究，已通过根目录 `.gitignore` 的 `/refer/` 排除。

## 服务边界

FastAPI 提供 HTTP 接口，python-socketio 的 ASGI 包装层提供 Socket.IO。入口为 `app/main.py`，默认监听 `0.0.0.0:5000`。SQLite 保存消息，后端磁盘缓存保存天气和上传图片。LVGL 前端无需复制数据库、和风天气签名或管理后台功能。

当前可展示内容为天气、一个事件、消息列表，外加前端本地时钟。事件来自固定占位数据，不能当作真实日程服务。

## 现有协议

| 操作 | 接口/事件 | 返回及行为 |
|---|---|---|
| 读取消息 | `GET /api/messages` | `{ "messages": [...] }`，按创建时间降序、ID 降序，无分页 |
| 请求天气 | Socket.IO `request_weather` | 当前连接收到 `weather_data`；失败收到 `weather_error` |
| 请求事件 | Socket.IO `request_event` | 当前连接收到 `event_data`；失败收到 `event_error` |
| 消息变化 | Socket.IO `messages_updated` | 广播变更通知，载荷随操作变化 |
| 读取图片 | 消息 `content` 中的 URL | 上传文件经 `/uploads/` 提供 |

天气和事件没有现成的 HTTP GET 接口，也没有后台定时广播；需要客户端主动请求。现有公共读取不要求管理令牌。

### 消息模型

字段为 `id`、`type`、`content`、`sub_content`、`source_name`、`created_at`。

- `text`：正文在 `content`；创建接口拒绝非空 `sub_content`，当前存储实现也没有自动生成副标题，因此前端应根据 `created_at` 格式化时间。接口错误文案所说的“服务器生成副标题”与实现不一致。
- `image`：`content` 为图片 URL；`sub_content`、`source_name` 为 null。
- `notify`：正文在 `content`，来源在 `source_name`；`sub_content` 可为 null。
- `created_at` 为 SQLite 的服务器本地时间字符串，没有时区偏移；部署时需明确服务器时区，不能直接当作 UTC。

`messages_updated` 可能包含 `{message: ...}`、`{deleted_id: ...}`、`{restored_id: ...}` 或 `{cleared: true}`。首版统一将其视为“消息快照失效”，合并通知后重新 GET 列表。连接恢复后也重新获取快照，避免漏掉断线期间的删除或恢复。

### 天气模型

`weather_data` 字段为 `weather`、`temperature`、`icon`、`rain_notification`、`aqi`、`aqi_category`。温度与图标通常为字符串，AQI 可能为数值；解析层处理 null、空串和类型差异，再转换为前端领域模型。当前天气描述使用 `lang=en`，降水和空气质量使用中文，前端不应假设描述全部是中文。

后端当前天气和空气质量缓存 1800 秒；降水缓存有雨时 300 秒、摘要恰为“未来两小时无降水”时 3600 秒。缓存过期且请求失败时，当前实现不会回退到过期缓存。天气聚合串行读取三个来源，其中任一失败都会产生整体 `weather_error`。载荷没有观测时间或缓存时间，前端只能标记接收时间，不能据此声称数据刚刚观测。

### 事件模型

`event_data` 包含 `name`、`time`、`date`、`location`，目前全部为固定值。首版可用于卡片布局，但应明确是占位数据。

## 接入路径

### 保持后端兼容

HTTP 客户端负责消息快照和图片，Socket.IO 客户端负责天气、事件与消息通知。Socket.IO 是带握手、心跳和事件封装的协议，不能将普通 WebSocket 客户端直接视为兼容实现。原生客户端依赖需要另行核对与后端 python-socketio 5.x 的兼容性。

建议先沿用协议，完整消息展示可先由 HTTP 跑通，天气/事件在选定兼容客户端后接入。不手写一个缺少重连、心跳与握手的 Socket.IO 简化实现。

### 可选后端扩展

如果希望 C++ 客户端依赖更少，可在后端增加 `GET /api/weather` 和 `GET /api/event`，复用现有 service，并保留原 Socket.IO 接口供旧前端使用。这样前端可先全部 HTTP 轮询，再评估实时通知。此方案需要修改并部署后端，当前尚未实施。

## LVGL 模块落点

```text
services/backend_client      # 协议、连接、请求与重试
services/image_cache         # 图片下载、解码/缩放与有界缓存
model/panel_state            # WeatherState、EventState、MessageState
ui/components/weather_card
ui/components/event_card
ui/components/message_card   # text / image / notify
```

启动时先显示本地时钟与缓存数据；网络连接后拉取消息并请求天气、事件。建议初始策略为天气每 5 分钟、事件每 5 分钟刷新，消息通过通知刷新并每 60 秒兜底同步；具体间隔可配置。若采用纯 HTTP 路径，可先按 5–10 秒轮询消息。

每类数据独立记录加载状态、接收时间与错误。失败保留最近成功数据并显示过期/离线状态，连接恢复后重新同步。所有网络、图片下载及耗时解码在 UI 线程之外处理，结果经有界事件队列提交；LVGL 对象和图像资源生命周期由 UI 线程管理。

图片首版限制支持的格式、下载体积、解码像素总量和缓存容量，按实际展示区域缩放，避免多张全尺寸图片占满内存。后端仅 HEIC/HEIF 上传会转换为 JPEG，其他文件可能保留 PNG、GIF、WebP 等原格式，因此不能假设所有图片都可由同一解码器处理。

## 对接时需处理的限制

- 上传图片默认生成 `http://127.0.0.1:5000/uploads/...` 地址；后端与屏幕不在同一台机器时，应设置设备可达的 `PUBLIC_BASE_URL` 或 `LOCAL_BASE_URL`。当前 compose 文件未传入这两个变量，容器部署时需补充配置。
- 消息列表无分页、无数量限制。前端可限制展示条数，但不能减少响应体，规模增长后应增加后端分页或 limit 参数。
- Socket.IO 的异步天气处理器直接调用同步网络请求，冷缓存时会阻塞事件循环，并可能影响心跳及消息通知；后续后端改进应考虑线程卸载或异步 I/O。
- 删除、恢复与清空需要管理登录和 Bearer 令牌；面板首版只读展示，继续使用现有网页管理页面。管理令牌有效期 8 小时，后端重启失效。

## 下一步

先实现 mock 的天气、事件和三类消息卡片，确定横屏布局；随后接入真实 HTTP 消息和图片，再确定 Socket.IO 客户端或后端 HTTP 扩展。端到端验证包括新增、删除、恢复、清空、断网重连、坏图片和天气错误。
