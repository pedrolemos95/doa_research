/* Este exemplo mostra como a biblioteca é utilizada. Ex.: Como inicializar os estimadores, */
/* como carregar as amostras de IQ, como produzir uma estimativa de ângulo, etc. */

#include <stdio.h> // fopen()
#include "doa.h"

#define IQFILE "iqsamplesnew.txt"

// Allocate 2D float Buffer for IQ samples
int allocate2DFloatBuffer (float*** buf, int rows, int cols){

    *buf = (float**) malloc (sizeof(float*)*rows);
    if (*buf == NULL){
        return 0;
    }

    for (int i = 0; i < rows; i++){
        (*buf)[i] = (float*) malloc(sizeof(float)*cols);
        if ((*buf)[i] == NULL){
            return 0;
        }
    }

    return 1;
}

int main(int argc, char const *argv[])
{
    // Load IQ samples from file
    float** i_samples;
    float** q_samples;

    // Allocate memory for the IQs
    allocate2DFloatBuffer(&i_samples, IQLENGTH, NUM_ANTENNAS);
    allocate2DFloatBuffer(&q_samples, IQLENGTH, NUM_ANTENNAS);

    // Open the file containing the IQ samples
    FILE* fp;

    fp = fopen(IQFILE, "r");
    if (fp == NULL){
        printf("Error in opening file\n");
        return -1;
    }

    /** This section is used to read the samples as strings and the convert to float **/
    char iqsampleline[MAX_SAMPSIZE];
    // Two auxiliary variables because of strtok function
    char* i_samplech;
    char* q_samplech;
    char i_samplesch[MAX_SAMPSIZE];
    char q_samplesch[MAX_SAMPSIZE];

    int n = 0;
    int k = 0;

    while(fgets(iqsampleline, 30, fp) != NULL){
        // Read one IQ sample as a strings
        i_samplech = strtok(iqsampleline, ",\n");
        q_samplech = strtok(NULL, ",\n");

        // Parse the strings into two strings containing I and Q
        strcpy(i_samplesch, i_samplech);
        strcpy(q_samplesch, q_samplech);

        // Convert these to float values
        i_samples[k][n] = atof(i_samplesch);
        q_samples[k][n] = atof(q_samplesch);

        n++;
        if (n == NUM_ANTENNAS){
            n = 0;
            k++;
        }
    }

    fclose(fp);

    // Initialize the estimator with specific element distance
    auto doa_estimator = aoa_estimator();
    float elements_distance = 0.32;
    doa_estimator.initDoAEstimator(elements_distance, 0);

    // Initialize selection matrices used in ESPRIT algorithm
    doa_estimator.initSelectionMatrices(0);

    // Load iq samples into the estimator
    doa_estimator.load_x(i_samples, q_samples, NUM_ANTENNAS, IQLENGTH, 0);

    // Compensate phase rotation from IQ samples
    float phase_rotation = -179.117203;
    doa_estimator.compensateRotation(phase_rotation, 0);

    // Estimate covariance matrix
    doa_estimator.estimateRxx(0);

    doa_estimator.processESPRIT(0);

    float azimuth, elevation;

    doa_estimator.getProcessed(1, &azimuth, &elevation);

    printf("Processed value: az: %f, el: %f\n", azimuth, elevation);
    /* code */
    return 0;
}