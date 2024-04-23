add_paths()

%% Clean variables and close windows
clear; clc; close all;

%% Simulation setup
% Receiver position
rx_pos = [12;9;3.0];

% Transmitter trajectory
N = 20;
y_pos = [linspace(1,12,50), 12*ones(1,50), 12*ones(1,100)];
x_pos = [zeros(1,50), linspace(1,12,50), 12*ones(1,100)];
tx_pos = [1; 1; 1] + [x_pos; y_pos; zeros(1,numel(x_pos))];
% tx_pos = [7; 5; 1] + [zeros(1,N); zeros(1,N); zeros(1,N)];
tx_height = 1.0; % in [m]
rx_height = rx_pos(3);

rp = load_receiver_parameters;

map_file = "office_map.stl";

%% Pre-visualization of channel layout
% viewer_pre = siteviewer("SceneModel", load_map(map_file));
% tx_pre = txsite("cartesian", "AntennaPosition", tx_pos(:,1), "TransmitterFrequency",2.402e9);
% rx_pre = rxsite("cartesian", "AntennaPosition", rx_pos);
% show(tx_pre);
% show(rx_pre);

%% Raytracing simulation

% simulate propagation rays
num_observations = numel(tx_pos)/3;
[rays,tx,rx] = simulate_propagation(map_file, tx_pos, rx_pos, 1, num_observations);
strong_rays = remove_weak_rays(rays, -40);

%% Extract rays simulation parameters
% Calculate RSSI
rssi_fun = @(rays_array) sum(arrayfun(@(ray) sqrt(10^(-ray.PathLoss/10))*exp(1i*ray.PhaseShift), rays_array));
rssis = arrayfun(@(n) 20*log10(abs(rssi_fun(strong_rays{n}{1}{1}))), 1:num_observations);
snrs = rssis - 10*log10(rp.noise_power);

score_fun = @(rays_array) abs(rssi_fun(rays_array(1))/rssi_fun(rays_array))^2;
real_scores = cellfun(@(rays_cell) score_fun(rays_cell{1}{1}), strong_rays).';
real_scores(real_scores > 1) = 0;

%% Simulate channel observation

% Aperture dimensions
M_1 = 40; % frequency related
M_2 = 4; % spatial related
M_3 = 4; % spatial related
dimensions = [M_1; M_2; M_3];

[X,X_dmc,smc_parameters,weights,dmc_parameters] = arrayfun(@(n) generate_channel_observation_with_dmc(strong_rays{n}, dimensions), 1:num_observations, 'UniformOutput', false);

%% Estimate RSSI and score from observation
est_rssis = arrayfun(@(n) 10*log10(sum(abs(reshape(X{n}, [M_3*M_2 M_1])).^2, 1)/(M_2*M_3)).', 1:num_observations, 'UniformOutput', false);
rssis_dmc = [est_rssis{:}];
% rssis_dmc = rssis_dmc(ceil(M_1/2),:);
rssis_dmc = rssis_dmc(6, :);

%% View rays and channel profile
% viewer = siteviewer("SceneModel", load_map(map_file));
% observation_index = 1;
% plot(strong_rays{observation_index}{1}{1});
% rtchan = comm.RayTracingChannel(strong_rays{observation_index}{1}{1},tx,rx);
% showProfile(rtchan);

%% Estimate channel

% estimated the parameters
[los_candidates, los_weights, path_parameters, path_weights] = cellfun(@(channel_obs) ...
    scored_estimator(channel_obs, dimensions), X, 'UniformOutput', false);

[simpler_los_candidates, ~, ~, ~] = cellfun(@(channel_obs) ...
    simpler_scored_estimator(channel_obs, dimensions), X, 'UniformOutput', false);

% Get estimated scores
scores = cellfun(@(los_cand) los_cand(end), simpler_los_candidates);

% Get estimated dirs
k = 2*pi*(rp.d/rp.lam);
dir_fun = @(mu) (1/k)*[mu(2); mu(3); sqrt(k^2 - mu(2)^2 + mu(3)^2)];

% Real dirs
real_parameters_fun = @(ray) parameter_mapping([ray.PropagationDelay;(pi/180)*flip(ray.AngleOfArrival)] , "physical");
real_dirs = cellfun(@(rays_cell) dir_fun(real_parameters_fun(rays_cell{1}{1}(1))), strong_rays, 'UniformOutput', false);

% Estimated dirs
estimated_dirs = cellfun(@(param) dir_fun(param), simpler_los_candidates, 'UniformOutput', false);

% Estimated positions
dz = rx_height - tx_height;
pos_estimate_fun = @(mu) real(rx_pos + [0;0;tx_height - rx_height] + (dz/(k*sqrt(1 - (mu(2)^2 + mu(3)^2)/k^2)))*[mu(2);mu(3);0]);
pos_estimates = cellfun(@(parameter) pos_estimate_fun(parameter), los_candidates, 'UniformOutput', false);
simpler_pos_estimates = cellfun(@(parameter) pos_estimate_fun(parameter), simpler_los_candidates, 'UniformOutput', false);

%% Visualize the estimates

% viewer = siteviewer("SceneModel", load_map(map_file));
% show(tx);
% show(rx);
% 
% % plot(strong_rays{1}{1}{1});
% plot_positions(pos_estimates, "red");
% plot_positions(simpler_pos_estimates, "blue");

%% Generate simulation report
% Calculate position errors
dir_error = arrayfun(@(n) (180/pi)*real(acos(estimated_dirs{n}.'*real_dirs{n}/(norm(estimated_dirs{n})*norm(real_dirs{n})))), 1:num_observations);
pos_error = arrayfun(@(n) norm(simpler_pos_estimates{n} - tx_pos(:,n)), 1:num_observations);

simulation_info = sprintf("Tx Position: x: %.2f y: %.2f z:%.2f\n", tx_pos(1), tx_pos(2), tx_pos(3));
simulation_info = strcat(simulation_info, sprintf("Rx Position: x: %.2f y: %.2f z: %.2f\n", rx_pos(1,1), rx_pos(2,1), rx_pos(3,1)));

% Mean squared error
simulation_info = strcat(simulation_info, sprintf("Average squared dir error (deg): %.2f\n", mean(dir_error().^2)));
simulation_info = strcat(simulation_info, sprintf("Average score: %.2f\n", mean(scores)));

% RSSI and Scores correlation
tmp = xcorr(1./pos_error, scores, 1, 'normalized');
scores_corr = tmp(1);
tmp = xcorr(1./pos_error, rssis_dmc, 1, 'normalized');
rssi_corr = tmp(1);

%% Graphs
% graph_1 = create_exportable_graph(1, 1:num_observations, dir_error, ...
%     ["Direction Error", "Observation index", "Dir error squared (deg)"]);
% 
% graph_2 = create_exportable_graph(2, 1:num_observations, scores, ...
%     ["Scores", "Observation index", "Score"]);
% 
% graph_3 = create_exportable_graph(3, 1:num_observations, pos_error, ...
%     ["Position error (distance)", "Observation index", "Error (m)"]);
% 
% graph_4 = create_exportable_graph(4, 1:num_observations, real_scores, ...
%     ["Real scores", "Observation index", "Score"]);
% 
% graph_5 = create_exportable_graph(5, 1:num_observations, rssis_dmc, ...
%     ["RSSI Values", "Observation index", "RSSI (dBW)"]);
% 
% % Write to files
% sim_name = "los_simulation";
% export_simulation_report(simulation_info, [graph_1, graph_2, graph_3, graph_4, graph_5]);