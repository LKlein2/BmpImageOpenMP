#include <stdio.h>
#include <stdlib.h>
#pragma pack(1)

typedef struct cabecalho {
	unsigned short tipo;
	unsigned int tamanho_arquivo;
	unsigned short reservado1;
	unsigned short reservado2;
	unsigned int offset;
	unsigned int tamanho_cabecalho;
	int largura;
	int altura;
	unsigned short planos;
	unsigned short bits;
	unsigned int compressao;
	unsigned int tamanho_imagem;
	int xresolucao;
	int yresolucao;
	unsigned int cores_usadas;
	unsigned int cores_significantes;
} CABECALHO;

typedef struct rgb{
	unsigned char blue;
	unsigned char green;
	unsigned char red;
} RGB;

void lerArquivo(RGB** img, CABECALHO header, FILE *file)
{
    int alinhamento;
    unsigned char byte;

	for(int i = 0 ; i < header.altura ; i++) {
		alinhamento = (header.largura * 3) % 4;

		if (alinhamento != 0){
			alinhamento = 4 - alinhamento;
		}

		for(int j = 0 ; j < header.largura ; j++){
			fread(&img[i][j], sizeof(RGB), 1, file);
		}

		for(int j = 0; j<alinhamento; j++){
			fread(&byte, sizeof(unsigned char), 1, file);
		}
	}
}

void escreverArquivo(RGB** img, CABECALHO header, FILE *file)
{
    int alinhamento;
    unsigned char byte;

	for(int i = 0 ; i < header.altura ; i++) {
		alinhamento = (header.largura * 3) % 4;

		if (alinhamento != 0){
			alinhamento = 4 - alinhamento;
		}

		for(int j = 0 ; j < header.largura ; j++){
			fwrite(&img[i][j], sizeof(RGB), 1, file);
		}

		for(int j = 0; j<alinhamento; j++){
			fwrite(&byte, sizeof(unsigned char), 1, file);
		}
	}
}

int * ordenarVetor(int* array, int size) {
	for(int i = 0 ; i < size ; i++) {
		for(int j = 0 ; j < size - 1 ; j++) {
			if(array[j] > array[j + 1])
			{
				int temp = array[j];
				array[j] = array[j + 1];
				array[j + 1] = temp;
			}
		}
	}
	return array;
}

/*
MAIN ---------------------------------------------------------------------
*/
int main(int argc, char **argv ){
    FILE *fileIn, *fileOut;
    CABECALHO header;
    RGB **imgIn, **imgOut, **imgFinal, **imgAux;
    int mascara;

    argc = 4;
    argv[1] = "c:\\temp\\borboleta.bmp";
    argv[2] = "c:\\temp\\lixo.bmp";
    argv[3] = "7";

    //Testa o numero de argumentos
    if (argc != 4) {
		printf("%s <img_entrada> <img_saida> <mascara> <Threads> \n", argv[0]);
		exit(0);
	}

    //Abre o arquivo de input
	fileIn = fopen(argv[1], "rb");
	if (fileIn == NULL) {
        printf("Erro ao abrir o arquivo %s\n", argv[1]);
		exit(0);
	}

	//Abre o arquivo de output
	fileOut = fopen(argv[2], "wb");
	if (fileOut == NULL){
		printf("Erro ao abrir o arquivo %s\n", argv[2]);
		exit(0);
	}

    //Verifica tamanho da mascara
	mascara = atoi(argv[3]);
    if (mascara != 3 && mascara != 5 && mascara != 7) {
        printf("Mascara de %d não suportada!\n", mascara);
		exit(0);
    }

    //Lê o cabeçalho do arquivo de entrada
	fread(&header, sizeof(CABECALHO), 1, fileIn);
	fwrite(&header, sizeof(CABECALHO), 1, fileOut);

    //Aloca o espaço para os pixels da imagem
    imgIn  = (RGB **)malloc(header.altura * sizeof(RGB *));
    imgOut = (RGB **)malloc(header.altura * sizeof(RGB *));
	imgFinal = (RGB **)malloc(header.altura * sizeof(RGB *));
    for(int i = 0 ; i < header.altura ; i++) {
		imgIn[i]  = (RGB *)malloc(header.largura * sizeof(RGB));
		imgOut[i] = (RGB *)malloc(header.largura * sizeof(RGB));
		imgFinal[i] = (RGB *)malloc(header.largura * sizeof(RGB));
	}
	
	lerArquivo(imgIn, header, fileIn);

	int sizeOfArray = (mascara * mascara);
	int* arrayR = (int*)malloc(sizeOfArray * sizeof(int));
	int* arrayG = (int*)malloc(sizeOfArray * sizeof(int));
	int* arrayB = (int*)malloc(sizeOfArray * sizeof(int));

	//Calcula o range
	//Para mascara de 3: -1 até 1 então range = 1
	//Para mascara de 5: -2 até 2 então range = 2
	//Para mascara de 7: -3 até 3 então range = 3
	int range = (mascara - 1) / 2;
	int mediana = sizeOfArray / 2;	

	for (int i = 0; i < header.altura; i++)
	{
		for (int j = 0; j < header.largura; j++)
		{
			int posicaoArray = 0;

			//Range eixo X
			for (int k = -range; k <= range; k++)
			{
				int posAltura = i + k;
				
				//Range eixo Y
				for (int l = -range; l <= range; l++)
				{
					int posLargura = j + l;

					if (posAltura < 0 || posLargura < 0 || posAltura >= header.altura || posLargura >= header.largura) {
						arrayR[posicaoArray] = 0;
						arrayG[posicaoArray] = 0;
						arrayB[posicaoArray] = 0;
						posicaoArray++;
					} else {
						arrayR[posicaoArray] = imgIn[posAltura][posLargura].red;
						arrayG[posicaoArray] = imgIn[posAltura][posLargura].green;
						arrayB[posicaoArray] = imgIn[posAltura][posLargura].blue;
						posicaoArray++;
					}
				}				
			}		

			imgOut[i][j].red   = ordenarVetor(arrayR, sizeOfArray)[mediana];
			imgOut[i][j].green = ordenarVetor(arrayG, sizeOfArray)[mediana];
			imgOut[i][j].blue  = ordenarVetor(arrayB, sizeOfArray)[mediana];
		}
	}

	free(arrayR);
	free(arrayG);
	free(arrayB);

	escreverArquivo(imgOut, header, fileOut);

	fclose(fileIn);
	fclose(fileOut);
}
