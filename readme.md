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