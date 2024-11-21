#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_FILENAME_LENGTH 256
#define M_PI 3.14159265358979323846 // valor de PI π

#pragma pack(push, 1) // alinhamento de estrutura
typedef struct {
    uint16_t bfType;      // tipo de arquivo
    uint32_t bfSize;      // tamanho do arquivo
    uint16_t bfReserved1; // reservado
    uint16_t bfReserved2; // reservado
    uint32_t bfOffBits;   // deslocamento até os dados da imagem
} BITMAPFILEHEADER;

typedef struct {
    uint32_t biSize;            // tamanho desta estrutura
    int32_t  biWidth;           // largura da imagem
    int32_t  biHeight;          // altura da imagem
    uint16_t biPlanes;          // número de planos de cor
    uint16_t biBitCount;        // número de bits por pixel
    uint32_t biCompression;     // tipo de compressão
    uint32_t biSizeImage;       // tamanho da imagem
    int32_t  biXPelsPerMeter;    // resolução horizontal
    int32_t  biYPelsPerMeter;    // resolução vertical
    uint32_t biClrUsed;         // número de cores na paleta
    uint32_t biClrImportant;    // cores importantes
} BITMAPINFOHEADER;

typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
} RGB;

#pragma pack(pop) // retorna ao alinhamento anterior

int image_index = 1; // Contador global para o índice das imagens

typedef struct {
    double real;
    double imag;
} Complex;

void ensure_directory_exists(const char *dir) {
    struct stat st = {0};
    if (stat(dir, &st) == -1) {
        mkdir(dir);
    }
}

void write_bmp(const char *filename, RGB *pixels, int width, int height) {
    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("Erro ao criar arquivo BMP");
        return;
    }

    // preencher cabeçalho do arquivo BMP
    bfh.bfType = 0x4D42; // 'BM'
    bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + width * height * sizeof(RGB);
    bfh.bfReserved1 = 0;
    bfh.bfReserved2 = 0;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    // preencher cabeçalho da informação BMP
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = width;
    bih.biHeight = height;
    bih.biPlanes = 1;
    bih.biBitCount = 24; // 24 bits por pixel
    bih.biCompression = 0; // Sem compressão
    bih.biSizeImage = 0; // Pode ser zero
    bih.biXPelsPerMeter = 0;
    bih.biYPelsPerMeter = 0;
    bih.biClrUsed = 0;
    bih.biClrImportant = 0;

    // escrever cabeçalhos
    fwrite(&bfh, sizeof(BITMAPFILEHEADER), 1, fp);
    fwrite(&bih, sizeof(BITMAPINFOHEADER), 1, fp);
    fwrite(pixels, sizeof(RGB), width * height, fp);

    fclose(fp);
}

// FFT = Fast Fourier Transform
void fft(Complex *x, int N) {
    if (N <= 1) return;

    Complex *even = (Complex *)malloc(N / 2 * sizeof(Complex));
    Complex *odd = (Complex *)malloc(N / 2 * sizeof(Complex));
    for (int i = 0; i < N / 2; i++) {
        even[i] = x[i * 2];
        odd[i] = x[i * 2 + 1];
    }

    fft(even, N / 2);
    fft(odd, N / 2);

    for (int k = 0; k < N / 2; k++) {
        double t = -2.0 * M_PI * k / N;
        Complex exp = {cos(t), sin(t)};
        Complex temp = {exp.real * odd[k].real - exp.imag * odd[k].imag, exp.real * odd[k].imag + exp.imag * odd[k].real};
        x[k].real = even[k].real + temp.real;
        x[k].imag = even[k].imag + temp.imag;
        x[k + N / 2].real = even[k].real - temp.real;
        x[k + N / 2].imag = even[k].imag - temp.imag;
    }

    free(even);
    free(odd);
}

void save_fft_to_txt(const char *filename, Complex *fft_result, int N) {
    FILE *fp = fopen(filename, "w");
    if (fp) {
        for (int i = 0; i < N; i++) {
            fprintf(fp, "%lf %lf\n", fft_result[i].real, fft_result[i].imag);
        }
        printf("Gerando arquivo txt do arquivo : %s\n", filename);
        fclose(fp);
    } else {
        perror("Erro ao criar arquivo TXT");
    }
}

void apply_fft(RGB *channel, int width, int height, const char *dat_filename, const char *txt_filename) {
    int N = width * height;
    Complex *fft_result = (Complex *)malloc(N * sizeof(Complex));

    for (int i = 0; i < N; i++) {
        fft_result[i].real = channel[i].red; 
        fft_result[i].imag = 0.0;
    }

    fft(fft_result, N);

    FILE *fp = fopen(dat_filename, "wb");
    if (fp) {
        fwrite(fft_result, sizeof(Complex), N, fp);
        fclose(fp);
    }

    save_fft_to_txt(txt_filename, fft_result, N);
    free(fft_result);
}

void extract_channels(const char *input_file) {
    FILE *fp = fopen(input_file, "rb");
    if (!fp) {
        perror("Erro ao abrir arquivo BMP");
        return;
    }

    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;

    if (fread(&bfh, sizeof(BITMAPFILEHEADER), 1, fp) != 1) {
        perror("Erro ao ler cabeçalho do arquivo BMP");
        fclose(fp);
        return;
    }

    if (fread(&bih, sizeof(BITMAPINFOHEADER), 1, fp) != 1) {
        perror("Erro ao ler cabeçalho de informações do BMP");
        fclose(fp);
        return;
    }

    int width = bih.biWidth;
    int height = bih.biHeight;
    RGB *pixels = malloc(width * height * sizeof(RGB));
    if (!pixels) {
        perror("Erro ao alocar memoria.");
        fclose(fp);
        return;
    }

    fread(pixels, sizeof(RGB), width * height, fp);
    fclose(fp);

    RGB *red_channel = malloc(width * height * sizeof(RGB));
    RGB *green_channel = malloc(width * height * sizeof(RGB));
    RGB *blue_channel = malloc(width * height * sizeof(RGB));

    if (!red_channel || !green_channel || !blue_channel) {
    perror("Erro ao alocar memoria para os canais.");
    free(pixels);
    free(red_channel);
    free(green_channel);
    free(blue_channel);
    return;
    }

    printf("Extraindo canais de cores do arquivo: %s\n", input_file);
    for (int i = 0; i < width * height; i++) {
        
        red_channel[i].red = pixels[i].red;
        red_channel[i].green = 0;
        red_channel[i].blue = 0;

        
        green_channel[i].red = 0;
        green_channel[i].green = pixels[i].green;
        green_channel[i].blue = 0;

        
        blue_channel[i].red = 0;
        blue_channel[i].green = 0;
        blue_channel[i].blue = pixels[i].blue;
    }

    char filename[MAX_FILENAME_LENGTH];
    printf("Gerando .bmp do arquivo: %s\n", input_file);
    
    snprintf(filename, sizeof(filename), "output_channels/red_channel_%02d.bmp", image_index);
    write_bmp(filename, red_channel, width, height);

    
    snprintf(filename, sizeof(filename), "output_channels/green_channel_%02d.bmp", image_index);
    write_bmp(filename, green_channel, width, height);

    
    snprintf(filename, sizeof(filename), "output_channels/blue_channel_%02d.bmp", image_index);
    write_bmp(filename, blue_channel, width, height);

    printf("Gerando .DAT do arquivo: %s\n", input_file);
   
    snprintf(filename, sizeof(filename), "output_fft_DAT/red_channel_fft_%02d.dat", image_index);
    char txt_filename[MAX_FILENAME_LENGTH];
    snprintf(txt_filename, sizeof(txt_filename), "output_fft_TXT/red_channel_fft_%02d.txt", image_index);
    apply_fft(red_channel, width, height, filename, txt_filename);
    
    
    snprintf(filename, sizeof(filename), "output_fft_DAT/green_channel _fft_%02d.dat", image_index);
    snprintf(txt_filename, sizeof(txt_filename), "output_fft_TXT/green_channel_fft_%02d.txt", image_index);
    apply_fft(green_channel, width, height, filename, txt_filename);

    
    snprintf(filename, sizeof(filename), "output_fft_DAT/blue_channel_fft_%02d.dat", image_index);
    snprintf(txt_filename, sizeof(txt_filename), "output_fft_TXT/blue_channel_fft_%02d.txt", image_index);
    apply_fft(blue_channel, width, height, filename, txt_filename);

    free(pixels);
    free(red_channel);
    free(green_channel);
    free(blue_channel);

    image_index++;
}

void process_images_in_directory(const char *directory) {
    ensure_directory_exists("output_fft_DAT");
    ensure_directory_exists("output_fft_TXT");
    ensure_directory_exists("output_channels");
    struct dirent *entry;
    DIR *dp = opendir(directory);

    if (dp == NULL) {
        perror("Erro ao abrir diretório");
        return;
    }

    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_type == DT_REG && strstr(entry->d_name, ".bmp")) {
            char input_file[MAX_FILENAME_LENGTH];
            snprintf(input_file, sizeof(input_file), "%s/%s", directory, entry->d_name);
            printf("Processando arquivo: %s\n", input_file);
            extract_channels(input_file);
        } else {
            printf("Arquivo ignorado: %s\n", entry->d_name);
        }
    }

    closedir(dp);
}

int main() {
    process_images_in_directory("img");
    printf("Programa Concluído com sucesso!\n");
    return 0;
}
