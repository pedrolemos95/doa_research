addpath estimator/

%% Clean variables and close windows
clear; clc; close all;

%% Simulation 1: Estimated vs Real K dependency on channels conditions (high and low K)

% Simulation setup BEGIN %
M_f = 10; % frequencies
M_1 = 4; % rows in the antenna array
M_2 = 4; % columns in the antenna array
num_observations = 100;
angle_of_arrival = (pi/180)*[30;20]; % [elevation, azimuth]
% Simulation setup END %

% Simulation block BEGIN %
K_high = 4;
K_threshold = 1; % Doesn't matter in this simulation
results = aoa_simulation(K_high, K_threshold, [M_f;M_1;M_2], num_observations, angle_of_arrival);
est_ks_high = results("est_ks");

K_low = 1;
K_threshold = 1; % Doesn't matter in this simulation
results = aoa_simulation(K_low, K_threshold, [M_f;M_1;M_2], num_observations, angle_of_arrival);
est_ks_low = results("est_ks");
% Simulation block END %

% Result analysis and export BEGIN %
real_ks.y_data = [K_high*ones(num_observations,1); K_low*ones(num_observations,1)];
estimated_ks.y_data = [est_ks_high; est_ks_low];
graph_1.y_data = real_ks;
graph_2.y_data = estimated_ks;

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
angle_of_arrival = (pi/180)*[30;20]; % [elevation, azimuth]
num_observations = 100;
K_high = 4;
K_low = 1;
% Simulation setup END %

% Simulation block BEGIN %
K_threshold = 1; % Doesn't matter in this simulation

% 4 frequencies
M_f = 4;
results = aoa_simulation(K_high, K_threshold, [M_f;M_1;M_2], num_observations, angle_of_arrival);
est_ks_high = results("est_ks");

results = aoa_simulation(K_low, K_threshold, [M_f;M_1;M_2], num_observations, angle_of_arrival);
est_ks_low = results("est_ks");
graph_1.y_data = [est_ks_high; est_ks_low];

% 16 frequencies
M_f = 16;
results = aoa_simulation(K_high, K_threshold, [M_f;M_1;M_2], num_observations, angle_of_arrival);
est_ks_high = results("est_ks");

results = aoa_simulation(K_low, K_threshold, [M_f;M_1;M_2], num_observations, angle_of_arrival);
est_ks_low = results("est_ks");
graph_2.y_data = [est_ks_high; est_ks_low];

% 64 frequencies
M_f = 64;
results = aoa_simulation(K_high, K_threshold, [M_f;M_1;M_2], num_observations, angle_of_arrival);
est_ks_high = results("est_ks");

results = aoa_simulation(K_low, K_threshold, [M_f;M_1;M_2], num_observations, angle_of_arrival);
est_ks_low = results("est_ks");
graph_3.y_data = [est_ks_high; est_ks_low];

real_ks.y_data = [K_high*ones(num_observations,1); K_low*ones(num_observations,1)];

% Simulation block END %

% Result analysis and export BEGIN %

f = containers.Map();
f("figure_name") = "simulation_2";
f("graphs") = {real_ks, graph_1, graph_2, graph_3};
f("legends") = {'$$K$$', '$$F = 4$$', '$$F = 16$$', '$$F = 64$$'};
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
angle_of_arrival = (pi/180)*[30;20]; % [elevation, azimuth]
num_observations = 500;
K_sweep = 0.5:0.5:10; % The values of K in which we calculate the mean K estimate
K_threshold = 1; % Doesn't matter in this simulation
% Simulation setup END %

% Simulation block BEGIN %
% 4 frequencies
M_f = 4;
mean_ks = zeros(numel(K_sweep),1);
for index = 1:numel(K_sweep)
       results = aoa_simulation(K_sweep(index), K_threshold, [M_f;M_1;M_2], num_observations, angle_of_arrival);
       mean_ks(index) = mean(results("est_ks"));
end
graph_1.y_data = mean_ks;
graph_1.x_data = K_sweep;

% 16 frequencies
M_f = 16;
mean_ks = zeros(numel(K_sweep),1);
for index = 1:numel(K_sweep)
       results = aoa_simulation(K_sweep(index), K_threshold, [M_f;M_1;M_2], num_observations, angle_of_arrival);
       mean_ks(index) = mean(results("est_ks"));
end
graph_2.y_data = mean_ks;
graph_2.x_data = K_sweep;

% 64 frequencies
M_f = 64;
mean_ks = zeros(numel(K_sweep),1);
for index = 1:numel(K_sweep)
       results = aoa_simulation(K_sweep(index), K_threshold, [M_f;M_1;M_2], num_observations, angle_of_arrival);
       mean_ks(index) = mean(results("est_ks"));
end
graph_3.y_data = mean_ks;
graph_3.x_data = K_sweep;

% Simulation block END %

% Result analysis and export BEGIN %

f = containers.Map();
f("figure_name") = "simulation_3";
f("graphs") = {graph_1, graph_2, graph_3};
f("legends") = {'$$M_f=4$$', '$$M_f=16$$', '$$M_f=64$$'};
f("ylim") = [0 15];
f("xlabel") = "$K$";
f("ylabel") = "$\hat{K}$";
f("linestyles") = {'-', '--', '-'};
f("markers") = {'o','none','none'};
f("markersfacecolors") = {'b','none','none'};
create_figure(f);
% Result analysis and export END %

%% Simulation 4: Mean AoA error dependency on K
% Simulation setup BEGIN %
M_1 = 4; % rows in the antenna array
M_2 = 4; % columns in the antenna array
angle_of_arrival = (pi/180)*[30;20]; % [elevation, azimuth]
num_observations = 400; % The number of observations per channel condition (K)
K_sweep = 0.1:0.05:4; % The values of K in which we want the channel to assume
K_threshold = K_sweep; % The values of threholds with which we want to classify the estimates
% Simulation setup END %

% Simulation block BEGIN %
% 16 frequencies
M_f = 16;
mean_aoa_error = zeros(numel(K_sweep),1);
for index = 1:numel(K_sweep)
       results = aoa_simulation(K_sweep(index), 1, [M_f;M_1;M_2], num_observations, angle_of_arrival);
       % Calculate aoa error
       mean_aoa_error(index) = mean(results("unclass_doa_error"));
end
graph_1.y_data = mean_aoa_error;
graph_1.x_data = K_sweep;

% Simulation block END %

% Result analysis and export BEGIN %

f = containers.Map();
f("figure_name") = "simulation_4";
f("graphs") = {graph_1};
f("xlabel") = "$K$";
f("ylabel") = "Mean AoA Error";
f("tabletext") = sprintf('Elevation = %d (º)\nAzimuth = %d (º)\n', ceil((180/pi)*angle_of_arrival(1)), ceil((180/pi)*angle_of_arrival(2)));
create_figure(f);
% Result analysis and export END %

%% Simulation 5: Positioning demo
% Simulation setup BEGIN %
M_f = 10; % frequencies
M_1 = 4; % rows in the antenna array
M_2 = 4; % columns in the antenna array
K_threshold = 2;
num_observations = 200;
angle_of_arrival = (pi/180)*[60;20]; % [elevation, azimuth]
% Simulation setup END %

% Simulation block BEGIN %
K_high = 3;
results = aoa_simulation(K_high, K_threshold, [M_f;M_1;M_2], num_observations, angle_of_arrival);
unclass_estimated_pos_high_k = results("unclass_estimated_pos");
class_estimated_pos_high_k = results("class_estimated_pos");
tx_pos = results("tx_pos")+[0;0;0.15];% sum 0.15 in z to put it slightly above other positions
rx_pos = results("rx_pos");

K_low = 0.2;
results = aoa_simulation(K_low, K_threshold, [M_f;M_1;M_2], num_observations, angle_of_arrival);
unclass_estimated_pos_low_k = results("unclass_estimated_pos");
class_estimated_pos_low_k = results("class_estimated_pos");

unclass_estimated_pos = [unclass_estimated_pos_high_k, unclass_estimated_pos_low_k];
class_estimated_pos = [class_estimated_pos_high_k, class_estimated_pos_low_k] + [0;0;0.1]; % sum 0.1 in z to put it slightly above other positions

f = containers.Map();
f("figure_name") = "simulation_5_1";
f("graphs") = {unclass_estimated_pos, class_estimated_pos, rx_pos, tx_pos};
f("legends") = {"Without classification", "With classification", "Locator Position", "Real Position"};
f("markerssize") = {50,50,300,50};
f("xlabel") = "$X$";
f("ylabel") = "$Y$";
f("xlim") = [0 3];
f("ylim") = [-1 3];
create_3d_figure(f);

% Generate the cdf for the position error
unclass_x = unclass_estimated_pos(1,:);
unclass_y = unclass_estimated_pos(2,:);
class_x = class_estimated_pos(1,:);
class_y = class_estimated_pos(2,:);
error_unclass = sqrt(sum(([unclass_x; unclass_y] - tx_pos(1:2)).^2));
error_class = sqrt(sum(([class_x; class_y] - tx_pos(1:2)).^2));

% h = cdfplot(error_unclass); set(h, 'LineWidth', 2); title('');
% graph_1.y_data = h.YData;
% graph_1.x_data = h.XData;
[y_data, x_data] = ecdf(error_unclass);
graph_1.y_data = y_data;
graph_1.x_data = x_data;

[y_data, x_data] = ecdf(error_class);
graph_2.y_data = y_data;
graph_2.x_data = x_data;

f = containers.Map();
f("figure_name") = "simulation_5_2";
f("graphs") = {graph_1, graph_2};
f("legends") = {"Without classification", "With classification"};
f("xlabel") = "Position error (m)";
f("ylabel") = "$P(X < x)$";
create_figure(f);

%% Simulation 6: AoA estimation error dependency on AoA elevation
% Simulation setup BEGIN %
M_f = 16; % frequencies
M_1 = 4; % rows in the antenna array
M_2 = 4; % columns in the antenna array
K_threshold = 1;
num_observations = 500;
angle_of_arrival = (pi/180)*[60;20]; % [elevation, azimuth]
% Simulation setup END %

% Simulation block BEGIN %
elevations = (pi/180)*(0:2:90);

K = 0.1;
mean_aoa_errors = 1:length(elevations);
for el_idx = 1:numel(elevations)
    angle_of_arrival = [elevations(el_idx);angle_of_arrival(2)];
    results = aoa_simulation(K, K_threshold, [M_f;M_1;M_2], num_observations, angle_of_arrival);
    mean_aoa_errors(el_idx) = mean(results("unclass_doa_error"));
end
graph_1.y_data = mean_aoa_errors;
graph_1.x_data = (180/pi)*elevations;

K = 1;
mean_aoa_errors = 1:length(elevations);
for el_idx = 1:numel(elevations)
    angle_of_arrival = [elevations(el_idx);angle_of_arrival(2)];
    results = aoa_simulation(K, K_threshold, [M_f;M_1;M_2], num_observations, angle_of_arrival);
    mean_aoa_errors(el_idx) = mean(results("unclass_doa_error"));
end
graph_2.y_data = mean_aoa_errors;
graph_2.x_data = (180/pi)*elevations;

K = 4;
mean_aoa_errors = 1:length(elevations);
for el_idx = 1:numel(elevations)
    angle_of_arrival = [elevations(el_idx);angle_of_arrival(2)];
    results = aoa_simulation(K, K_threshold, [M_f;M_1;M_2], num_observations, angle_of_arrival);
    mean_aoa_errors(el_idx) = mean(results("unclass_doa_error"));
end
graph_3.y_data = mean_aoa_errors;
graph_3.x_data = (180/pi)*elevations;

% Simulation block END %
f = containers.Map();
f("figure_name") = "simulation_6";
f("graphs") = {graph_1, graph_2, graph_3};
f("legends") = {"$K=0.5$", "$K=1$", "$K=4$"};
f("xlabel") = "Elevation (º)";
f("ylabel") = "Mean AoA error (º)";
create_figure(f);

%% Simulation 7: Position estimation error dependency on AoA elevation
% Simulation setup BEGIN %
M_f = 16; % frequencies
M_1 = 4; % rows in the antenna array
M_2 = 4; % columns in the antenna array
K_threshold = 1;
num_observations = 500;
angle_of_arrival = (pi/180)*[60;20]; % [elevation, azimuth]
% Simulation setup END %

% Simulation block BEGIN %
elevations = (pi/180)*(0:1:90);

K = 0.5;
mean_pos_errors = 1:length(elevations);
for el_idx = 1:numel(elevations)
    angle_of_arrival = [elevations(el_idx);angle_of_arrival(2)];
    results = aoa_simulation(K, K_threshold, [M_f;M_1;M_2], num_observations, angle_of_arrival);

    unclass_estimated_pos = results("unclass_estimated_pos");
    unclass_x = unclass_estimated_pos(1,:);
    unclass_y = unclass_estimated_pos(2,:);
    error_unclass = sqrt(sum(([unclass_x; unclass_y] - tx_pos(1:2)).^2));

    mean_pos_errors(el_idx) = mean(error_unclass);
end
graph_1.y_data = mean_aoa_errors;
graph_1.x_data = (180/pi)*elevations;

K = 1;
mean_aoa_errors = 1:length(elevations);
for el_idx = 1:numel(elevations)
    angle_of_arrival = [elevations(el_idx);angle_of_arrival(2)];
    results = aoa_simulation(K, K_threshold, [M_f;M_1;M_2], num_observations, angle_of_arrival);
    
    unclass_estimated_pos = results("unclass_estimated_pos");
    unclass_x = unclass_estimated_pos(1,:);
    unclass_y = unclass_estimated_pos(2,:);
    error_unclass = sqrt(sum(([unclass_x; unclass_y] - tx_pos(1:2)).^2));

    mean_pos_errors(el_idx) = mean(error_unclass);

end
graph_2.y_data = mean_aoa_errors;
graph_2.x_data = (180/pi)*elevations;

K = 4;
mean_aoa_errors = 1:length(elevations);
for el_idx = 1:numel(elevations)
    angle_of_arrival = [elevations(el_idx);angle_of_arrival(2)];
    results = aoa_simulation(K, K_threshold, [M_f;M_1;M_2], num_observations, angle_of_arrival);

    unclass_estimated_pos = results("unclass_estimated_pos");
    unclass_x = unclass_estimated_pos(1,:);
    unclass_y = unclass_estimated_pos(2,:);
    error_unclass = sqrt(sum(([unclass_x; unclass_y] - tx_pos(1:2)).^2));

    mean_pos_errors(el_idx) = mean(error_unclass);
end
graph_3.y_data = mean_aoa_errors;
graph_3.x_data = (180/pi)*elevations;


f = containers.Map();
f("figure_name") = "simulation_7";
f("graphs") = {graph_1, graph_2, graph_3};
f("legends") = {"$K=0.5$", "$K=1$", "$K=4$"};
f("xlabel") = "Elevation (º)";
f("ylabel") = "Mean Position error (º)";
create_figure(f);
