#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define PI         3.141592653589793
#define PI2     (2.0 * PI)
#define SIZE       8
#define SWAP(a,b)  tempr=(a);(a)=(b);(b)=tempr

char chunkID[5];
int chunkSize;
char format[5];

char subChunk1ID[5];
int subChunk1Size;
short audioFormat;
short numChannels;
int sampleRate;
int byteRate;
short blockAlign;
short bitsPerSample;

int channelSize;
char subChunk2ID[5];
int subChunk2Size;

short* data;

float impulseResp[460000];
float impulseSize;
void print()
{
	chunkID[5] = '\0';
	format[5] = '\0';
	subChunk1ID[5] = '\0';
	subChunk2ID[5] = '\0';
	
	printf("\n============= HEADER INFO =============\n");
	printf(" chunkID:%s\n", chunkID);
	printf(" chunkSize:%d\n", chunkSize);
	printf(" format:%s\n", format);
	printf(" subChunk1ID:%s\n", subChunk1ID);
	printf(" subChunk1Size:%d\n", subChunk1Size);
	printf(" audioFormat:%d\n", audioFormat);
	printf(" numChannels:%d\n", numChannels);
	printf(" sampleRate:%d\n", sampleRate);
	printf(" byteRate:%d\n", byteRate);
	printf(" blockAlign:%d\n", blockAlign);
	printf(" bitsPerSample:%d\n", bitsPerSample);
	printf(" subChunk2ID:%s\n", subChunk2ID);
	printf(" subChunk2Size:%d\n", subChunk2Size);
}

void complexDFT(float x[], int N)
{
    int n, k, nn;
    double omega = TWO_PI / (double)N;
    double *a, *b;

    // Allocate temporary arrays
    a = (double *)calloc(N, sizeof(double));
    b = (double *)calloc(N, sizeof(double));

    // Perform the DFT
    for (k = 0; k < N; k++) {
	a[k] = b[k] = 0.0;
	for (n = 0, nn = 0; n < N; n++, nn += 2) {
	    a[k] += (x[nn] * cos(omega * n * k));
	    b[k] -= (x[nn] * sin(omega * n * k));
	}
    }

    // Pack result back into input data array
    for (n = 0, k = 0; n < N*2; n += 2, k++) {
	x[n] = a[k];
	x[n+1] = b[k];
    }

    // Free up memory used for arrays
    free(a);
    free(b);
}

int loadWave(char* filename, char* impulse)
{
	FILE* in = fopen(filename, "rb");
	FILE* pulse = fopen(impulse, "rb");
	if (in != NULL)
	{		
		printf("Reading %s...\n",filename);

		fread(chunkID, 1, 4, in);
		fread(&chunkSize, 1, 4, in);
		fread(format, 1, 4, in);

		//sub chunk 1
		fread(subChunk1ID, 1, 4, in);
		fread(&subChunk1Size, 1, 4, in);
		fread(&audioFormat, 1, 2, in);
		fread(&numChannels, 1, 2, in);
		fread(&sampleRate, 1, 4, in);
		fread(&byteRate, 1, 4, in);
		fread(&blockAlign, 1, 2, in);
		fread(&bitsPerSample, 1, 2, in);		
		
		//read extra bytes
		if(subChunk1Size == 18)
		{
			short empty;
			fread(&empty, 1, 2, in);		
		}
		
		//sub chunk 2
		fread(subChunk2ID, 1, 4, in);
		fread(&subChunk2Size, 1, 4, in);

		//read data		
		int bytesPerSample = bitsPerSample/8;
		int numSamples = subChunk2Size / bytesPerSample;
		data = (short*) malloc(sizeof(short) * numSamples);
		
		//fread(data, 1, bytesPerSample*numSamples, in);
		
		int i=0;
		short sample=0;
		while(fread(&sample, 1, bytesPerSample, in) == bytesPerSample)
		{		
			data[i++] = sample;
			sample = 0;			
		}
		
		fseek(pulse, 0, SEEK_END);
		impulseSize = ftell(pulse);
		printf("impulse size %f\n", impulseSize);
		fread(impulseResp, 1, impulseSize, pulse);
		
		fclose(in);
		printf("Closing %s...\n",filename);
	}
	else
	{
		printf("Can't open file: %s\n", filename);
		return 0;
	}
	return 1;
}

int saveWave(char* filename)
{
	FILE* out = fopen(filename, "wb");

	if (out != NULL)
	{		
		printf("\nWriting %s...\n",filename);

		fwrite(chunkID, 1, 4, out);
		fwrite(&chunkSize, 1, 4, out);
		fwrite(format, 1, 4, out);

		//sub chunk 1
		fwrite(subChunk1ID, 1, 4, out);
		fwrite(&subChunk1Size, 1, 4, out);
		fwrite(&audioFormat, 1, 2, out);
		fwrite(&numChannels, 1, 2, out);
		fwrite(&sampleRate, 1, 4, out);
		fwrite(&byteRate, 1, 4, out);
		fwrite(&blockAlign, 1, 2, out);
		fwrite(&bitsPerSample, 1, 2, out);		
		
		//read extra bytes
		if(subChunk1Size == 18)
		{
			short empty = 0;
			fwrite(&empty, 1, 2, out);		
		}
		
		//sub chunk 2
		fwrite(subChunk2ID, 1, 4, out);
		fwrite(&subChunk2Size, 1, 4, out);

		//read data		
		int bytesPerSample = bitsPerSample / 8;
		int sampleCount =  subChunk2Size / bytesPerSample;
		
		//impulse response - echo
		int IRSize = 12;
		float IR[IRSize];
		for(p = 0; p < IRSize; p++)
		{
			IR[p] = 0.7;
			
		}
		
		
		
		//write the data
		float* newData = (float*) malloc(sizeof(float) * (sampleCount + IRSize - 1));
		float maxSample = -1;
		float MAX_VAL = 32767.f;	//FIXME: find based on bits per sample
			
		int i = 1;
		
		int n, k, nn;
		double omega = TWO_PI / (double)IRSize;
		double *a, *b;
		
		// Allocate temporary arrays
		a = (double *)calloc(IRSize, sizeof(double));
		b = (double *)calloc(IRSize, sizeof(double));
		
		//for(i = 0; i<sampleCount; ++i)
		//{			
			//convolve
			/*int j;
			for(j=0; j<IRSize; ++j)
				//newData[i+j] += ((float)data[i] / MAX_VAL) * IR[j];*/
				
				
			// Perform the DFT
			for (k = 0; k < IRSize; k++) {
				a[k] = b[k] = 0.0;
				for (n = 0, nn = 0; n < IRSize; n++, nn += 2) {
					a[k] += (newData[nn] * cos(omega * n * i));
					
				for (n = 0, nn = 0; n < IRSize; n++, nn += 2)
				{
					b[k] -= (newData[nn] * sin(omega * n * i));
				}
					
					
				}
				i++;
				
				free(a);
				free(b);
				//Keep track of max value for scaling
				if(i==0)
					maxSample = newData[0];
				else if(newData[i] > maxSample)
					maxSample = newData[i];
		//}		
		}
		
		int m;
		//scale and re write the data
		for(m=0; m < sampleCount + IRSize - 1; ++m)
		{
			newData[m] = (newData[k] / maxSample) ;
			short sample = (short) (newData[m] * MAX_VAL);
			fwrite(&sample, 1, bytesPerSample, out);
		}
		
		//clean up
		free(newData);
		fclose(out);
		printf("Closing %s...\n",filename);
	}
	else
	{
		printf("Can't open file: %s\n", filename);
		return 0;
	}
	return 1;
}

int loadImpulse(char* filename)
{
	FILE* in = fopen(filename, "rb");
	fseek(in, 0, SEEK_END);
	impulseSize = ftell(in);
	printf("here %f\n", impulseSize);
	fread(impulseResp, 1, impulseSize, in);
	return 1;
}

int main(int argc, char* argv[])
{
	char* filename = argv[1];
	char* impulse = argv[2];
	char* outFilename = "outDFT.wav";
	clock_t startTime, endTime;
	double totalTime;
	
	if(argc == 4)
		outFilename = argv[2];
	else if(argc != 3)
	{
		printf("Invalid Usage: ./program <input.wav> <impulse.wav>\n");
		return -1;
	}

	//loadImpulse(impulse);
	if(loadWave(filename, impulse))
	{
		print();
		
		startTime = clock();
		saveWave(outFilename);
		endTime = clock();
		
		free(data);		
		totalTime = (double)(endTime - startTime) / CLOCKS_PER_SEC;
		printf("total time: %f\n", totalTime);
	}
	else
		return -1;
	return 0;
}