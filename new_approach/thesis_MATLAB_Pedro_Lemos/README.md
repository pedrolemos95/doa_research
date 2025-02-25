# Description
This work simulates a Rice channel and generates the observations for this channel with an antenna array.
The simulation takes two channel condition values, K_high and K_low as well as the sampling parameters and 

# How to use it

To set different parameters, change the variables:
M_1: to set the number of frequencies
M_2 and M_3: to set the number of antennas. The array is URA, so M_2 and M_3 are the number of rows and columns of the array
num_observations: to change the number of observations
K_high: K for good channel conditions. Can be any value, but it is one out of the two channel condition to be simulated
K_low: K for bad channel conditions. The second channel condition to be simulated

# The physical parameters
"Delays", "elevations" and "azimuths" variables are fixed during the simulations since we want to understand the effect of changing the aperture dimensions. But, it can be changed to verify that the conclusions about the estimator (how it behaves with a varying number of frequencies, antennas and LoS/NLoS ratio) remain the same. The "Delays" variable only make sense if we try to estimate multiple signals, which is not the case.