# Description
This work simulates a Rice channel and generates the observations for this channel with an antenna array.

# How to use it

For each simulation, to set different parameters, change the variables:
M_f: to set the number of frequencies
M_1 and M_2: to set the number of antennas. The array is URA, so M_1 and M_2 are the number of rows and columns of the array
num_observations: to change the number of observations

# The physical parameters
"Delays", "elevations" and "azimuths" variables are fixed during the simulations since we want to understand the effect of changing the aperture dimensions. But, it can be changed to verify that the conclusions about the estimator (how it behaves with a varying number of frequencies, antennas and LoS/NLoS ratio) remain the same. The "Delays" variable only make sense if we try to estimate multiple signals, which is not the case.
