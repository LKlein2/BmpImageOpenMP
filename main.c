#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
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

double horaAtual(void){
     struct timeval tval;
     gettimeofday(&tval, NULL);
     return (tval.tv_sec + tval.tv_usec/1000000.0);
}

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
    int mascara, threads;
	double tempoIni, tempoFim;

	/*
    argc = 5;
    argv[1] = "c:\\temp\\borboleta.bmp";
    argv[2] = "c:\\temp\\lixo.bmp";
    argv[3] = "7";
	argv[4] = "4";
	*/

    //Testa o numero de argumentos
    if (argc != 5) {
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

	//Verifica o número de threads
	threads = atoi(argv[4]);
    if (threads < 1) {
        printf("Número de threads deve ser maior ou igual a 1");
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
	int* arrayR;
	int* arrayG;
	int* arrayB;

	//Calcula o range
	//Para mascara de 3: -1 até 1 então range = 1
	//Para mascara de 5: -2 até 2 então range = 2
	//Para mascara de 7: -3 até 3 então range = 3
	int range = (mascara - 1) / 2;
	int mediana = sizeOfArray / 2;	

	int threadId, numThreads;
	int posAltura, posLargura, posicaoArray; 
	int i, j, k, l, temp, inicio, fim;

	tempoIni = horaAtual();

	//// Define a quantidade de threads
	omp_set_num_threads(threads);
	#pragma omp parallel private (posAltura, posLargura, posicaoArray, i, j, k, l, temp, arrayR, arrayG, arrayB, inicio, fim) 
	{	
		arrayR = (int *)malloc(sizeOfArray * sizeof(int));
		arrayG = (int *)malloc(sizeOfArray * sizeof(int));
		arrayB = (int *)malloc(sizeOfArray * sizeof(int));

		threadId = omp_get_thread_num();
		numThreads = omp_get_num_threads();
		printf("%d Partir de %d ate %d \n",threadId, threadId * (header.altura / numThreads),((header.altura / numThreads) * (threadId + 1)));

		inicio = (threadId * (header.altura / numThreads));
		fim = ((threadId + 1) * (header.altura / numThreads));
		for (i = inicio; i < fim ; i++)
		{
			for (j = 0; j < header.largura; j++)
			{
				posicaoArray = 0;

				//Range eixo X
				for (k = -range; k <= range; k++)
				{
					posAltura = i + k;
					
					//Range eixo Y
					for (l = -range; l <= range; l++)
					{
						posLargura = j + l;

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

				for(l = 0 ; l < sizeOfArray ; l++) {
					for(k = 0 ; k < sizeOfArray - 1 ; k++) {
						if(arrayR[k] > arrayR[k + 1])
						{
							temp = arrayR[k];
							arrayR[k] = arrayR[k + 1];
							arrayR[k + 1] = temp;
						}
					}
				}

				for(l = 0 ; l < sizeOfArray ; l++) {
					for(k = 0 ; k < sizeOfArray - 1 ; k++) {
						if(arrayG[k] > arrayG[k + 1])
						{
							temp = arrayG[k];
							arrayG[k] = arrayG[k + 1];
							arrayG[k + 1] = temp;
						}
					}
				}

				for(l = 0 ; l < sizeOfArray ; l++) {
					for(k = 0 ; k < sizeOfArray - 1 ; k++) {
						if(arrayB[k] > arrayB[k + 1])
						{
							temp = arrayB[k];
							arrayB[k] = arrayB[k + 1];
							arrayB[k + 1] = temp;
						}
					}
				}

				imgOut[i][j].red   = arrayR[mediana];
				imgOut[i][j].green = arrayG[mediana];
				imgOut[i][j].blue  = arrayB[mediana];
			}
		}
		
		free(arrayR);
		free(arrayG);
		free(arrayB);
	}

	tempoFim = horaAtual();
	printf("Tempo de execução: %f\n", tempoFim - tempoIni);
	
	escreverArquivo(imgOut, header, fileOut);

	fclose(fileIn);
	fclose(fileOut);
}
