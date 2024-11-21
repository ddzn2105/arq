#include <stdio.h>      // Biblioteca padrão de entrada e saída.
#include <stdlib.h>     // Biblioteca padrão para alocação de memória, controle de processos, etc.
#include <stdint.h>     // Biblioteca para tipos de dados com tamanhos definidos (ex: uint16_t, int32_t).
#include <math.h>       // Biblioteca matemática para cálculos (ex: seno, cosseno, PI).
#include <dirent.h>     // Biblioteca para manipulação de diretórios.
#include <string.h>     // Biblioteca para manipulação de strings.
#include <sys/stat.h>   // Biblioteca para manipulação de arquivos e diretórios (stat, mkdir).

#define MAX_FILENAME_LENGTH 256 // Define o tamanho máximo para nomes de arquivos.
#define M_PI 3.14159265358979323846 // Define o valor de PI, caso não esteja disponível na math.h.


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
    uint8_t blue;  // Intensidade do azul.
    uint8_t green; // Intensidade do verde.
    uint8_t red;   // Intensidade do vermelho.
} RGB;

#pragma pack(pop) // Restaura o alinhamento padrão.

int image_index = 1; // Contador global para o índice das imagens

typedef struct {
    double real; // Parte real do número complexo.
    double imag; // Parte imaginária do número complexo.
} Complex; // Estrutura para representar números complexos.

void ensure_directory_exists(const char *dir) {
    struct stat st = {0};  // Armazena informações sobre o diretório.
    if (stat(dir, &st) == -1) { // Verifica se o diretório existe.
        mkdir(dir); // Cria o diretório, se não existir.
    }
}

void write_bmp(const char *filename, RGB *pixels, int width, int height) {
    BITMAPFILEHEADER bfh; // Cabeçalho do arquivo BMP.
    BITMAPINFOHEADER bih; // Cabeçalho de informações BMP.
    FILE *fp = fopen(filename, "wb"); // Abre arquivo para escrita em binário.
    if (!fp) { // Verifica erros ao abrir o arquivo.
        perror("Erro ao criar arquivo BMP");
        return;
    }

    // Preenche o cabeçalho do arquivo BMP.
    bfh.bfType = 0x4D42; // 'BM' em ASCII.
    bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + width * height * sizeof(RGB);
    bfh.bfReserved1 = 0; // Reservado (sempre 0).
    bfh.bfReserved2 = 0; // Reservado (sempre 0).
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER); // Offset para os dados da imagem.

    // Preenche o cabeçalho de informações BMP.
    bih.biSize = sizeof(BITMAPINFOHEADER); // Tamanho da estrutura.
    bih.biWidth = width; // Largura da imagem.
    bih.biHeight = height; // Altura da imagem.
    bih.biPlanes = 1; // Planos de cor.
    bih.biBitCount = 24; // Bits por pixel (24 para RGB).
    bih.biCompression = 0; // Sem compressão.
    bih.biSizeImage = 0; // Tamanho da imagem (0 para sem compressão).
    bih.biXPelsPerMeter = 0; // Resolução horizontal.
    bih.biYPelsPerMeter = 0; // Resolução vertical.
    bih.biClrUsed = 0; // Todas as cores usadas.
    bih.biClrImportant = 0; // Todas as cores importantes.

    // Escreve cabeçalhos e dados de pixel no arquivo.
    fwrite(&bfh, sizeof(BITMAPFILEHEADER), 1, fp);
    fwrite(&bih, sizeof(BITMAPINFOHEADER), 1, fp);
    fwrite(pixels, sizeof(RGB), width * height, fp);

    fclose(fp); // Fecha o arquivo.
}

// FFT = Fast Fourier Transform
void fft(Complex *x, int N) {
    if (N <= 1) return; // Caso base: vetor de tamanho 1 já está ordenado.

    Complex *even = (Complex *)malloc(N / 2 * sizeof(Complex)); // Aloca memória para elementos pares.
    Complex *odd = (Complex *)malloc(N / 2 * sizeof(Complex));  // Aloca memória para elementos ímpares.

    // Separa elementos pares e ímpares.
    for (int i = 0; i < N / 2; i++) {
        even[i] = x[i * 2]; // Elementos de índice par.
        odd[i] = x[i * 2 + 1]; // Elementos de índice ímpar.
    }

    fft(even, N / 2); // Recursão nos pares.
    fft(odd, N / 2);  // Recursão nos ímpares.

    // Combina resultados.
    for (int k = 0; k < N / 2; k++) {
        double t = -2.0 * M_PI * k / N; // Calcula o ângulo.
        Complex exp = {cos(t), sin(t)}; // Termo exponencial.
        Complex temp = { // Multiplicação complexa.
            exp.real * odd[k].real - exp.imag * odd[k].imag,
            exp.real * odd[k].imag + exp.imag * odd[k].real
        };
        // Atualiza os valores no vetor original.
        x[k].real = even[k].real + temp.real;
        x[k].imag = even[k].imag + temp.imag;
        x[k + N / 2].real = even[k].real - temp.real;
        x[k + N / 2].imag = even[k].imag - temp.imag;
    }

    free(even); // Libera memória dos pares.
    free(odd);  // Libera memória dos ímpares.
}

void save_fft_to_txt(const char *filename, Complex *fft_result, int N) {
    FILE *fp = fopen(filename, "w"); // Abre o arquivo para escrita em modo texto.
    if (fp) { // Verifica se o arquivo foi aberto com sucesso.
        for (int i = 0; i < N; i++) {
            // Escreve os valores reais e imaginários no arquivo.
            fprintf(fp, "%lf %lf\n", fft_result[i].real, fft_result[i].imag);
        }
        fclose(fp); // Fecha o arquivo.
    } else {
        perror("Erro ao criar arquivo TXT"); // Exibe mensagem de erro se falhar.
    }
}

void apply_fft(RGB *channel, int width, int height, const char *dat_filename, const char *txt_filename) {
    int N = width * height; // Calcula o total de pixels.
    Complex *fft_result = (Complex *)malloc(N * sizeof(Complex)); // Aloca memória para os resultados da FFT.

    for (int i = 0; i < N; i++) {
        fft_result[i].real = channel[i].red; // Usa o valor do canal vermelho como entrada real.
        fft_result[i].imag = 0.0; // Parte imaginária é inicializada como 0.
    }

    fft(fft_result, N); // Aplica a Transformada Rápida de Fourier.

    // Salva os resultados no formato binário (arquivo .dat).
    FILE *fp = fopen(dat_filename, "wb");
    if (fp) {
        fwrite(fft_result, sizeof(Complex), N, fp); // Escreve o resultado no arquivo binário.
        fclose(fp);
    }

    save_fft_to_txt(txt_filename, fft_result, N); // Salva os resultados em formato texto.

    free(fft_result); // Libera a memória alocada.
}

void extract_channels(const char *input_file) {
    FILE *fp = fopen(input_file, "rb"); // Abre o arquivo BMP no modo leitura binária.
    if (!fp) {
        perror("Erro ao abrir arquivo BMP"); // Mensagem de erro caso o arquivo não seja aberto.
        return;
    }

    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;

    // Lê o cabeçalho do arquivo BMP.
    if (fread(&bfh, sizeof(BITMAPFILEHEADER), 1, fp) != 1) {
        perror("Erro ao ler cabeçalho do arquivo BMP");
        fclose(fp);
        return;
    }

    // Lê o cabeçalho de informações da imagem.
    if (fread(&bih, sizeof(BITMAPINFOHEADER), 1, fp) != 1) {
        perror("Erro ao ler cabeçalho de informações do BMP");
        fclose(fp);
        return;
    }

    int width = bih.biWidth;   // Largura da imagem.
    int height = bih.biHeight; // Altura da imagem.
    RGB *pixels = malloc(width * height * sizeof(RGB)); // Aloca memória para armazenar os pixels.
    if (!pixels) {
        perror("Erro ao alocar memória.");
        fclose(fp);
        return;
    }

    fread(pixels, sizeof(RGB), width * height, fp); // Lê os dados dos pixels da imagem.
    fclose(fp); // Fecha o arquivo.

    // Aloca memória para os três canais de cor.
    RGB *red_channel = malloc(width * height * sizeof(RGB));
    RGB *green_channel = malloc(width * height * sizeof(RGB));
    RGB *blue_channel = malloc(width * height * sizeof(RGB));

    if (!red_channel || !green_channel || !blue_channel) {
        perror("Erro ao alocar memória para os canais.");
        free(pixels);
        free(red_channel);
        free(green_channel);
        free(blue_channel);
        return;
    }

    printf("Extraindo canais de cores do arquivo: %s\n", input_file);

    // Separa os canais de cor.
    for (int i = 0; i < width * height; i++) {
        red_channel[i].red = pixels[i].red; // Apenas o canal vermelho.
        red_channel[i].green = 0;
        red_channel[i].blue = 0;

        green_channel[i].red = 0;
        green_channel[i].green = pixels[i].green; // Apenas o canal verde.
        green_channel[i].blue = 0;

        blue_channel[i].red = 0;
        blue_channel[i].green = 0;
        blue_channel[i].blue = pixels[i].blue; // Apenas o canal azul.
    }

    // Cria nomes para os arquivos de saída.
    char filename[MAX_FILENAME_LENGTH];
    snprintf(filename, sizeof(filename), "output_channels/red_channel_%02d.bmp", image_index);
    write_bmp(filename, red_channel, width, height); // Salva o canal vermelho como BMP.

    snprintf(filename, sizeof(filename), "output_channels/green_channel_%02d.bmp", image_index);
    write_bmp(filename, green_channel, width, height); // Salva o canal verde como BMP.

    snprintf(filename, sizeof(filename), "output_channels/blue_channel_%02d.bmp", image_index);
    write_bmp(filename, blue_channel, width, height); // Salva o canal azul como BMP.

    // Aplica FFT aos canais e salva os resultados.
    snprintf(filename, sizeof(filename), "output_fft_DAT/red_channel_fft_%02d.dat", image_index);
    char txt_filename[MAX_FILENAME_LENGTH];
    snprintf(txt_filename, sizeof(txt_filename), "output_fft_TXT/red_channel_fft_%02d.txt", image_index);
    apply_fft(red_channel, width, height, filename, txt_filename);

    snprintf(filename, sizeof(filename), "output_fft_DAT/green_channel_fft_%02d.dat", image_index);
    snprintf(txt_filename, sizeof(txt_filename), "output_fft_TXT/green_channel_fft_%02d.txt", image_index);
    apply_fft(green_channel, width, height, filename, txt_filename);

    snprintf(filename, sizeof(filename), "output_fft_DAT/blue_channel_fft_%02d.dat", image_index);
    snprintf(txt_filename, sizeof(txt_filename), "output_fft_TXT/blue_channel_fft_%02d.txt", image_index);
    apply_fft(blue_channel, width, height, filename, txt_filename);

    free(pixels); // Libera memória para os dados de pixels originais.
    free(red_channel); // Libera memória para o canal vermelho.
    free(green_channel); // Libera memória para o canal verde.
    free(blue_channel); // Libera memória para o canal azul.

    image_index++; // Incrementa o índice global para a próxima imagem.
}

void process_images_in_directory(const char *directory) {
    ensure_directory_exists("output_fft_DAT"); // Cria diretório para os arquivos .dat.
    ensure_directory_exists("output_fft_TXT"); // Cria diretório para os arquivos .txt.
    ensure_directory_exists("output_channels"); // Cria diretório para os canais separados.

    struct dirent *entry; // Estrutura para armazenar informações de entrada no diretório.
    DIR *dp = opendir(directory); // Abre o diretório especificado.

    if (dp == NULL) { // Verifica se o diretório foi aberto com sucesso.
        perror("Erro ao abrir diretório");
        return;
    }

    while ((entry = readdir(dp)) != NULL) { // Itera sobre os arquivos no diretório.
        if (entry->d_type == DT_REG && strstr(entry->d_name, ".bmp")) { 
            // Verifica se é um arquivo regular e com extensão ".bmp".
            char input_file[MAX_FILENAME_LENGTH];
            snprintf(input_file, sizeof(input_file), "%s/%s", directory, entry->d_name);
            printf("Processando arquivo: %s\n", input_file);
            extract_channels(input_file); // Processa a imagem BMP.
        } else {
            printf("Arquivo ignorado: %s\n", entry->d_name); // Ignora arquivos que não sejam BMP.
        }
    }

    closedir(dp); // Fecha o diretório.
}


int main() {
    process_images_in_directory("img"); // Processa as imagens no diretório "img".
    printf("Programa Concluído com sucesso!\n"); // Mensagem de finalização.
    return 0;
}

