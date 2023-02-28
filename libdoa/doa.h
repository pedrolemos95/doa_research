#include "eigen/Eigen/Dense"
#include "eigen/Eigen/Core"
#include "eigen/Eigen/Eigenvalues"
#include <stdlib.h>

#define PI_CTE 3.14159265359
#define NUM_ANTENNAS 16
#define IQLENGTH 4 // The Number of samples used to estimate the DoA
#define MAX_SAMPSIZE 20

using namespace std;

class aoa_estimator{

    private:
    /* Signal related */
    int num_samples = IQLENGTH;

    /* Algorithm constants */
    int antenna_num = 16;
    float distance = 0.5;
    float product;
    float angle_step = 1*PI_CTE/180;

    /* Last estimated direction*/
    float az_index = 0;
    float el_index = 0;
    float az_esprit; // Azimuth from ESPRIT algorithm
    float el_esprit; // Elevation from ESPRIT algorithm

    /* Antenna array output. It is the algorithm input */
    Eigen::MatrixXcd x{NUM_ANTENNAS, IQLENGTH};

    /* Input signal autocorrelation matrix */
    Eigen::MatrixXcd Rxx{NUM_ANTENNAS, NUM_ANTENNAS};
    Eigen::MatrixXcd Qn{16,15}; // Noise eigenvectors. MUSIC related
    Eigen::MatrixXcd Qs{16,1}; // Signal eigenvector

    float x_st;
    float y_st;
    std::complex<float> phi_x;
    std::complex<float> phi_y;
    Eigen::MatrixXcd a_x{1,4};  // steering vector x component
    Eigen::MatrixXcd a_y{1,4};  // steering vector y component
    Eigen::MatrixXcd st_vec{16,1}; // steering vector
    Eigen::MatrixXcd Qn_Prd{16,16}; // Qn times Qn.adjoint(). It should be stored before executing the MUSIC spectrum scanning
    Eigen::MatrixXcd aux{1,1}; // Used for the MUSIC spectrum denominator

    /* ESPRIT selection matrices */
    Eigen::MatrixXcd Js_1y{12,16};
    Eigen::MatrixXcd Js_2y{12,16};
    Eigen::MatrixXcd Js_1x{12,16};
    Eigen::MatrixXcd Js_2x{12,16};
    /* ESPRIT signal matrices */
    Eigen::MatrixXcd Qs_1x{12,1};
    Eigen::MatrixXcd Qs_2x{12,1};
    Eigen::MatrixXcd Qs_1y{12,1};
    Eigen::MatrixXcd Qs_2y{12,1};
    /* Auxiliary variables for ESPRIT algorithm */
    Eigen::MatrixXcd phi_X_LS{1,1};
    Eigen::MatrixXcd phi_Y_LS{1,1};

    public:
    /* Init DoA estimator */
    void initDoAEstimator(float elements_distance, int debug_flag);
    /* Calculate wavelength based on channel central frequency */
    float calculate_wavelength(float channel_frequency);
    /* Estimate phase rotation based on CTE reference period IQ data */
    float estimate_phase_rotation(float* i_samples, float* q_samples, int reference_period_length);
    /* Compensate the phase rotation due to switching/CTE frequncy offset */
    void compensateRotation(float phase_rotation, int debug_flag);
    /* Load antenna array output matrix */
    void load_x(float** i_samples, float** q_samples, int num_antennas, int num_samples, int debug_flag);
    /* Estimate and decomposes covariance matrix */
    void estimateRxx(int debug_flag);
    /* Update the steering vector */
    void updateSteeringVector(float azimuth, float elevation, int debug_flag);
    /* MUSIC spectrum function. It computes the MUSIC Spatial Spectrum power in the given direction using the Steering Vecotor and Qn matrix */
    float MUSICSpectrum(float azimuth, float elevation, int debug_flag);
    /* Process MUSIC Spectrum */
    void processMUSIC(int azimuth_max, int elevation_max, int debug_flag);
    /* Initialize Selecion Matrices for ESPRIT algorithm */
    void initSelectionMatrices(int debug_flag);
    /* Process ESPRIT algorithm */
    void processESPRIT(float channel_frequency, int debug_flag);
    /* Get processed values */
    void getProcessed(int algorithm, float* x, float* y);
};