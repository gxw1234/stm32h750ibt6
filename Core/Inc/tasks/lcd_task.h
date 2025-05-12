#ifndef LCD_TASK_H
#define LCD_TASK_H

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

/* 屏幕参数定义 */
#define LCD_WIDTH  240
#define LCD_HEIGHT 240

/* 颜色定义 */
#define COLOR_BLACK   0x0000
#define COLOR_BLUE    0x001F
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F
#define COLOR_YELLOW  0xFFE0
#define COLOR_WHITE   0xFFFF



/* 字体参数 */
#define FONT_0806      8 /* 8x6 像素字体 */
#define FONT_1206     12 /* 12x6 像素字体 */
#define FONT_1608     16 /* 16x8 像素字体 */
#define FONT_2010     20 /* 20x10 像素字体 */
#define FONT_2412     24 /* 24x12 像素字体 */

/* LCD控制函数声明 */
void LCD_Init(void);
void LCD_Clear(uint16_t color);
void LCD_Draw_Point(uint16_t x, uint16_t y, uint16_t color);
void LCD_Draw_Line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void LCD_Fill_Rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void LCD_Set_Window(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end);

/* 字符显示函数声明 */
void LCD_Show_Char(uint16_t x, uint16_t y, uint8_t ch, uint16_t color, uint16_t bg_color, uint8_t size);
void LCD_Show_String(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg_color, uint8_t size);


/* FreeRTOS任务函数声明 */
void LCD_Task(void *pvParameters);

#endif /* LCD_TASK_H */
