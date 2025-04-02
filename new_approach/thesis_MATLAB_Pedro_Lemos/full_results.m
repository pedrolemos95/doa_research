addpath estimator/

%% Clean variables and close windows
clear; clc; close all;

%% Simulation 1: Estimated vs Real K dependency on channels conditions (high and low K)

% Simulation setup BEGIN %
M_f = 10; % frequencies
M_1 = 4; % rows in the antenna array
M_2 = 4; % columns in the antenna array
num_observations = 100;
% Simulation setup END %

% Simulation block BEGIN %
K_high = 4;
K_threshold = 1; % Doesn't matter in this simulation
results = aoa_simulation(K_high, K_threshold, [M_f;M_1;M_2], num_observations);
est_ks_high = results("est_ks");

K_low = 1;
K_threshold = 1; % Doesn't matter in this simulation
results = aoa_simulation(K_low, K_threshold, [M_f;M_1;M_2], num_observations);
est_ks_low = results("est_ks");
% Simulation block END %

% Result analysis and export BEGIN %
real_ks = [K_high*ones(num_observations,1); K_low*ones(num_observations,1)];
estimated_ks = [est_ks_high; est_ks_low];

f = containers.Map();
f("figure_name") = "simulation_1";
f("graphs") = {real_ks, estimated_ks};
f("legends") = {'$$K$$', '$$\hat{K}$$'};
f("ylim") = [0 7];
f("xlabel") = "Sample";
f("ylabel") = "$\hat{K}$";
f("linestyles") = {'--', '-'};
create_figure(f);
% Result analysis and export END %

%% Simulation 2: Estimated vs Real K dependency on the number of frequencies
% Simulation setup BEGIN %
M_1 = 4; % rows in the antenna array
M_2 = 4; % columns in the antenna array
num_observations = 100;
K_high = 4;
K_low = 1;
% Simulation setup END %

% Simulation block BEGIN %
K_threshold = 1; % Doesn't matter in this simulation

% 4 frequencies
M_f = 4;
results = aoa_simulation(K_high, K_threshold, [M_f;M_1;M_2], num_observations);
est_ks_high = results("est_ks");

results = aoa_simulation(K_low, K_threshold, [M_f;M_1;M_2], num_observations);
est_ks_low = results("est_ks");
graph_1 = [est_ks_high; est_ks_low];

% 16 frequencies
M_f = 16;
results = aoa_simulation(K_high, K_threshold, [M_f;M_1;M_2], num_observations);
est_ks_high = results("est_ks");

results = aoa_simulation(K_low, K_threshold, [M_f;M_1;M_2], num_observations);
est_ks_low = results("est_ks");
graph_2 = [est_ks_high; est_ks_low];

% 64 frequencies
M_f = 64;
results = aoa_simulation(K_high, K_threshold, [M_f;M_1;M_2], num_observations);
est_ks_high = results("est_ks");

results = aoa_simulation(K_low, K_threshold, [M_f;M_1;M_2], num_observations);
est_ks_low = results("est_ks");
graph_3 = [est_ks_high; est_ks_low];

real_ks = [K_high*ones(num_observations,1); K_low*ones(num_observations,1)];

% Simulation block END %

% Result analysis and export BEGIN %

f = containers.Map();
f("figure_name") = "simulation_2";
f("graphs") = {real_ks, graph_1, graph_2, graph_3};
f("legends") = {'$$K$$', '$$\hat{F = 4}$$', '$$\hat{F = 16}$$', '$$\hat{F = 64}$$'};
f("ylim") = [0 10];
f("xlabel") = "Sample";
f("ylabel") = "$\hat{K}$";
f("linewidths") = {1,1,1,1};
create_figure(f);
% Result analysis and export END %

%% Simulation 3: Estimated K bias dependency on the number of frequencies
% Simulation setup BEGIN %
M_1 = 4; % rows in the antenna array
M_2 = 4; % columns in the antenna array
num_observations = 500;
K_sweep = 0.5:0.1:10; % The values of K in which we calculate the mean K estimate
K_threshold = 1; % Doesn't matter in this simulation
% Simulation setup END %

% Simulation block BEGIN %
% 4 frequencies
M_f = 4;
mean_ks = zeros(numel(K_sweep),1);
for index = 1:numel(K_sweep)
       results = aoa_simulation(K_sweep(index), K_threshold, [M_f;M_1;M_2], num_observations);
       mean_ks(index) = mean(results("est_ks"));
end
graph_1 = mean_ks;

% 16 frequencies
M_f = 16;
mean_ks = zeros(numel(K_sweep),1);
for index = 1:numel(K_sweep)
       results = aoa_simulation(K_sweep(index), K_threshold, [M_f;M_1;M_2], num_observations);
       mean_ks(index) = mean(results("est_ks"));
end
graph_2 = mean_ks;

% 64 frequencies
M_f = 64;
mean_ks = zeros(numel(K_sweep),1);
for index = 1:numel(K_sweep)
       results = aoa_simulation(K_sweep(index), K_threshold, [M_f;M_1;M_2], num_observations);
       mean_ks(index) = mean(results("est_ks"));
end
graph_3 = mean_ks;

% Simulation block END %

% Result analysis and export BEGIN %

f = containers.Map();
f("figure_name") = "simulation_3";
f("graphs") = {graph_1, graph_2, graph_3};
f("legends") = {'$$M_f=4$$', '$$M_f=16$$', '$$M_f=64$$'};
f("ylim") = [0 15];
f("xlabel") = "Sample";
f("ylabel") = "$\hat{K}$";
f("linestyles") = {'-', '--', '-'};
f("markers") = {'o','none','none'};
f("markersfacecolors") = {'r','none','none'};
create_figure(f);
% Result analysis and export END %

%% Simulation 4: Mean AoA error dependency on K threshold

%% Simulation X: AoA estimation error dependency on AoA elevation