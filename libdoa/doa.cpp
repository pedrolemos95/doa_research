#include <iostream>
#include <fstream>
#include <string.h>
#include <stdio.h>
#include <complex>
#include <cmath>
#include "doa.h"

using namespace std;

#define SPEED_OF_LIGHT 299792458 // Speed of light in meters/second

/**
 * Direction of Arrival estimation library. MUSIC and ESPRIT algorithms for a 16-element Uniform Rectangular Array.
 *
 * @copyright   UFSC
 * @file        libaoa.cpp
 * @author      Pedro Lemos
 */

void aoa_estimator::initDoAEstimator(float elements_distance, int debug_flag){

    /* Distance between antenna array elements */
    this->distance = elements_distance;

};

float aoa_estimator::calculate_wavelength(float channel_frequency)
{
    return SPEED_OF_LIGHT/channel_frequency;
}

float aoa_estimator::estimate_phase_rotation(float* i_samples, float* q_samples, int reference_period_length)
{
    float sample_phase = 0;
    float next_sample_phase = 0;
    float phase_difference = 0;
    float phase_difference_acc = 0;
    for (int sample = 0; sample < ( reference_period_length - 1); sample++)
    {
        sample_phase = atan2(q_samples[sample], i_samples[sample]);
        next_sample_phase = atan2(q_samples[sample + 1], i_samples[sample + 1]);
        phase_difference = atan2(sin(next_sample_phase - sample_phase), cos(next_sample_phase - sample_phase));
        phase_difference_acc += atan2(sin(phase_difference), cos(phase_difference));
    }
    phase_difference_acc /= (reference_period_length-1);
    phase_difference_acc *= 2*180/PI_CTE;

    return phase_difference_acc;
}

void aoa_estimator::compensateRotation(float phase_rotation, int debug_flag){

    std::complex<double> phase_rot;
    phase_rot = {0, PI_CTE*phase_rotation/180};

    for (int antenna = 0; antenna < NUM_ANTENNAS; antenna++){
        for (int snapshot = 0; snapshot < IQLENGTH; snapshot++){
            this->x(antenna,snapshot) = (this->x(antenna,snapshot))*(exp((std::complex<double> (antenna,0))*phase_rot));
        }
    }

}

void aoa_estimator::load_x(float** i_samples, float** q_samples, int num_antennas, int num_samples, int debug_flag){

    // Auxiliary complex variable
    std::complex<float> iq_sample(0,0);

    for (int n = 0; n < num_antennas; n++){
        for (int k = 0; k < num_samples; k++){
            // Transform to complex value
            iq_sample = {i_samples[k][n], q_samples[k][n]};
            // Assign to aoa_estimator class
            this->x(n,k) = iq_sample;
        }
    }
}

void aoa_estimator::estimateRxx(int debug_flag){

    // Estimate covariance matrix
    this->Rxx = (this->x)*(this->x.adjoint())/IQLENGTH;

    // Eigendecomposition
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXcd> eigensolver(this->Rxx);

    // Find the eigenvectors related to noise
    float maximum_value = 0;
    int index = 0;

    for (int n = 0; n < this->antenna_num; n++){
        if (eigensolver.eigenvalues()(n) > maximum_value){
            index = n;
        }
    }

    this->Qn.block(0,0,16,index) = eigensolver.eigenvectors().block(0,0,16,index);
    this->Qn.block(0,index,16,this->antenna_num - (index+1)) = eigensolver.eigenvectors().block(0,index+1,16,this->antenna_num - (index+1));

    // Assign Signal eigenvectors
    this->Qs = eigensolver.eigenvectors().block(0, index, 16, 1);

    this->Qn_Prd = Qn*Qn.adjoint();

}

void aoa_estimator::updateSteeringVector(float azimuth, float elevation, int debug_flag){

    this->x_st = 2*PI_CTE*this->distance*sin(elevation)*cos(azimuth);
    this->y_st = 2*PI_CTE*this->distance*sin(elevation)*sin(azimuth);

    this->phi_x = {0, this->x_st};
    this->phi_y = {0, this->y_st};

    for (int n = 0; n < 4; n++){
        this->a_x(0,n) = exp((std::complex<float> (n,0))*this->phi_x);
        this->a_y(0,n) = exp((std::complex<float> (n,0))*this->phi_y);
    }

    for (int n = 0; n < 4; n++){
        for (int k = 0; k < 4; k++){
            this->st_vec(4*n+k,0) = this->a_y(0,n)*this->a_x(0,k);
        }
    }

}

float aoa_estimator::MUSICSpectrum(float azimuth, float elevation, int debug_flag){

    float product = 0;

    // Update the steering vector to be used right after
    updateSteeringVector(azimuth, elevation, 0);
    // Compute the MUSIC Spectrum denominator
    this->aux = this->st_vec.adjoint()*this->Qn_Prd*this->st_vec;

    // Just extract the real part and invert
    product = this->aux(0,0).real();
    product = 1/product;

    return product;
}

void aoa_estimator::processMUSIC(int azimuth_max, int elevation_max, int debug_flag){

    // Just some auxiliary variables
    float last_value = 0;
    float product;
    float azimuth, elevation;

    for (int n_azim = 0; n_azim < azimuth_max; n_azim++){
        for (int n_elev = 0; n_elev < elevation_max; n_elev++){

            azimuth = n_azim*(this->angle_step);
            elevation = n_elev*(this->angle_step);
            product = MUSICSpectrum(azimuth, elevation, 0);

            // Store the last max value and index
            if (product > last_value){
                last_value = product;
                this->az_index = azimuth;
                this->el_index = elevation;
            }
        }
    }
}

void aoa_estimator::initSelectionMatrices(int debug_flag){

    this->Js_1y <<  1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                    0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                    0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,
                    0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,
                    0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,
                    0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,
                    0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,
                    0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,
                    0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,
                    0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,
                    0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,
                    0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0;

    this->Js_1x <<  1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                    0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                    0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,
                    0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,
                    0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,
                    0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,
                    0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,
                    0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,
                    0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,
                    0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,
                    0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,
                    0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0;

    this->Js_2y <<  0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,
                    0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,
                    0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,
                    0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,
                    0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,
                    0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,
                    0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,
                    0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,
                    0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,
                    0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,
                    0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,
                    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1;

    this->Js_2x <<  0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                    0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,
                    0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,
                    0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,
                    0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,
                    0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,
                    0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,
                    0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,
                    0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,
                    0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,
                    0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,
                    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1;
}

void aoa_estimator::processESPRIT(float channel_frequency, int debug_flag){

    this->Qs_1x = (this->Js_1x)*(this->Qs);
    this->Qs_2x = (this->Js_2x)*(this->Qs);
    this->Qs_1y = (this->Js_1y)*(this->Qs);
    this->Qs_2y = (this->Js_2y)*(this->Qs);

    this->phi_X_LS = ((this->Qs_1x.adjoint()*this->Qs_1x).inverse())*((this->Qs_1x.adjoint())*Qs_2x);
    this->phi_Y_LS = ((this->Qs_1y.adjoint()*this->Qs_1y).inverse())*((this->Qs_1y.adjoint())*Qs_2y);

    float A, B, azimuth, elevation, wavelength;
    A = arg(this->phi_Y_LS(0,0));
    B = arg(this->phi_X_LS(0,0));
    azimuth = atan2(B, A);
    wavelength = calculate_wavelength(channel_frequency);
    // elevation = asin(A/(2*PI_CTE*(this->distance/wavelength)*sin(azimuth))); // Eq. 1

// Forçando a barra na cara dura. Quando o argumento A/(2*PI_CTE*(this->distance/wavelength)*sin(azimuth)) é maior que 1 em (Eq. 1)
// o resultado é nan, já que a função arco seno não está definida para valores fora do intervalo [-1,1]
// Mas, se trocarmos pela função arco seno (std::asin do header <complex>) que aceita numeros complexos, ela está definida para valores maiores
// que 1. Ainda não sei justificar, mas pegando a saída desta função e tirando o módulo o resultado faz sentido.
std::complex<double> elevation_input(A/(2*PI_CTE*(this->distance/wavelength)*sin(azimuth)),0);
elevation = std::abs(std::acos(elevation_input));

    this->az_esprit = 180*azimuth/PI_CTE;
    this->el_esprit = 180*elevation/PI_CTE;
}

void aoa_estimator::getProcessed(int algorithm, float* az, float* el){
    if (algorithm == 1){
        *az = this->az_esprit;
        *el = this->el_esprit;
    }
    if (algorithm == 2){
        *az = 180*this->az_index/PI_CTE;
        *el = 180*this->el_index/PI_CTE;
    }
}