addpath estimator/

%% Clean variables and close windows
clear; clc; close all;

%% Array response calculation
% channel parameters
delays = 10*1e-9;
elevations = 60*(pi/180);
azimuths = 20*(pi/180);
parameters = parameter_mapping([delays;elevations;azimuths], "physical");

% Aperture dimensions
M_1 = 4; % frequency related
M_2 = 4; % spatial related
M_3 = 4; % spatial related
dimensions = [M_1; M_2; M_3];

smc = specular_model(parameters, dimensions);
