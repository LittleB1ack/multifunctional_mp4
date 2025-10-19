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

1. 初始化后提速：卡进入传输态且 4-bit 成功后，适当提高 SDIO 时钟（降低分频）提升吞吐，若错误率上升则回退。
