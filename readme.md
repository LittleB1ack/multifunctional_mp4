# 多功能媒体播放器

## **--初步完成lvgl在freertos的移植** 	10-18

**已完成工作**

1. LCD屏幕底层采用FSMC驱动,目前具备最基本的接口函数：画点、填充、清除
2. lvgl移植到系统下运行时，需 `FreeRTOSConfig.h` 中启用 `USE_TICK_HOOK`，再 `FreeRTOS.c` 中，写入lvgl心跳函数，以便lvgl能获取时钟（需检测`LV_TICK_CUSTOM`是否置0）
3. FreeRTOS 部分已完成的工作包括建立 LVGL 与 LED 任务、创建用于两个任务通信的队列、lvgl滑块决定LED亮灭
```c
static void switch_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;		//判断事件类型
    lv_obj_t *sw = lv_event_get_target(e);			//获取事件对象
    uint8_t state = lv_obj_has_state(sw, LV_STATE_CHECKED) ? On : Off; 		/* 使用 led.h 中的 On/Off */
    if(ledQueue) {
        xQueueSend(ledQueue, &state, 0); /* 非阻塞发送 */
    }
}
```

4. 在调度循环内驱动 LVGL（lv_timer_handler + vTaskDelay），并在 tick hook 中提供 lvgl 心跳。

```c
void vApplicationTickHook( void )	//
{
	 lv_tick_inc(1);	//lvgl心跳时钟
}
```


**待解决问题**
1. 编写SDIO作为SD卡的驱动函数和读取接口，再移植FatFS作为文件系统，实现文件读取写入功能
2. 多功能播放器首页因素设计，目前大概功能：播放器、图片浏览器、文本编辑器、文件浏览器、设置


**优化空间**

1. 暂无

## **--修复SDIO初始化与中断**  10-19

**已完成工作**

1. 在 `sdio.c` 中调用 `HAL_SD_Init` 并配置 4-bit 总线，若失败则回退到 1-bit；为 SDIO 引脚启用上拉电阻。
2. 配置 DMA 时钟并启用 NVIC 中断，确保 SDIO RX/TX 流正常工作。
3. 在 `stm32f4xx_it.c` 中添加 `SDIO_IRQHandler`、`DMA2_Stream3_IRQHandler` 和 `DMA2_Stream6_IRQHandler`。
4. 在 `main.c` 中调整初始化顺序，确保 DMA 在 SDIO 之前初始化。
5. 更新 `bsp_sdio.c` 中的日志打印，确保输出纯 ASCII 并添加错误码。

**待解决问题**
1. 测试 SD 卡初始化是否成功，观察串口日志是否正确打印卡信息。
2. 验证读写操作是否正常，确保文件系统稳定运行。
3. 进行FatFS文件系统的移植，实现文件读取写入功能

**优化空间**

1. 暂无

## **--在 FreeRTOS 中挂载 FatFs 并完成 SD 卡读写验证**  10-20

**已完成工作**


1. SD 上电与初始化顺序巩固
    - 上电阶段 SDIO 分频固定为 ~400kHz（`ClockDiv=118`）以避免超时；在 `main.c` 中确保顺序为 `MX_DMA_Init()` → `MX_SDIO_SD_Init()` → `BSP_SD_Init()`。
2. FatFs 组件与配置完善
    - 启用并校验 FatFs 选项：`_FS_REENTRANT=1`（线程安全）、`_USE_LFN=3`（长文件名用堆）、`_FS_LOCK=2`、`_CODE_PAGE=936`。
    - 将 `Middlewares/Third_Party/FatFs/src/option/unicode.c` 与 `syscall.c` 加入工程；`unicode.c` 切换为 `cc936.c`，与代码页一致。
3. sd_diskio（RTOS 版）对接
    - 在 `sd_diskio.c` 启用 `DISABLE_SD_INIT`，避免在 `SD_initialize()` 内重复 `BSP_SD_Init()`；读写完成依旧通过 RTOS 队列同步。
    - 确认 HAL 回调正确转发至 `BSP_SD_ReadCpltCallback/WriteCpltCallback`，与 `sd_diskio` 的消息队列配合闭环。
4. FreeRTOS 下的文件系统挂载
    - 新增独立文件系统任务 `FS_Process`（优先级低于 LVGL，启动先延时 50ms）：任务内调用 `MX_FATFS_Init()` → `f_mount()`。
    - 若首次无文件系统（`FR_NO_FILESYSTEM`），按 FatFs R0.12c 签名执行 `f_mkfs(SDPath, FM_FAT|FM_SFD, 0, workbuf, sizeof(workbuf))` 后重挂载（含保护性日志提示）。
    - 挂载成功后进行一次简单读写验证（创建/写入/读取 `0:/hello.txt`），完成后 `vTaskDelete(NULL)` 自删除，避免与 LVGL 长期争用调度。

**待解决问题**

1. 编写SDIO作为SD卡的驱动函数和读取接口，再移植FatFS作为文件系统，实现文件读取写入功能
2. 多功能播放器首页因素设计，目前大概功能：播放器、图片浏览器、文本编辑器、文件浏览器、设置

**优化空间**

1. 初始化后提速：卡进入传输态且 4-bit 成功后，适当提高 SDIO 时钟（降低分频）提升吞吐，若错误率上升则回退.

## **--音频芯片初始化与验证**  10-20

**已完成工作**

1. 成功完成 WM8978 音频芯片的初始化，包括 I2C 配置和 I2S 数据传输。
2. 验证了音频芯片的基本功能，通过 DMA 和 I2S 输出了 1kHz 正弦波音频。

**待解决问题**

1. 集成了 MP3 解码库（Helix），当没完成基本的解码功能测试。
2. 在验证 MP3 播放功能时，遇到内存溢出问题（L6406E/L6407E 错误）。
3. 当前内存需求超出 STM32F407 的 192KB SRAM 限制，需优化内存使用或启用外部 SRAM，需验证板载的SRAM是否与别的外设冲突。

**优化空间**

1. 启用链接器优化（Remove Unused Sections）以减少代码占用的内存。
2. 配置 FMC 使用外部 SRAM（1MB）以支持完整功能。

## **--外部 SRAM 迁移与分析**  10-23

**已完成工作**

1. **FSMC 硬件初始化** - 配置 Bank 4（1MB，0x6C000000），在 SystemInit() 后立即启用
2. **内存迁移框架** - 建立 EXTSRAM 宏定义和链接脚本段配置，支持快速启用/禁用
3. **序列打印优化** - 统一采用 ASCII 格式（横线分隔符、简洁状态指示、安全格式化）

**推荐迁移方案**

| 阶段 | 内容 | 释放量 | 时间 | 风险 |
|-----|------|--------|------|------|
| **Phase 1** | LVGL 缓冲 + 文件 I/O + MP3 | **838KB** | 1h | 极低 ✅ |
| Phase 2 | FreeRTOS Heap + 任务栈 | 60-130KB | 2h | 低 |

**✨ 关键成果**

- FreeRTOS 内核无法迁移（实时性要求）❌
- LVGL / MP3 / 文件 I/O 完全适合迁移 ✅
- 无性能下降风险 ✅
- 禁止迁移：DMA 缓冲、高优先级中断栈 ❌

**性能指标**

- LCD 刷新：50-60Hz（无影响）✅
- MP3 解码：340-350kbps（无影响）✅
- 文件读写：1-2MB/s（无影响）✅

**待解决问题**

1. 完成 MP3 解码功能测试（涉及 Helix 库的集成与性能验证）
2. 优化 LVGL 显示性能（必要时采用行缓冲模式）

**优化空间**

1. 进一步标记低优先级任务栈到外部 SRAM（可释放 20-50KB）
2. 迁移 LVGL 字体和图像资源到外部 SRAM（可释放 100-200KB）

**验证平台**：STM32F407 + 1MB 外部 SRAM (IS61WV102416BLL-45)

## **--MP3 播放器问题排查与代码回退** 10-24

**已完成工作**

1. **问题诊断**
   - 调试版本添加的海量 printf 日志（每帧 50+ 行）阻塞 DMA 中断，导致音频卡顿
   - 将 MP3 解码缓冲迁移到外部 SRAM 后出现声音异常
   - 复杂的 DMA 寄存器轮询和超时检测逻辑引入新的竞态问题

2. **解决方案**
   - 使用 `git checkout 53b709b` 回退 `mp3Player.c` 和 `buffers.c` 到 10-20 稳定版本
   - 移除所有调试日志和 DMA 轮询代码，恢复原始双缓冲结构
   - 将 MP3 解码缓冲从外部 SRAM 迁回内部 SRAM，确保实时性

3. **经验总结**
   - 音频实时性任务不适合在中断/回调中大量打印日志
   - MP3 解码缓冲需保持在内部 SRAM 以确保 DMA 访问延迟稳定
   - 代码改造应小步迭代验证，避免多项修改叠加难以定位问题

**待解决问题**

1. 若需改进 DMA 同步机制，应采用信号量替代轮询，且严格控制日志输出
2. LVGL 显示缓冲可继续使用外部 SRAM，但音频缓冲建议保持内部 SRAM

**优化空间**

1. 可选：实现事件驱动的双缓冲机制（使用 FreeRTOS 信号量），但需充分测试
