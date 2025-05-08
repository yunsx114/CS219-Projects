#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <time.h> 
#include "arm_neon.h"

#define _OPENMP_            //是否启用OPENMP
#define _NEON_              //是否启用NEON(ARM架构才能用)
#define _OUTPUTTIME_        //是否输出运行时间
#define _SIMPLE_LIGHT_ADD_  //是否采用简单亮度提升
#define FLT_EPSILON 1e-6f   //浮点较小值
#define PI 3.1415926        //圆周率


#define CHECK_NULL(ptr, msg) if ((ptr) == NULL) { fprintf(stderr, "Error: %s\n", msg); exit(EXIT_FAILURE); }
#define CHECK_FILE(file, filename, msg) if ((file) == NULL) { fprintf(stderr, "Error: %s '%s'\n", msg, filename); exit(EXIT_FAILURE); }
#define COMPARE(NAME1,NAME2,NUM) strcmp(NAME1, NAME2) == 0 && i + NUM < argc
#define PIC_ROW_SIZE(ars,image) ars = image->dib_header.width * 3 + (4 - ((image->dib_header.width * 3) % 4)) % 4;

/* 用 "opt/homebrew/opt/llvm/bin/clang -fopenmp ..." 代替 "gcc" 来启用OpenMP */
#ifdef _OPENMP_
#include <omp.h>  
#endif

#ifdef _OUTPUTTIME_
    #define TIME_INIT struct timespec start, end;
    #define TIME_START clock_gettime(CLOCK_MONOTONIC, &start);
    #define TIME_END(OPNAME) clock_gettime(CLOCK_MONOTONIC, &end);printf("The %s operation finished in: %.6f millisecond\n", get_op_name(OPNAME), time_diff(&start, &end) / 1e6);
#else
    #define TIME_INIT
    #define TIME_START
    #define TIME_END(OPNAME)
#endif

 
#pragma pack(push, 1) // Ensure no padding in structs
typedef struct {
    char signature[2];  //must be "BM"
    int file_size;
    short reserved1;
    short reserved2;
    int data_offset;
} BMPHeader;

typedef struct {
    int header_size;
    int width;
    int height;
    short planes;
    short bits_per_pixel;
    int compression;
    int image_size;
    int x_pixels_per_meter;
    int y_pixels_per_meter;
    int colors_used;
    int important_colors;
} DIBHeader;
#pragma pack(pop)

typedef struct {
    BMPHeader file_header;
    DIBHeader dib_header;
    unsigned char* pixel_data;
} BMPImage;
 

static float VOID_MATRIX[] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 0
};
static float VINTAGE_MATRIX[] = {
    0.393f, 0.769f, 0.189f, 0,
    0.349f, 0.686f, 0.168f, 0,
    0.272f, 0.534f, 0.131f, 0,
    0,      0,      0,      0
};
static float INVERT_MATRIX[] = {
    -1,     0,      0,     255,
    0,     -1,      0,     255,
    0,      0,     -1,     255,
    0,      0,      0,       0
};
static float GRAYSCALE_MATRIX[] = {
    0.299f, 0.587f, 0.114f, 0,
    0.299f, 0.587f, 0.114f, 0,
    0.299f, 0.587f, 0.114f, 0,
    0,      0,      0,      0
};
static float* FILTER_MATRICES[] = {
    VOID_MATRIX,            // 0
    VINTAGE_MATRIX,         // 1
    INVERT_MATRIX,          // 2
    GRAYSCALE_MATRIX        // 3
};



static float VOID_KERNEL[] = {
    0, 0, 0,
    0, 1, 0,
    0, 0, 0
};
static float LAPLACIAN_KERNEL[] = {
    -1, -1, -1,
    -1, 9 , -1,
    -1, -1, -1
};
static float SOBEL_KERNEL[] = {
    -1, 0, 1,
    -2, 1, 2,
    -1 ,0, 1
};
static float MASK_KERNEL[] = {
    0 ,-1, 0,
    -1, 5,-1,
    0 ,-1, 0
};
static float* KERNALS[] = {
    VOID_KERNEL,        //0
    LAPLACIAN_KERNEL,   //1
    SOBEL_KERNEL,       //2
    MASK_KERNEL         //3
};


typedef enum {
    OP_ADD_L,       // 亮度 ✅
    OP_ADD_H,       // 色调 ✅
    OP_ADD_S,       // 饱和度 ✅
    OP_ADD_V,       // 明度 ✅
    OP_BLEND,       // 混合 ✅
    OP_RESCALE,     // 缩放 ✅
    OP_CUT,         // 裁剪 ✅
    OP_ROTATE,      // 旋转 ✅
    OP_FLIP,        // 镜像 ✅
    OP_SHARP,       // 锐化 ✅
    OP_BLUR,        // 模糊 ✅
    OP_FILTER,      // 滤镜 ✅
    OP_BINARIZE,    // 二值化 ✅
} OperationType;

typedef struct {
    OperationType type;
    int int1; // 类似寄存器的效果
    int int2;
    float float1;
    int int3;
    int int4;
    char* blend_image_path;
} Operation;

// 函数指针定义
typedef void (*BinarizeFunc)(unsigned char*, unsigned char);
typedef void (*AdjustFunc)(unsigned char* pixel, float int1,float int2);
typedef void (*LightFunc)(uint8x16_t* v_pixels, uint8x16_t v_offset);

/* 函数声明 */
// 读写函数
void cvt32_24bmp(FILE* file, BMPImage* image);
void cvt8_24bmp(FILE* file, BMPImage* image);
void write_bmp(const char* filename, BMPImage* image);
void free_bmp(BMPImage* image);
BMPImage* read_bmp(const char* filename);

// 功能函数
void image_color_adjust(BMPImage* image, int val, size_t type);
void image_simp_light_adjust(BMPImage* image, int L_val);
void image_Binarization(BMPImage* image,unsigned char threshold,int type);
void image_filter(BMPImage* image,int type);
void image_flip(BMPImage* image);
BMPImage* image_rescale(BMPImage* image, size_t new_width, size_t new_height);
BMPImage* image_cut(BMPImage* image, size_t offset_x, size_t offset_y, size_t new_width, size_t new_height);
BMPImage* image_rotate(BMPImage* image, float angle);
BMPImage* images_blend(BMPImage* image1, BMPImage* image2, float ratio);
BMPImage* image_G_Blur(BMPImage* image, float scale, size_t iter);
BMPImage* image_sharpen(BMPImage* image, int type, size_t iter);

// 内部函数
BinarizeFunc get_binarize_func(int type);
AdjustFunc get_adjust_func(int type);
LightFunc get_light_func(int type);

void cvtRGB_HSV(int* R, int* G, int* B, float* H, float* S, float* V);
void cvtHSV_RGB(float* H, float* S, float* V,int* R, int* G, int* B);

void binarize_norm(unsigned char* pixel, unsigned char threshold);
void binarize_inv(unsigned char* pixel, unsigned char threshold);
void binarize_trunc(unsigned char* pixel, unsigned char threshold);
void binarize_tozero(unsigned char* pixel, unsigned char threshold);
void binarize_tozero_inv(unsigned char* pixel, unsigned char threshold);

void adjust_L(unsigned char* pixel, float delt, float offset);
void adjust_H(unsigned char* pixel, float H_v, float none);
void adjust_S(unsigned char* pixel, float H_v, float none);
void adjust_V(unsigned char* pixel, float H_v, float none);
void adjust_NONE(unsigned char* pixel, float val, float none);

void light_add(uint8x16_t* v_pixels, uint8x16_t v_offset);
void light_sub(uint8x16_t* v_pixels, uint8x16_t v_offset);

void mix_arrays(uint8_t * p1, const uint8_t * p2, float ratio, size_t num);
void generate_G_kernel_1D(float* kernel, int size);
void bilinearVec3(const unsigned char* p1, const unsigned char* p2, unsigned char* p3, float dx, float dy, int _x1, int _x2);

long time_diff(struct timespec* start, struct timespec* end);
const char* get_op_name(int OPNAME);

BMPImage* create_new_image(BMPImage* ref_img, size_t new_width, size_t new_height);
BMPImage* image_scale_larger(BMPImage* image,size_t new_height,size_t new_width);
BMPImage* image_scale_smaller(BMPImage* image, size_t new_width, size_t new_height);

void print_help();


const char* get_op_name(int OPNAME) 
{
    switch (OPNAME) 
    {
        case OP_ADD_L: return "addL";
        case OP_ADD_H: return "addH";
        case OP_ADD_S: return "addS";
        case OP_ADD_V: return "addV";
        case OP_BLEND: return "blend";
        case OP_RESCALE: return "rescale";
        case OP_CUT: return "cut";
        case OP_ROTATE: return "rotate";
        case OP_FLIP: return "flip";
        case OP_SHARP: return "sharp";
        case OP_FILTER: return "filter";
        case OP_BINARIZE: return "binarize";
        default: return "unknown";
    }
}
long time_diff(struct timespec *start, struct timespec *end) 
{
    if (start && end) 
        return (end->tv_sec - start->tv_sec) * 1e9 + (end->tv_nsec - start->tv_nsec);
    else {
        printf("wrong time interval parsing\n");
        return 0;
    }
}

// 帮助文档
void print_help() 
{
    printf("BMP图像处理工具\n");
    printf("支持8/24/32位BMP文件格式\n\n");
    printf("用法: ./bmpedit1 -i 输入文件.bmp -o 输出文件.bmp -op <操作> [参数]\n");
    printf("选项:\n");
    printf("  -i <文件名>                 输入BMP文件路径 (必填)\n");
    printf("  -o <文件名>                 输出BMP文件路径 (必填)\n");
    printf("  -op <操作> <参数>           要执行的操作及参数\n");
    printf("                              可通过多次输入-op进行多种操作,并按输入顺序执行\n\n");
    printf("可用操作:\n");
    printf("  addL <值>                  调整亮度 (范围: -100到100)\n");
    printf("  addH <值>                  调整色调 (范围: 任意整数)\n");
    printf("  addS <值>                  调整饱和度 (范围: -100到100)\n");
    printf("  addV <值>                  调整明度 (范围: -100到100)\n");
    printf("  blend <路径> <比例>        与另一图像混合 (比例: 0.0-1.0)\n");
    printf("  rescale <宽> <高>          缩放图像到新尺寸 (宽高为正整数)\n");
    printf("  cut <x偏移> <y偏移> <新宽> <新高>\n");
    printf("                            裁剪图像 (以左下角为原点)\n");
    printf("  rotate <角度>              旋转图像 (角度范围: 任意浮点数)\n");
    printf("  flip                       水平翻转图像\n");
    printf("  sharp <类型> <迭代次数>    锐化图像 (类型: 0-3, 迭代次数: 正整数)\n");
    printf("                                类型说明:\n");
    printf("                                0-无效果\n");
    printf("                                1-拉普拉斯锐化\n");
    printf("                                2-索贝尔锐化\n");
    printf("                                3-掩模锐化\n");
    printf("  blur <比例> <迭代次数>     高斯模糊 (比例: 0.0-1.0,对应高斯核1-41宽度, 迭代次数: 正整数)\n");
    printf("  filter <类型>              应用滤镜 (类型: 0-3)\n");
    printf("                                类型说明:\n");
    printf("                                0-原图\n");
    printf("                                1-复古\n");
    printf("                                2-反色\n");
    printf("                                3-灰度\n");
    printf("  binarize <阈值> <类型>     二值化 (阈值: 0-255, 类型: 0-4)\n");
    printf("                                类型说明:\n");
    printf("                                0-普通二值化\n");
    printf("                                1-反色二值化\n");
    printf("                                2-截断二值化\n");
    printf("                                3-零处理二值化\n");
    printf("                                4-反色零处理\n\n");
    printf("示例:\n");
    printf("  调整亮度并应用复古滤镜:\n");
    printf("  ./bmpedit -i in.bmp -o out.bmp -op addL 50 -op filter 1\n");
    printf("  混合两张图片:\n");
    printf("  ./bmpedit -i in.bmp -o out.bmp -op blend other.bmp 0.5\n");
    printf("  旋转并缩放图像:\n");
    printf("  ./bmpedit -i in.bmp -o out.bmp -op rotate 90 -op rescale 800 600\n");
    printf("  高斯模糊迭代三次:\n");
    printf("  ./bmpedit -i in.bmp -o out.bmp -op blur 0.5 3\n");
    printf("  得到竖直镜像:\n");
    printf("  ./bmpedit -i in.bmp -o out.bmp -op flip -op rotate 180 \n");
}

int main(int argc, char* argv[]) 
{
    if (argc < 2 || strcmp(argv[1], "-help") == 0) 
    {
        print_help();
        return 0;
    }

#ifdef _OPENMP_
    int max_threads = omp_get_max_threads();
    omp_set_num_threads(max_threads);
#endif
    
    //TIME_INIT

    char *input_path = NULL;
    char *output_path = NULL;
    Operation *operations = (Operation*)malloc(argc * sizeof(Operation));
    CHECK_NULL(operations, "Memory reallocation for operation queue failed");
    int op_count = 0;

    for (int i = 1; i < argc; i++) 
    {
        if (COMPARE(argv[i],"-i",1)) 
            input_path = argv[++i];
        else if (COMPARE(argv[i],"-o",1))
            output_path = argv[++i];
        else if (COMPARE(argv[i],"-op",1)) 
        {
            char * op_name = argv[++i];
            Operation op = {0};

            if (COMPARE(op_name,"addL",1)) 
            {
                op.type = OP_ADD_L;
                op.int1 = atoi(argv[++i]); // Lightness
            }
            else if (COMPARE(op_name,"addH",1)) 
            {
                op.type = OP_ADD_H;
                op.int1 = atoi(argv[++i]); // Hue
            } 
            else if (COMPARE(op_name,"addS",1)) 
            {
                op.type = OP_ADD_S;
                op.int1 = atoi(argv[++i]); // Saturation
            } 
            else if (COMPARE(op_name,"addV",1)) 
            {
                op.type = OP_ADD_V;
                op.int1 = atoi(argv[++i]); // Value
            } 
            else if (COMPARE(op_name,"blend",2)) 
            {
                op.type = OP_BLEND;
                op.blend_image_path = argv[++i]; // path
                op.float1 = atof(argv[++i]);
            } 
            else if (COMPARE(op_name,"rescale",2)) 
            {
                op.type = OP_RESCALE;
                op.int1 = atoi(argv[++i]); // width
                op.int2 = atoi(argv[++i]); // height
            }
            else if (COMPARE(op_name,"cut",4)) 
            {
                op.type = OP_CUT;
                op.int1 = atoi(argv[++i]); // offset_x
                op.int2 = atoi(argv[++i]); // offset_y
                op.int3 = atoi(argv[++i]); // new width
                op.int4 = atoi(argv[++i]); // new height
            } 
            else if (COMPARE(op_name,"rotate",1)) 
            {
                op.type = OP_ROTATE;
                op.float1 = atof(argv[++i]); // angle
            } 
            else if (COMPARE(op_name,"flip",0))
            {
                op.type = OP_FLIP;
            } 
            else if (COMPARE(op_name,"sharp",2)) 
            {
                op.type = OP_SHARP;
                op.int1 = atoi(argv[++i]); // type
                op.int2 = atoi(argv[++i]); // iteration steps
            } 
            else if (COMPARE(op_name,"blur",2)) 
            {
                op.type = OP_BLUR;
                op.float1 = atof(argv[++i]); // scale
                op.int1 = atoi(argv[++i]); // iteration steps
            } 
            else if (COMPARE(op_name,"filter",1)) 
            {
                op.type = OP_FILTER;
                op.int1 = atoi(argv[++i]); // filter type
            } 
            else if (COMPARE(op_name,"binarize",2)) 
            {
                op.type = OP_BINARIZE;
                op.int1 = atoi(argv[++i]); // threshold
                op.int2 = atoi(argv[++i]); // type
            } 
            else 
            {
                fprintf(stderr, "Error: operation unknown or missing arguments \n");
                return EXIT_FAILURE;
            }

            operations[op_count++] = op;
        }
    }


    if (!input_path || !output_path || op_count == 0) 
    {
        fprintf(stderr, "Error: Missing required arguments\n");
        print_help();
        free(operations);
        return EXIT_FAILURE;
    }

    BMPImage *image = read_bmp(input_path);
    if (!image) 
    {
        fprintf(stderr, "Error: Failed to read input image\n");
        free(operations);
        return EXIT_FAILURE;
    }
    
    BMPImage *result = image;
    for (int i = 0; i < op_count; i++)
    {
        //TIME_START

        Operation op = operations[i];
        switch (op.type) {
            case OP_ADD_L:
            #ifdef _SIMPLE_LIGHT_ADD_
                image_simp_light_adjust(result,op.int1);
            #else
                image_color_adjust(result,op.int1,0);
            #endif
                break;
            case OP_ADD_H:
                image_color_adjust(result,op.int1,1);
                break;
            case OP_ADD_S:
                image_color_adjust(result,op.int1,2);
                break;
            case OP_ADD_V:
                image_color_adjust(result,op.int1,3);
                break;
            case OP_BLEND: 
            {
                BMPImage *image2 = read_bmp(op.blend_image_path);
                if (!image2) 
                {
                    fprintf(stderr, "Error: Failed to read blend image\n");
                    free_bmp(result);
                    free(operations);
                    return EXIT_FAILURE;
                }
                BMPImage *blended = images_blend(result, image2,op.float1);
                if(!image2)free_bmp(image2);
                if(result != image) free_bmp(result);
                result = blended;
                break;
            }
            case OP_RESCALE:{
                BMPImage *rescaled = image_rescale(result, op.int1, op.int2);
                if(result != image) free_bmp(result);
                result = rescaled;
                break;
            }
            case OP_CUT:{ 
                BMPImage *cut = image_cut(result, op.int1, op.int2, op.int3, op.int4);
                if(result != image) free_bmp(result);
                result = cut;
                break;
            }
            case OP_ROTATE:{
                BMPImage *rotated = image_rotate(result, op.float1);
                result = rotated;
                break;
            }
            case OP_SHARP:{
                BMPImage *sharpened = {0};
                sharpened = image_sharpen(result,op.int1,op.int2);
                result = sharpened;
                break;
            }
            case OP_BLUR:{
                BMPImage *blurred = {0};
                blurred = image_G_Blur(result,op.float1,op.int1);
                result = blurred;
                break;
            }
            case OP_FILTER: 
                image_filter(result, op.int1);
                break;
            case OP_BINARIZE: 
                image_Binarization(result, op.int1, op.int2);
                break;
            case OP_FLIP: 
                image_flip(result);
                break;
        }
        //TIME_END(op.type)
    }


    write_bmp(output_path, result);
    free_bmp(image);
    free(operations);
    return EXIT_SUCCESS;
}


// 函数定义

/*
 * 将32位BMP图像转换为24位格式
 * file: 输入文件指针
 * image: 目标BMP图像结构体指针
 * 优化: 使用内存对齐分配，减少内存访问时间
 */
void cvt32_24bmp(FILE* file, BMPImage* image) 
{
    int height = image->dib_header.height;
    int width = image->dib_header.width;

    int src_row_size = image->dib_header.width * 4;
    int src_padding = (4 - (src_row_size % 4)) % 4;
    int src_actual_row_size = src_row_size + src_padding;

    int dst_actual_row_size;
    PIC_ROW_SIZE(dst_actual_row_size,image);

    //image->pixel_data = (unsigned char*)malloc(dst_actual_row_size * image->dib_header.height);
    posix_memalign((void **)&image->pixel_data, 16, dst_actual_row_size * image->dib_header.height);
    CHECK_NULL(image->pixel_data, "Memory allocation for pixel data failed");

    fseek(file, image->file_header.data_offset, SEEK_SET);
    
    unsigned char* src_row = (unsigned char*)malloc(src_actual_row_size);
    CHECK_NULL(src_row, "Temporary row allocation failed");

    for (int y = 0; y < height; y++) 
    {
        fread(src_row, src_actual_row_size, 1, file);
            for (int x = 0; x < width; x++) 
            {
            int src_pos = x * 4;
            int dst_pos = y * dst_actual_row_size + x * 3;
            
            image->pixel_data[dst_pos]     = src_row[src_pos];     // B
            image->pixel_data[dst_pos + 1] = src_row[src_pos + 1]; // G
            image->pixel_data[dst_pos + 2] = src_row[src_pos + 2]; // R
        }
    }

    // Update headers to 24-bit format
    image->dib_header.bits_per_pixel = 24;
    image->dib_header.header_size = 40;
    image->dib_header.compression = 0;
    image->dib_header.image_size = dst_actual_row_size * image->dib_header.height;
    image->file_header.file_size = sizeof(BMPHeader) + sizeof(DIBHeader) + image->dib_header.image_size;
    image->file_header.data_offset = sizeof(BMPHeader) + sizeof(DIBHeader);

    free(src_row);
}

/*
 * 将8位BMP图像转换为24位格式
 * file: 输入文件指针
 * image: 目标BMP图像结构体指针
 * 优化: 使用调色板转换，内存对齐分配
 */
void cvt8_24bmp(FILE* file, BMPImage* image) 
{
    int width = image->dib_header.width;
    int height = image->dib_header.height;

    int dst_actual_row_size;
    PIC_ROW_SIZE(dst_actual_row_size,image);

    //image->pixel_data = (unsigned char*)malloc(dst_actual_row_size * height);
    posix_memalign((void **)&image->pixel_data, 16, dst_actual_row_size * height);
    CHECK_NULL(image->pixel_data, "Memory allocation for pixel data failed");

    fseek(file, 54, SEEK_SET);
    unsigned char* palette = (unsigned char*)malloc(1024);
    CHECK_NULL(palette, "Temporary palette allocation failed");
    fread(palette, 1, 1024, file);

    fseek(file, image->file_header.data_offset, SEEK_SET);
    unsigned char* src_row = (unsigned char*)malloc(width);
    CHECK_NULL(src_row, "Temporary row allocation failed");
    
    for (int y = 0; y < height; y++) 
    {
        fread(src_row, 1, width, file);
        for (int x = 0; x < width; x++) 
        {
            int dst_pos = y * dst_actual_row_size + x * 3;
            unsigned char idx = src_row[x] * 4;
            
            image->pixel_data[dst_pos]     = palette[idx];    // B
            image->pixel_data[dst_pos + 1] = palette[idx+1];  // G
            image->pixel_data[dst_pos + 2] = palette[idx+2];  // R
        }
    }

    // Update headers to 24-bit format
    image->dib_header.bits_per_pixel = 24;
    image->dib_header.header_size = 40;
    image->dib_header.compression = 0;
    image->dib_header.image_size = dst_actual_row_size * height;
    image->file_header.file_size = sizeof(BMPHeader) + sizeof(DIBHeader) + image->dib_header.image_size;
    image->file_header.data_offset = sizeof(BMPHeader) + sizeof(DIBHeader);

    free(src_row);
    free(palette);
}

/*
 * 读取BMP文件并转换为统一24位格式
 * filename: 输入文件名
 * 返回: BMP图像结构体指针
 * 优化: 支持8/24/32位自动转换，内存对齐分配
 */
BMPImage* read_bmp(const char* filename) 
{
    FILE* file = fopen(filename, "rb");
    CHECK_FILE(file, filename, "Could not open file");

    BMPImage* image = (BMPImage*)malloc(sizeof(BMPImage));
    CHECK_NULL(image, "Memory allocation failed");

    // Read headers
    fread(&image->file_header, sizeof(BMPHeader), 1, file);
    fread(&image->dib_header, sizeof(DIBHeader), 1, file);

    // Verify BMP format
    if (image->file_header.signature[0] != 'B' || image->file_header.signature[1] != 'M')
    {
        fprintf(stderr, "Error: Not a valid BMP file\n");
        fclose(file);
        free(image);
        exit(EXIT_FAILURE);
    }
    int actual_row_size;
    // If convert is needed
    short bpp = image->dib_header.bits_per_pixel;
    switch (bpp)
    {
    case 24:
        PIC_ROW_SIZE(actual_row_size,image);
        //image->pixel_data = (unsigned char*)malloc(actual_row_size * image->dib_header.height);
        posix_memalign((void **)&image->pixel_data, 16, actual_row_size * image->dib_header.height);
        CHECK_NULL(image->pixel_data, "Memory allocation for pixel data failed");
        fseek(file, image->file_header.data_offset, SEEK_SET);
        fread(image->pixel_data, actual_row_size * image->dib_header.height, 1, file);
        break;
    case 32:
        cvt32_24bmp(file, image);
        break;
    case 8:
        cvt8_24bmp(file, image);
        break;
    default:
        fprintf(stderr, "Error: Only 8/24/32-bit BMP files are supported\n");
        fclose(file);
        free(image);
        exit(EXIT_FAILURE);
        break;
    }
    
    fclose(file);
    return image;
}

/*
 * 将BMP图像写入文件
 * filename: 输出文件名
 * image: 要写入的BMP图像
 */
void write_bmp(const char* filename, BMPImage* image) 
{
   
    CHECK_NULL(image,"Error: Failed to write out image")

    FILE* file = fopen(filename, "wb");
    CHECK_FILE(file, filename, "Could not create output file");

    // Write headers
    fwrite(&image->file_header, sizeof(BMPHeader), 1, file);
    fwrite(&image->dib_header, sizeof(DIBHeader), 1, file);

    // Calculate row size with padding
    int row_size = image->dib_header.width * 3;
    int padding = (4 - (row_size % 4)) % 4;
    row_size += padding;

    // Write pixel data
    fseek(file, image->file_header.data_offset, SEEK_SET);
    fwrite(image->pixel_data, row_size * image->dib_header.height, 1, file);

    fclose(file);
}
 
/*
 * 释放BMP图像内存
 * image: 要释放的BMP图像
 */
void free_bmp(BMPImage* image) 
{
    if (image != NULL) {
        free(image->pixel_data);
        free(image);
    }
}

/*
 * 创建新BMP图像(空白)
 * ref_img: 参考图像(复制头信息)
 * new_width: 新图像宽度
 * new_height: 新图像高度
 * 返回: 新BMP图像指针
 * 优化: 内存对齐分配
 */
BMPImage* create_new_image(BMPImage* ref_img,size_t new_width,size_t new_height) 
{
    CHECK_NULL(ref_img,"Error: NULL image can't be processed")

    BMPImage* new_img = (BMPImage*)malloc(sizeof(BMPImage));
    CHECK_NULL(new_img, "Memory allocation failed");

    // Copy headers from first image
    memcpy(&new_img->file_header, &ref_img->file_header, sizeof(BMPHeader));
    memcpy(&new_img->dib_header, &ref_img->dib_header, sizeof(DIBHeader));

    // Set to new height and width
    new_img->dib_header.width = new_width;
    new_img->dib_header.height = new_height;

    // Calculate new row size with padding
    int new_actual_row_size;
    PIC_ROW_SIZE(new_actual_row_size,new_img);

    // Allocate memory for new pixel data (initialized to black)
    //new_img->pixel_data = (unsigned char*)calloc(new_actual_row_size * new_height, 1);
    posix_memalign((void **)&new_img->pixel_data, 16, new_actual_row_size * new_height);
    CHECK_NULL(new_img->pixel_data, "Memory allocation for new image failed");

    // Update file size in header
    new_img->file_header.file_size = sizeof(BMPHeader) + sizeof(DIBHeader) + (new_actual_row_size * new_height);
    new_img->dib_header.image_size = new_actual_row_size * new_height;

    return new_img;
}

/*
 * RGB颜色空间转HSV颜色空间
 * R,G,B: 输入RGB值(0-255)
 * H,S,V: 输出HSV值(H:0-360,S/V:0-1)
 */
void cvtRGB_HSV(int* R, int* G, int* B, float* H, float* S, float* V) 
{
    //if (!R || !G || !B || !H || !S || !V) return;

    float std_R = (float)*R / 255;
    float std_G = (float)*G / 255;
    float std_B = (float)*B / 255;

    float C_max = fmaxf(std_R, fmaxf(std_G, std_B));
    float C_min = fminf(std_R, fminf(std_G, std_B));
    float delta = C_max - C_min;

    if (delta < FLT_EPSILON) {
        *H = 0.0f;
    } else {
        if (fabs(C_max - std_R) < FLT_EPSILON) {
            *H = 60 * (std_G - std_B) / delta;
        } else if (fabs(C_max - std_G) < FLT_EPSILON) {
            *H = 60 * (std_B - std_R) / delta + 120;
        } else if (fabs(C_max - std_B) < FLT_EPSILON) {
            *H = 60 * (std_R - std_G) / delta + 240;
        }
        if (*H < 0) *H += 360;
    }

    *V = C_max;
    *S = (C_max < FLT_EPSILON) ? 0 : (delta / C_max);
}

/*
 * HSV颜色空间转RGB颜色空间
 * H,S,V: 输入HSV值(H:0-360,S/V:0-1)
 * R,G,B: 输出RGB值(0-255)
 */
void cvtHSV_RGB(float* H, float* S, float* V,int* R, int* G, int* B) 
{
    //if (!R || !G || !B ||!H || !S || !V) return;

    *H = fmodf(*H, 360.0f);
    if (*H < 0) *H += 360.0f;
    *S = fmaxf(0.0f, fminf(1.0f, *S));
    *V = fmaxf(0.0f, fminf(1.0f, *V));

    float C = (*V) * (*S);
    float X = C * (1 - fabsf(fmodf(*H / 60.0f, 2) - 1));
    float m = *V - C;

    float r, g, b;
    switch ((int)(*H / 60.0f)) 
    {
        case 0:  r = C; g = X; b = 0; break;
        case 1:  r = X; g = C; b = 0; break;
        case 2:  r = 0; g = C; b = X; break;
        case 3:  r = 0; g = X; b = C; break;
        case 4:  r = X; g = 0; b = C; break;
        case 5:  r = C; g = 0; b = X; break;
        default: r = g = b = 0; break;
    }

    *R = (int)((r + m) * 255 + 0.5f);
    *G = (int)((g + m) * 255 + 0.5f);
    *B = (int)((b + m) * 255 + 0.5f);

    *R = (int)fmaxf(0, fminf(255, *R));
    *G = (int)fmaxf(0, fminf(255, *G));
    *B = (int)fmaxf(0, fminf(255, *B));
}

/*
 * 简单亮度调整(使用NEON指令优化)
 * image: 要处理的BMP图像
 * L_val: 亮度调整值(-255到255)
 * 优化: NEON SIMD指令并行处理，OpenMP多线程
 * light_add light_sub get_light_func 分别为要内联的函数和映射函数的函数
 */
inline void light_add(uint8x16_t* v_pixels, uint8x16_t v_offset){
    *v_pixels = vqaddq_u8(*v_pixels, v_offset);
}
inline void light_sub(uint8x16_t* v_pixels, uint8x16_t v_offset){
    *v_pixels = vqsubq_u8(*v_pixels, v_offset);
}
LightFunc get_light_func(int type){
    switch (type)
    {
        case 0: return light_add;
        case 1: return light_sub;
        default: return light_add;
    }
}
void image_simp_light_adjust(BMPImage* image, int L_val) 
{
    CHECK_NULL(image, "Error: NULL image can't be processed");

    int row_size = image->dib_header.width * 3;
    int height_size = image->dib_header.height;
    int actual_row_size;
    PIC_ROW_SIZE(actual_row_size, image);

    L_val = (L_val < -255 ? -255 : (L_val > 255 ? 255 : L_val));
    uint8_t abs_L = (uint8_t)(L_val < 0 ? -L_val : L_val);
    LightFunc Func = get_light_func(L_val >= 0 ? 0 : 1);

    #pragma omp parallel for schedule(guided)
    for (int y = 0; y < height_size; y++) 
    {
        unsigned char* cur_row = image->pixel_data + y * actual_row_size;
        int x = 0;
#ifdef _NEON_
            uint8x16_t v_offset = vdupq_n_u8(abs_L);
            for (; x + 15 < row_size; x += 16) {
                uint8x16_t v_pixels = vld1q_u8(cur_row + x);
                Func(&v_pixels,v_offset);
                vst1q_u8(cur_row + x, v_pixels);
            }
#endif  
        for (; x < row_size; x++) {
            int new_val = cur_row[x] + L_val;
            cur_row[x] = (unsigned char)(new_val < 0 ? 0 : (new_val > 255 ? 255 : new_val));

        }
    }
}

/*
 * 图像颜色调整(亮度/色调/饱和度/明度)
 * image: 要处理的BMP图像
 * val: 调整值(亮度:-100-100, 色调:任意, 饱和度/明度:-100-100)
 * type: 调整类型(0:亮度, 1:色调, 2:饱和度, 3:明度)
 * 优化: 使用HSV颜色空间转换保持色彩准确性
 * adjust_* 均为要根据函数指针调用的内联小函数
 */
inline void adjust_L(unsigned char* pixel, float delt, float offset) {
    pixel[0] = (unsigned char)(pixel[0] * delt + offset);
    pixel[1] = (unsigned char)(pixel[1] * delt + offset);
    pixel[2] = (unsigned char)(pixel[2] * delt + offset);
}
inline void adjust_H(unsigned char* pixel, float H_v, float none) {
    float H, S, V;
    int R = pixel[2], G = pixel[1], B = pixel[0];

    cvtRGB_HSV(&R, &G, &B, &H, &S, &V);
    H += H_v;
    cvtHSV_RGB(&H, &S, &V, &R, &G, &B);

    pixel[0] = (unsigned char)B;
    pixel[1] = (unsigned char)G;
    pixel[2] = (unsigned char)R;
}
inline void adjust_S(unsigned char* pixel, float S_v, float none) {
    float H, S, V;
    int R = pixel[2], G = pixel[1], B = pixel[0];

    cvtRGB_HSV(&R, &G, &B, &H, &S, &V);
    S += S_v;
    cvtHSV_RGB(&H, &S, &V, &R, &G, &B);

    pixel[0] = (unsigned char)B;
    pixel[1] = (unsigned char)G;
    pixel[2] = (unsigned char)R;
}
inline void adjust_V(unsigned char* pixel, float V_v, float none) {
    float H, S, V;
    int R = pixel[2], G = pixel[1], B = pixel[0];

    cvtRGB_HSV(&R, &G, &B, &H, &S, &V);
    V += V_v;
    cvtHSV_RGB(&H, &S, &V, &R, &G, &B);

    pixel[0] = (unsigned char)B;
    pixel[1] = (unsigned char)G;
    pixel[2] = (unsigned char)R;
}
inline void adjust_NONE(unsigned char* pixel, float val, float none) {
    ;
}
AdjustFunc get_adjust_func(int type) 
{
    switch(type) 
    {
        case 0: return adjust_L;
        case 1: return adjust_H;
        case 2: return adjust_S;
        case 3: return adjust_V;
        default: return adjust_NONE;
    }
}
void image_color_adjust(BMPImage* image, int val, size_t type) {

    CHECK_NULL(image, "Error: NULL image can't be processed");

    int row_size = image->dib_header.width;
    int col_size = image->dib_header.height;
    int actual_row_size;
    PIC_ROW_SIZE(actual_row_size, image);

    float param1 = .0f;
    float param2 = .0f;

    float alpha = .0f;
    switch (type) 
    {
        case 0:
            val = val > 100 ? 100 : val < -100 ? -100 : val;
            alpha = (float)val / 100;
            param1 = alpha > 0 ? 1 - alpha : 1 + alpha;
            param2 = alpha > 0 ? alpha * 255 : 0;
            break;
        case 1:
            param1 = val % 360;
            break;
        default:
            param1 = (float)val / 100;
            break;
    }

    AdjustFunc func = get_adjust_func(type);

    #pragma omp parallel for schedule(guided)
    for (int y = 0; y < col_size; y++) {
        unsigned char* cur_row = image->pixel_data + y * actual_row_size;
        for (int x = 0; x < row_size; x++) {
            func(&cur_row[x * 3],param1,param2);
        }
    }
}

/*
 * 应用颜色矩阵滤镜
 * image: 要处理的BMP图像
 * type: 滤镜类型(0-3)
 */
void image_filter(BMPImage* image,int type) 
{
    CHECK_NULL(image,"Error: NULL image can't be processed")

    if(type > (int)sizeof(FILTER_MATRICES)/sizeof(float*) || type < 0) type = 0;
    float* matrix = FILTER_MATRICES[type];

    int row_size = image->dib_header.width;
    int height_size = image->dib_header.height;
    int actual_row_size;
    PIC_ROW_SIZE(actual_row_size,image);

    #pragma omp parallel for schedule(guided)
    for (int y = 0; y < height_size; y++) 
    {
        unsigned char* cur_row = image->pixel_data + y * actual_row_size;
        for (int x = 0; x < row_size; x++)
        {
            int idx = x*3;
            int R = 0,G = 0,B = 0,M = 1;
            B = cur_row[idx];
            G = cur_row[idx+1];
            R = cur_row[idx+2];

            float new_R = matrix[0] * R + matrix[1] * G + matrix[2] * B + matrix[3] * M;
            float new_G = matrix[4] * R + matrix[5] * G + matrix[6] * B + matrix[7] * M;
            float new_B = matrix[8] * R + matrix[9] * G + matrix[10] * B + matrix[11] * M;

            cur_row[idx]   = (unsigned char)fmax(0, fmin(255, new_B));
            cur_row[idx+1] = (unsigned char)fmax(0, fmin(255, new_G));
            cur_row[idx+2] = (unsigned char)fmax(0, fmin(255, new_R));
        }
    }
 }

/*
* 图像二值化处理
* image: 要处理的BMP图像
* threshold: 阈值(0-255)
* type: 二值化类型(0-4)
* 前面的为要内联的函数和得到映射内联函数的函数
*/
inline void binarize_norm(unsigned char* pixel, unsigned char threshold) {
    int gray = 0.299f * pixel[2] + 0.587f * pixel[1] + 0.114f * pixel[0];
    unsigned char value = (gray > threshold) ? 255 : 0;
    pixel[0] = pixel[1] = pixel[2] = value;
}
inline void binarize_inv(unsigned char* pixel, unsigned char threshold) {
    int gray = 0.299f * pixel[2] + 0.587f * pixel[1] + 0.114f * pixel[0];
    unsigned char value = (gray > threshold) ? 0 : 255;
    pixel[0] = pixel[1] = pixel[2] = value;
}
inline void binarize_trunc(unsigned char* pixel, unsigned char threshold) {
    int gray = 0.299f * pixel[2] + 0.587f * pixel[1] + 0.114f * pixel[0];
    unsigned char value = (gray > threshold) ? threshold : gray;
    pixel[0] = pixel[1] = pixel[2] = value;
}
inline void binarize_tozero(unsigned char* pixel, unsigned char threshold) {
    int gray = 0.299f * pixel[2] + 0.587f * pixel[1] + 0.114f * pixel[0];
    unsigned char value = (gray > threshold) ? 0 : gray;
    pixel[0] = pixel[1] = pixel[2] = value;
}
inline void binarize_tozero_inv(unsigned char* pixel, unsigned char threshold) {
    int gray = 0.299f * pixel[2] + 0.587f * pixel[1] + 0.114f * pixel[0];
    unsigned char value = (gray > threshold) ? gray : 0;
    pixel[0] = pixel[1] = pixel[2] = value;
}
BinarizeFunc get_binarize_func(int type) 
{
    switch(type) 
    {
        case 0: return binarize_norm;
        case 1: return binarize_inv;
        case 2: return binarize_trunc;
        case 3: return binarize_tozero;
        case 4: return binarize_tozero_inv;
        default: return binarize_norm;
    }
}
void image_Binarization(BMPImage* image,unsigned char threshold,int type) 
{
    CHECK_NULL(image,"Error: NULL image can't be processed")

    int row_size = image->dib_header.width;
    int height_size = image->dib_header.height;
    int actual_row_size;
    PIC_ROW_SIZE(actual_row_size,image);

    BinarizeFunc bin_ptr = get_binarize_func(type);

    #pragma omp parallel for schedule(guided)
    for (int y = 0; y < height_size; y++) 
    {
        unsigned char* cur_row = image->pixel_data + y * actual_row_size;
        for (int x = 0; x < row_size; x++)
        {
            int idx = x*3;
            bin_ptr(&cur_row[idx], threshold);      
        }
    }
}

/*
 * 双线性插值函数(处理3通道)
 * p1,p2: 源像素行指针
 * p3: 目标像素指针
 * dx,dy: 插值系数
 * _x1,_x2: 源像素位置
 */
inline void bilinearVec3(const unsigned char * p1, const unsigned char * p2, unsigned char * p3, float dx, float dy, int _x1, int _x2)
{
    for(int i = 0 ; i < 3 ; i++){
        int t1 = (float)p1[_x1+i] + dx * (p1[_x2+i] - p1[_x1+i]);
        int t2 = (float)p2[_x1+i] + dx * (p2[_x2+i] - p2[_x1+i]);
        p3[i] = t1 + dy * (t2 - t1);
    }
}

/*
 * 图像放大处理
 * image: 源图像
 * new_width: 新宽度
 * new_height: 新高度
 * 返回: 缩放后的新图像
 * 优化:双线性插值
 */
BMPImage* image_scale_larger(BMPImage* image,size_t new_width,size_t new_height)
{
    CHECK_NULL(image,"Error: NULL image can't be processed")

    /** 计算实际长度和高度 */
    int old_width = image->dib_header.width;
    int old_height = image->dib_header.height;
    int actual_row_size;
    PIC_ROW_SIZE(actual_row_size,image);  

    BMPImage* new_image = create_new_image(image,new_width,new_height);
    
    /** 计算新长度和高度 */
    int new_actual_row_size;
    PIC_ROW_SIZE(new_actual_row_size,new_image);  

    float ratio_x = (float)old_width / new_width;
    float ratio_y = (float)old_height / new_height;
    
    /* 对新的图幅遍历每个像素点 */
    int max_threads = omp_get_max_threads();
    #pragma omp parallel for num_threads(max_threads)schedule(guided)
    for (int y = 0; y < new_height; y++) {

        float _y = y * ratio_y;
        int y1 = (int)_y;
        int y2 = y1+1;
        if(y2 >= old_height) y2 = old_height-1;

        unsigned char* cur_y1_row = image->pixel_data + y1 * actual_row_size;//old row1 position
        unsigned char* cur_y2_row = image->pixel_data + y2 * actual_row_size;//old row2 position
        unsigned char* cur_new_row = new_image->pixel_data + y * new_actual_row_size;//new row position

        for (int x = 0; x < new_width; x++) {

            float _x = x * ratio_x;
            int x1 = (int)_x;
            int x2 = x1+1;
            if(x2 >= old_width) x2 = old_width-1;

            float offset_x = _x - x1;
            float offset_y = _y - y1;

            bilinearVec3(cur_y1_row,cur_y2_row,&cur_new_row[x*3],offset_x,offset_y,x1*3,x2*3);
        }
    }

    return new_image;
}

/*
 * 图像缩小处理
 * image: 源图像
 * new_width: 新宽度
 * new_height: 新高度
 * 返回: 缩放后的新图像
 * 优化: 区域平均采样
 */
BMPImage* image_scale_smaller(BMPImage* image, size_t new_width, size_t new_height) 
{
    CHECK_NULL(image,"Error: NULL image can't be processed")

    int old_width = image->dib_header.width;
    int old_height = image->dib_header.height;

    BMPImage* new_image = create_new_image(image, new_width, new_height);
    int old_actual_row_size;
    PIC_ROW_SIZE(old_actual_row_size,image);
    int new_actual_row_size;
    PIC_ROW_SIZE(new_actual_row_size,new_image);

    float scale_x = (float)old_width / new_width;
    float scale_y = (float)old_height / new_height;

    
    int max_threads = omp_get_max_threads();
    #pragma omp parallel for num_threads(max_threads) schedule(guided)
    for (int y = 0; y < new_height; y++) 
    {
        unsigned char* new_row = new_image->pixel_data + y * new_actual_row_size;
        for (int x = 0; x < new_width; x++) 
        {
            float center_x = x * scale_x + (scale_x - 1) * 0.5f;
            float center_y = y * scale_y + (scale_y - 1) * 0.5f;

            int x1 = (int)(center_x - scale_x * 0.5f);
            int x2 = (int)(center_x + scale_x * 0.5f);
            int y1 = (int)(center_y - scale_y * 0.5f);
            int y2 = (int)(center_y + scale_y * 0.5f);

            x1 = (x1 < 0) ? 0 : x1;
            x2 = (x2 >= old_width) ? old_width - 1 : x2;
            y1 = (y1 < 0) ? 0 : y1;
            y2 = (y2 >= old_height) ? old_height - 1 : y2;
    
            int sum_b = 0, sum_g = 0, sum_r = 0;
            int total_weight = 0;
            
            for (int _y = y1; _y <= y2; _y++) {
                unsigned char* old_row = image->pixel_data + _y * old_actual_row_size;
                for (int _x = x1; _x <= x2; _x++) {
                    sum_b += old_row[_x * 3];
                    sum_g += old_row[_x * 3 + 1];
                    sum_r += old_row[_x * 3 + 2];
                    total_weight++;
                }
            }

            new_row[x * 3] = (unsigned char)(sum_b / total_weight);
            new_row[x * 3 + 1] = (unsigned char)(sum_g / total_weight);
            new_row[x * 3 + 2] = (unsigned char)(sum_r / total_weight);
        }
    }

    return new_image;
}

/*
 * 图像缩放主函数
 * image: 源图像
 * new_width: 新宽度
 * new_height: 新高度
 * 返回: 缩放后的新图像
 * 优化: 自动选择放大或缩小算法
 */
BMPImage* image_rescale(BMPImage* image, size_t new_width, size_t new_height)
{
    CHECK_NULL(image,"Error: NULL image can't be processed")

    int old_width = image->dib_header.width;
    int old_height = image->dib_header.height;

    BMPImage* result = NULL;

    //纯缩小
    if(old_width >= new_width && old_height >= new_height){
        result = image_scale_smaller(image,new_width,new_height);
    //纯放大
    }else if(old_width < new_width && old_height < new_height){
        result = image_scale_larger(image,new_width,new_height);
    }else if(old_width >= new_width && old_height < new_height){
        BMPImage* tempt = image_scale_smaller(image,new_width,old_height);
        result = image_scale_larger(tempt,new_width,new_height);
        free(tempt);
    }else if(old_width < new_width && old_height >= new_height){
        BMPImage* tempt = image_scale_smaller(image,old_width,new_height);
        result = image_scale_larger(tempt,new_width,new_height);
        free(tempt);
    }

    return result;
}

/*
 * 图像裁剪
 * image: 源图像
 * offset_x/y: 起始坐标
 * new_width/height: 新尺寸
 * 返回: 裁剪后的新图像
 * 优化: 内存拷贝优化
 */
BMPImage* image_cut(BMPImage* image, size_t offset_x, size_t offset_y, size_t new_width, size_t new_height){
    
    CHECK_NULL(image,"Error: NULL image can't be processed")

    /** 计算实际长度和高度 */
    int old_width = image->dib_header.width;
    int old_height = image->dib_header.height;
    int actual_row_size;
    PIC_ROW_SIZE(actual_row_size,image);

    if(offset_x >= old_width || offset_y >= old_height)
    {
        printf("Offset too big for image cutting\n");
        return NULL;
    }

    if(new_width  > old_width  - offset_x) new_width  = old_width  - offset_x;
    if(new_height > old_height - offset_y) new_height = old_height - offset_y;

    BMPImage* new_image = create_new_image(image,new_width,new_height);
    CHECK_NULL(image,"Error: NULL image can't be processed")

    int new_row_size = new_image->dib_header.width*3;
    int new_actual_row_size;
    PIC_ROW_SIZE(new_actual_row_size,new_image);

    // unsigned char* cur_ref_row = image->pixel_data + offset_y * actual_row_size;
    // unsigned char* cur_new_row = new_image->pixel_data;
    // 
    // for (int y = 0; y < new_height; y++) {
    //     cur_ref_row += y * actual_row_size;
    //     cur_new_row += y * new_actual_row_size;
    //     for (int x = offset_x,_x = 0; x < offset_x + new_width; x++,_x++) {
    //         int _idx = _x*3;
    //         int idx = x*3;
    //         cur_new_row[_idx  ] = cur_ref_row[idx  ];
    //         cur_new_row[_idx+1] = cur_ref_row[idx+1];
    //         cur_new_row[_idx+2] = cur_ref_row[idx+2];
    //     }
    // }
    unsigned char* src_data = image->pixel_data + offset_y * actual_row_size + offset_x * 3;
    unsigned char* dst_data = new_image->pixel_data;
    for (int y = 0; y < new_height; y++) {
        memcpy(dst_data + y * new_actual_row_size, src_data + y * actual_row_size,new_row_size);
    }

    return new_image;
}

/*
 * 数组混合函数(支持NEON优化)
 * p1: 主数组(会被修改)
 * p2: 次数组
 * ratio: 混合比例(0-1)
 * num: 元素数量
 */
inline void mix_arrays(uint8_t *p1, const uint8_t *p2, float ratio, size_t num) {
    size_t i = 0;
    float inv_ratio = 1 - ratio;
#if defined(_NEON_)
    if(fabs(ratio-0.5)<FLT_EPSILON){
        for (; i + 15 < num; i += 16) {
            uint8x16_t v_p1 = vld1q_u8(p1 + i);
            uint8x16_t v_p2 = vld1q_u8(p2 + i);
            uint8x16_t v_result = vrhaddq_u8(v_p1, v_p2);

            vst1q_u8(p1 + i, v_result);
        }
    }
#endif
    for (; i < num; i++) {
        p1[i] = p1[i] * inv_ratio + p2[i] * ratio;
    }
}

/*
 * 图像混合
 * image1: 主图像
 * image2: 次图像
 * ratio: 混合比例(0-1)
 * 返回: 混合后的图像
 * 优化: 自动尺寸调整，NEON加速混合
 */
BMPImage* images_blend(BMPImage* image1, BMPImage* image2, float ratio) 
{
    CHECK_NULL(image1,"Error: NULL image can't be processed")
    CHECK_NULL(image2,"Error: NULL image can't be processed")

    if(ratio < 0) ratio = 0;
    if(ratio > 1) ratio = 1;

    int w1 = image1->dib_header.width;
    int w2 = image2->dib_header.width;
    int h1 = image1->dib_header.height;
    int h2 = image2->dib_header.height;
    int max_w = w1 > w2 ? w1 : w2;
    int max_h = h1 > h2 ? h1 : h2;

    BMPImage* image1_r;
    BMPImage* image2_r;

    if(w1==w2 && h1==h2){
        image1_r = image1;
        image2_r = image2;
    }else{
        if(max_h == h1 && max_w == w1){
            image1_r = image1;
            image2_r = image_rescale(image2,max_w,max_h);
        }
        else if(max_h == h2 && max_w == w2){
            image2_r = image2;
            image1_r = image_rescale(image1,max_w,max_h);
        }else{
            image1_r = image_rescale(image1, max_w, max_h);
            image2_r = image_rescale(image2, max_w, max_h);
        }
    }
    
    if (!image1_r || !image2_r) {
        if (image1_r) free_bmp(image1_r);
        if (image2_r) free_bmp(image2_r);
        return NULL;
    }
    
    int actual_row_size;
    PIC_ROW_SIZE(actual_row_size,image1_r);
    int height = image1_r->dib_header.height;
    int width = image1_r->dib_header.width;
    int row_size = width * 3;
     
    unsigned char* row1 = image1_r->pixel_data;
    unsigned char* row2 = image2_r->pixel_data;

    #pragma omp parallel for schedule(guided)
    for (int y = 0; y < height; y++) {
        int offset = y * actual_row_size;
        mix_arrays(&row1[offset],&row2[offset],ratio,row_size);
    }
    if(!image2)free_bmp(image2_r);
    return image1_r;
}
 
/*
 * 图像旋转
 * image: 源图像
 * angle: 旋转角度(度)
 * 返回: 旋转后的新图像
 * 优化: 双线性插值保持质量
 */
BMPImage* image_rotate(BMPImage* image, float angle) 
{
    CHECK_NULL(image,"Error: NULL image can't be processed")

    angle = fmod(angle, 360.0f);
    float radians = angle * (PI / 180.0f);

    int old_width = image->dib_header.width;
    int old_height = image->dib_header.height;
    int old_actual_row_size;
    PIC_ROW_SIZE(old_actual_row_size,image)

    float sin_abs = fabs(sin(radians));
    float cos_abs = fabs(cos(radians));
    float sin_val = sin(radians);
    float cos_val = cos(radians);

    int new_width = (int)(old_width * cos_abs + old_height * sin_abs);
    int new_height = (int)(old_height * cos_abs + old_width * sin_abs);
    BMPImage* new_image = create_new_image(image, new_width, new_height);

    int new_actual_row_size;
    PIC_ROW_SIZE(new_actual_row_size,new_image);

    float center_x_old = old_width / 2.0f;
    float center_y_old = old_height / 2.0f;
    float center_x_new = new_width / 2.0f;
    float center_y_new = new_height / 2.0f;

    unsigned char* old_row = image->pixel_data;
    #pragma omp parallel for schedule(guided)
    for (int y = 0; y < new_height; y++) 
    {
        unsigned char* new_row = new_image->pixel_data + y * new_actual_row_size;
        float y_new = y - center_y_new;
        for (int x = 0; x < new_width; x++) 
        {
            float x_new = x - center_x_new;
            
            float x_old = x_new * cos_val + y_new * sin_val + center_x_old;
            float y_old = -x_new * sin_val + y_new * cos_val + center_y_old;

            int x0 = (int)x_old;
            int y0 = (int)y_old;
            int x1 = x0 + 1;
            int y1 = y0 + 1;

            if (x0 < 0 || x1 >= old_width || y0 < 0 || y1 >= old_height) {
                continue;
            }

            float dx = x_old - x0;
            float dy = y_old - y0;
            
            bilinearVec3(&old_row[y0*old_actual_row_size],&old_row[y1*old_actual_row_size],&new_row[x*3],dx,dy,x0 * 3,x1 * 3);
        }
    }

    return new_image;
}

/*
 * 图像水平翻转
 * image: 要处理的图像
 */
void image_flip(BMPImage* image) {

    CHECK_NULL(image,"Error: NULL image can't be processed")

    int row_size = image->dib_header.width*3;
    int height = image->dib_header.height;
    int actual_row_size;
    PIC_ROW_SIZE(actual_row_size,image);

    
    int _start = row_size-3;
    int _end = row_size/2;
    #pragma omp parallel for schedule(guided)
    for (int y = 0; y < height; y++) {
        unsigned char* cur_row = image->pixel_data + y * actual_row_size;
        for (int x = 0,_x = _start; x < _end; x+=3,_x-=3) {
            unsigned char x1 = cur_row[x];
            unsigned char x2 = cur_row[x+1];
            unsigned char x3 = cur_row[x+2];
            cur_row[x] = cur_row[_x];
            cur_row[x+1] = cur_row[_x+1];
            cur_row[x+2] = cur_row[_x+2];
            cur_row[_x] = x1;
            cur_row[_x+1] = x2;
            cur_row[_x+2] = x3;
        }
    }
}

/*
 * 生成一维高斯核
 * kernel: 输出核数组
 * size: 核大小
 */
void generate_G_kernel_1D(float* kernel, int size) {
    const int center = size / 2;
    float sigma =  0.3f * ((size - 1) * 0.5f - 1) + 0.8f;
    float sum = 0.0f;
    
    for (int i = 0; i < size; ++i) {
        int x = i - center;
        kernel[i] = expf(-(x * x) / (2 * sigma * sigma));
        sum += kernel[i];
    }
    for (int i = 0; i < size; ++i) kernel[i] /= sum;
}

/*
 * 高斯模糊处理
 * image: 源图像
 * scale: 模糊程度(0-1)
 * iter: 迭代次数
 * 返回: 模糊后的图像
 * 优化: 分离式高斯模糊
 */
BMPImage* image_G_Blur(BMPImage* image, float scale, size_t iter)
{

    CHECK_NULL(image,"Error: NULL image can't be processed")
    if(scale < 0) scale = 0;
    if(scale > 1) scale = 1;

    int size = (int)(scale * 20) * 2 + 1;// f : {0~1} -> {1,3,5,...,41}
    int range = size / 2;
    float * kernel_1D = malloc(size*sizeof(float));
    generate_G_kernel_1D(kernel_1D,size);

    BMPImage* result = image_rescale(image,image->dib_header.width,image->dib_header.height);//a copy of the pic
    CHECK_NULL(result,"Error: NULL image can't be processed")

    int old_width = image->dib_header.width;
    int old_height = image->dib_header.height;
    int actual_row_size;
    PIC_ROW_SIZE(actual_row_size,image);

    unsigned char* cur_row = image->pixel_data;
    unsigned char* new_row = result->pixel_data;

    for(size_t i = 0 ; i < iter ; i++){
        #pragma omp parallel for schedule(guided)
        for (int y = 0; y < old_height; y++) {
            for (int x = 0; x < old_width; x++) {
                float sum_B = 0, sum_G = 0, sum_R = 0;

                for (int k = -range; k <= range; k++) {
                    int xk = x + k;
                    if (xk < 0) xk = 0;
                    if (xk >= old_width) xk = old_width - 1;

                    int idx = y * actual_row_size + xk * 3;
                    float w = kernel_1D[k + range];

                    sum_B += cur_row[idx] * w;
                    sum_G += cur_row[idx + 1] * w;
                    sum_R += cur_row[idx + 2] * w;
                }

                int _idx = y * actual_row_size + x * 3;
                new_row[_idx] = fminf(255.0f, fmaxf(0.0f, sum_B));
                new_row[_idx + 1] = fminf(255.0f, fmaxf(0.0f, sum_G));
                new_row[_idx + 2] = fminf(255.0f, fmaxf(0.0f, sum_R));
            }
        }

        #pragma omp parallel for schedule(guided)
        for (int x = 0; x < old_width; x++) { 
            for (int y = 0; y < old_height; y++) {
                float sum_B = 0, sum_G = 0, sum_R = 0;

                for (int k = -range; k <= range; k++) {
                    int yk = y + k;
                    if (yk < 0) yk = 0;
                    if (yk >= old_height) yk = old_height - 1;

                    int idx = yk * actual_row_size + x * 3;
                    float w = kernel_1D[k + range];

                    sum_B += new_row[idx] * w;
                    sum_G += new_row[idx + 1] * w;
                    sum_R += new_row[idx + 2] * w;
                }

                int _idx = y * actual_row_size + x * 3;
                new_row[_idx] = fminf(255.0f, fmaxf(0.0f, sum_B));
                new_row[_idx + 1] = fminf(255.0f, fmaxf(0.0f, sum_G));
                new_row[_idx + 2] = fminf(255.0f, fmaxf(0.0f, sum_R));
            }
        }

    }

    return result;
}

/*
 * 图像锐化处理
 * image: 源图像
 * type: 锐化类型(0-3)
 * iter: 迭代次数
 * 返回: 锐化后的图像
 */
BMPImage* image_sharpen(BMPImage* image, int type, size_t iter){

    CHECK_NULL(image,"Error: NULL image can't be processed")

    int size = 3; 
    int range = size / 2;
    if(type>= sizeof(KERNALS) / sizeof(KERNALS[0]) || type < 0)type = 0;

    BMPImage* result = image_rescale(image,image->dib_header.width,image->dib_header.height);

    int old_width = image->dib_header.width;
    int old_height = image->dib_header.height;
    int actual_row_size;
    PIC_ROW_SIZE(actual_row_size,image);

    unsigned char* new_row = result->pixel_data;
    unsigned char* cur_row = image->pixel_data;

    int row_offset = (old_width - 1) * 3;
    for(size_t i = 0 ; i < iter ; i++){
        #pragma omp parallel for schedule(guided)
        for (int y = 1; y < old_height-1; y++) {
            int col_offset = y * actual_row_size;
            new_row[col_offset] = cur_row[col_offset];
            new_row[col_offset+1] = cur_row[col_offset+1];
            new_row[col_offset+2] = cur_row[col_offset+2];
            col_offset += row_offset;
            new_row[col_offset] = cur_row[col_offset];
            new_row[col_offset+1] = cur_row[col_offset+1];
            new_row[col_offset+2] = cur_row[col_offset+2];
            for (int x = 1; x < old_width-1; x++) {
                float sum_B = 0,sum_G = 0,sum_R = 0;
                int p = 0;
                for(int i = -1 ; i <= 1 ; i++){
                    for(int j = -1 ; j <= 1 ; j++){
                        int idx = (y+i) * actual_row_size + (x+j) * 3;
                        sum_B += cur_row[idx] * (KERNALS[type][p]);
                        sum_G += cur_row[idx+1] * (KERNALS[type][p]);
                        sum_R += cur_row[idx+2] *(KERNALS[type][p]);
                        p++;
                    }
                }
                int ydx = y * actual_row_size + x * 3;
                new_row[ydx] = fminf(255.0f, fmaxf(0.0f, sum_B));
                new_row[ydx+1] = fminf(255.0f, fmaxf(0.0f, sum_G));
                new_row[ydx+2] = fminf(255.0f, fmaxf(0.0f, sum_R));
            }
        }
    }
    memcpy(new_row, cur_row, actual_row_size);
    memcpy(new_row+(old_height-1)*actual_row_size, cur_row+(old_height-1)*actual_row_size, actual_row_size);

    return result;
}
