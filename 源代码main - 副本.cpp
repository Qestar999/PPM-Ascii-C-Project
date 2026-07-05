#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 字符灰度表：空格最亮，@颜色最深
#define CHAR_ARR " .:-=+*#%@"
#define CHAR_TOTAL 10

// 自定义结构体：存储PPM图片的全部信息
struct PPM_PICTURE
{
    int width;
    int height;
    int max_color;
    unsigned char* pixel_data;
};
typedef struct PPM_PICTURE PPM_PICTURE;

// 全局窗口控件句柄
HWND window_path;
HWND window_result;
// 存储选中的文件路径
char file_location[260];
// 存储最终生成的字符画文本
char ascii_result[1024 * 50];

// 函数提前声明
PPM_PICTURE* read_ppm_file(const char* path);
void free_memory(PPM_PICTURE* pic);
void picture_to_ascii(PPM_PICTURE* pic, int output_w, int output_h, char* save_buf);
void open_file_select_window();
void make_ascii_image();
LRESULT CALLBACK window_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

// 
PPM_PICTURE* read_ppm_file(const char* path)
{
    FILE* file_ptr;
    file_ptr = fopen(path, "rb");
    if (file_ptr == NULL)
    {
        return NULL;
    }

    PPM_PICTURE* pic_info;
    pic_info = (PPM_PICTURE*)malloc(sizeof(PPM_PICTURE));
    if (pic_info == NULL)
    {
        fclose(file_ptr);
        return NULL;
    }

    char file_type[3];
    fscanf_s(file_ptr, "%2s", file_type, (unsigned int)sizeof(file_type));
    if (strcmp(file_type, "P6") != 0)
    {
        fclose(file_ptr);
        free(pic_info);
        return NULL;
    }

    // 跳过#开头的注释行
    int get_char;
    get_char = fgetc(file_ptr);
    while (get_char == '#')
    {
        while (fgetc(file_ptr) != '\n')
        {
        }
        get_char = fgetc(file_ptr);
    }
    ungetc(get_char, file_ptr);

    // 分步读取宽、高、最大颜色值
    fscanf_s(file_ptr, "%d", &pic_info->width);
    fscanf_s(file_ptr, "%d", &pic_info->height);
    fscanf_s(file_ptr, "%d", &pic_info->max_color);
    fgetc(file_ptr);

    int pixel_count;
    pixel_count = pic_info->width * pic_info->height;
    int total_byte;
    total_byte = pixel_count * 3;

    pic_info->pixel_data = (unsigned char*)malloc(total_byte);
    if (pic_info->pixel_data == NULL)
    {
        fclose(file_ptr);
        free(pic_info);
        return NULL;
    }

    fread(pic_info->pixel_data, 1, total_byte, file_ptr);
    fclose(file_ptr);

    return pic_info;
}

// 释放图片内存函数
void free_memory(PPM_PICTURE* pic)
{
    if (pic != NULL)
    {
        if (pic->pixel_data != NULL)
        {
            free(pic->pixel_data);
        }
        free(pic);
    }
}

// 图片转ASCII字符画函数
void picture_to_ascii(PPM_PICTURE* pic, int output_w, int output_h, char* save_buf)
{
    if (pic == NULL)
    {
        strcpy(save_buf, "错误：图片读取失败，请检查文件");
        return;
    }

    // 自动计算高度防止拉伸
    if (output_h <= 0)
    {
        float width_rate;
        width_rate = (float)output_w / pic->width;
        output_h = width_rate * pic->height;
        output_h = output_h * 0.48f;

        if (output_h < 1)
        {
            output_h = 1;
        }
    }

    float scale_x;
    scale_x = (float)pic->width / output_w;
    float scale_y;
    scale_y = (float)pic->height / output_h;

    char* write_point;
    write_point = save_buf;

    // 逐行逐列遍历像素
    for (int y = 0; y < output_h; y = y + 1)
    {
        for (int x = 0; x < output_w; x = x + 1)
        {
            int original_x;
            original_x = x * scale_x;
            original_x = original_x + scale_x / 2;

            int original_y;
            original_y = y * scale_y;
            original_y = original_y + scale_y / 2;

            // 防止坐标越界
            if (original_x >= pic->width)
            {
                original_x = pic->width - 1;
            }
            if (original_y >= pic->height)
            {
                original_y = pic->height - 1;
            }

            // 计算像素下标
            int pixel_index;
            pixel_index = original_y * pic->width;
            pixel_index = pixel_index + original_x;
            pixel_index = pixel_index * 3;

            // 取出RGB三个颜色
            unsigned char red, green, blue;
            red = pic->pixel_data[pixel_index];
            green = pic->pixel_data[pixel_index + 1];
            blue = pic->pixel_data[pixel_index + 2];

            // RGB转灰度分步计算
            float gray_value;
            gray_value = 0.299f * red;
            gray_value = gray_value + 0.587f * green;
            gray_value = gray_value + 0.114f * blue;
            gray_value = gray_value / pic->max_color;

            // 匹配对应字符
            int char_index;
            char_index = gray_value * (CHAR_TOTAL - 1);
            char_index = char_index + 0.5f;

            // 限制下标范围
            if (char_index >= CHAR_TOTAL)
            {
                char_index = CHAR_TOTAL - 1;
            }
            if (char_index < 0)
            {
                char_index = 0;
            }

            
            *write_point = CHAR_ARR[char_index];
            write_point = write_point + 1;
        }
        // 换行
        *write_point = '\n';
        write_point = write_point + 1;
    }
    // 字符串结束标记
    *write_point = '\0';
}

// 打开文件选择窗口，全部中文配置
void open_file_select_window()
{
    OPENFILENAMEA file_config;
    memset(&file_config, 0, sizeof(OPENFILENAMEA));
    char temp_path[260];
    memset(temp_path, 0, sizeof(temp_path));

    file_config.lStructSize = sizeof(OPENFILENAMEA);
    file_config.hwndOwner = GetActiveWindow();
    file_config.lpstrFilter = "PPM图片文件\0*.ppm\0所有文件\0*.*\0";
    file_config.lpstrFile = temp_path;
    file_config.nMaxFile = 260;
    file_config.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileNameA(&file_config))
    {
        strcpy(file_location, temp_path);
        SetWindowTextA(window_path, file_location);
        make_ascii_image();
    }
}

// 生成字符画主逻辑
void make_ascii_image()
{
    if (strlen(file_location) == 0)
    {
        SetWindowTextA(window_result, "请先选择一个PPM格式的图片文件！");
        return;
    }

    PPM_PICTURE* picture;
    picture = read_ppm_file(file_location);
    if (picture == NULL)
    {
        SetWindowTextA(window_result, "图片加载失败，请确认是标准P6格式PPM文件");
        return;
    }

    char temp_buf[1024 * 50];
    memset(temp_buf, 0, sizeof(temp_buf));
    picture_to_ascii(picture, 80, 0, temp_buf);

    free_memory(picture);

    strcpy(ascii_result, temp_buf);
    SetWindowTextA(window_result, temp_buf);
}

// 窗口回调函数：所有case添加{}，彻底解决C2360编译错误
LRESULT CALLBACK window_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        // 创建按钮、输入框，全部中文文字
        CreateWindowA("BUTTON", "选择PPM图片", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            20, 20, 120, 35, hwnd, (HMENU)1, NULL, NULL);

        CreateWindowA("BUTTON", "生成字符画", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            150, 20, 100, 35, hwnd, (HMENU)2, NULL, NULL);

        window_path = CreateWindowA("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_READONLY,
            260, 20, 400, 35, hwnd, (HMENU)3, NULL, NULL);

        window_result = CreateWindowA("EDIT", "请先选择PPM图片，再点击生成字符画按钮",
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY,
            20, 70, 700, 450, hwnd, (HMENU)4, NULL, NULL);

        // 设置等宽字体，防止排版错乱
        HFONT font_style;
        font_style = CreateFontA(14, 0, 0, 0, FW_NORMAL, 0, 0, 0,
            DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH | FF_MODERN, "Courier New");
        SendMessage(window_result, WM_SETFONT, (WPARAM)font_style, TRUE);
        break;
    }
    case WM_COMMAND:
    {
        int btn_id;
        btn_id = LOWORD(wparam);
        if (btn_id == 1)
        {
            open_file_select_window();
        }
        if (btn_id == 2)
        {
            make_ascii_image();
        }
        break;
    }
    case WM_SIZE:
    {
        RECT win_rect;
        GetClientRect(hwnd, &win_rect);
        int new_width;
        new_width = win_rect.right - 40;
        int new_height;
        new_height = win_rect.bottom - 100;
        SetWindowPos(window_result, NULL, 20, 70, new_width, new_height, SWP_NOZORDER);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcA(hwnd, msg, wparam, lparam);
    }
    return 0;
}

// 程序入口函数，全局数组初始化，无随机值报错
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    // 全局数组初始化
    memset(file_location, 0, sizeof(file_location));
    memset(ascii_result, 0, sizeof(ascii_result));

    WNDCLASSA win_class;
    memset(&win_class, 0, sizeof(WNDCLASSA));

    win_class.style = CS_HREDRAW | CS_VREDRAW;
    win_class.lpfnWndProc = window_message;
    win_class.hInstance = hInstance;
    win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    win_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    win_class.lpszClassName = "AsciiPictureWindow";
    RegisterClassA(&win_class);

    // 创建主窗口，中文标题
    HWND main_window;
    main_window = CreateWindowA("AsciiPictureWindow", "ASCII字符画生成器-C语言课程作业",
        WS_OVERLAPPEDWINDOW, 100, 100, 780, 580, NULL, NULL, hInstance, NULL);

    ShowWindow(main_window, nShowCmd);
    UpdateWindow(main_window);

    MSG msg_info;
    while (GetMessage(&msg_info, NULL, 0, 0))
    {
        TranslateMessage(&msg_info);
        DispatchMessage(&msg_info);
    }

    return (int)msg_info.wParam;
}