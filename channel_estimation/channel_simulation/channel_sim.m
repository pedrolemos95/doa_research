add_paths()

%% Clean variables and close windows

clear
clc
close all

%% MEASUREMENT SETUP
% aperture dimensions
M_1 = 40; % frequency related
M_2 = 4; % spatial related
M_3 = 4; % spatial related

% set trajectory
speed = 0.5; % [m/s]
sampling_time = 500e-3; % The interval between channel observations [s]
x_inc = speed*sampling_time;
y_inc = speed*sampling_time;
% x_pos = 0:x_inc:40;

N = 100;

y_pos = [linspace(1,12,50), 12*ones(1,50), 12*ones(1,100)];
x_pos = [zeros(1,50), linspace(1,12,50), 12*ones(1,100)];

tx_pos = [1; 1; 1] + [x_pos; y_pos; zeros(1,numel(x_pos))];
% tx_pos = [1; 1; 1] + [zeros(1,N); zeros(1,N); zeros(1,N)];


% tx_pos = repmat([5;2;0.5], [1 10]);

rx_pos = [12;9;3.0];

rp = load_receiver_parameters;

% rp.f0 = 1e6;

% Pre-visualization
map_file = "office_map.stl";

viewer_pre = siteviewer("SceneModel", load_map(map_file));
tx_pre = txsite("cartesian", "AntennaPosition", tx_pos(:,1), "TransmitterFrequency",2.402e9);
rx_pre = rxsite("cartesian", "AntennaPosition", rx_pos);
show(tx_pre);
show(rx_pre);

%% Raytracing simulation

% simulate propagation rays
num_observations = numel(tx_pos)/3;
[rays,tx,rx] = simulate_propagation(map_file, tx_pos, rx_pos, 1, num_observations);
strong_rays = remove_weak_rays(rays, -40);
% rays = get_only_specular_rays(rays, rp.noise_power);

%% Channel observation

dimensions = [M_1; M_2; M_3];
% X = arrayfun(@(n) generate_channel_observation(rays{n}, dimensions), 1:num_observations, 'UniformOutput', false);
[X,X_dmc,smc_parameters,weights,dmc_parameters] = arrayfun(@(n) generate_channel_observation_with_dmc(strong_rays{n}, dimensions), 1:num_observations, 'UniformOutput', false);

%% Raw PDP estimate
Xrz = reshape(X{11}, [M_1 M_2*M_3]);

Xdmc = reshape(X_dmc{11}, [M_1 M_2*M_3]);

F = (1/sqrt(M_1))*dftmtx(M_1);

Xt = F'*Xrz;
Xt_dmc = F'*Xdmc;

N = numel(Xt(1,:));
ht = sum(Xt.*conj(Xt),2)/N;

N = numel(Xt_dmc(1,:));
ht_dmc = sum(Xt_dmc.*conj(Xt_dmc),2)/N;

% close all;
% plot(10*log10(abs(ht)));
% hold on;
% plot(10*log10(abs(ht_dmc)));
% plot(sqrt(ht));

%% View rays
% viewer = siteviewer("SceneModel", load_map(map_file));
% plot(strong_rays{1}{1}{1});

%% Visualize channel parameters?
% rtchan = comm.RayTracingChannel(strong_rays{1}{1}{1},tx,rx);
% showProfile(rtchan);

%% Estimate channel

path_parameters = cell([1 num_observations]);
path_weights = cell([1 num_observations]);
dmc_estimates = cell([1 num_observations]);
remainders = cell([1 num_observations]);

% Get the L.O.S. parameters
los_ray = rays{1}{1}{1}(1);
delay = los_ray.PropagationDelay;
aoa = (pi/180)*los_ray.AngleOfArrival;

path_parameters{1} = parameter_mapping([delay;flip(aoa)], "physical");
path_weights{1} = sqrt(10^(-(los_ray.PathLoss)/10))*exp(-1i*los_ray.PhaseShift);

% Get first DMC estimate
dmc_only_observation = X{1} - specular_model(path_parameters{1}, dimensions)*path_weights{1};
dmc_estimates{1} = raw_dmc_estimate(reshape(dmc_only_observation, [M_1 M_2*M_3]));

noise_covariance = 1e-12*eye(M_1*M_2*M_3);

% for n=2:num_observations
%     channel_observation = X{n};
% 
%     [path_parameters{n}, path_weights{n}, remainders{n}, dmc_estimates{n}] = rimax_iteration_full(channel_observation, path_parameters{n-1}, ...
%     path_weights{n-1}, noise_covariance, dimensions, dmc_estimates{n-1});
% end

los_candidates = cell([1 num_observations]);
another_los_candidates = cell([1 num_observations]);
los_weights = cell([1 num_observations]);
est_weight = cell([1 num_observations]);
pos_estimates = cell([1 num_observations]);
another_pos_estimates = cell([1 num_observations]);
dir_error = zeros(num_observations,1);
another_dir_error = zeros(num_observations,1);
scores = zeros(num_observations, 1);
another_scores = zeros(num_observations, 1);
true_scores = zeros(num_observations, 1);
rssis = zeros(num_observations, 1);
for n=1:num_observations
    channel_observation = X{n};

    % Scale the measurement. This makes things numerically easier.
    scaling_factor = norm(channel_observation);
    channel_observation = channel_observation/scaling_factor;

    [los_candidates{n}, los_weights{n}, path_parameters{n}, path_weights{n}] = scored_estimator(channel_observation, dimensions);
    path_weights{n} = path_weights{n}*scaling_factor;

    [another_los_candidates{n}, ~, ~, ~] = simpler_scored_estimator(channel_observation, dimensions);
    another_scores(n) = another_los_candidates{n}(end);

    scores(n) = los_candidates{n}(end);

    % rssis(n) = ;
    % Get the LoS estimate error
    los_ray = strong_rays{n}{1}{1}(1);
    delay = los_ray.PropagationDelay;
    aoa = (pi/180)*los_ray.AngleOfArrival;
    los_parameters = parameter_mapping([delay;flip(aoa)], "physical");

    k = 2*pi*(rp.d/rp.lam);
    
    los_dir_x = (1/k)*los_parameters(2);
    los_dir_y = (1/k)*los_parameters(3);
    los_dir_z = sqrt(1 - los_dir_x^2 + los_dir_y^2);
    los_dir = [los_dir_x; los_dir_y; los_dir_z];

    est_dir_x = (1/k)*los_candidates{n}(2);
    est_dir_y = (1/k)*los_candidates{n}(3);
    est_dir_z = sqrt(1 - est_dir_x^2 + est_dir_y^2);
    est_dir = [est_dir_x; est_dir_y; est_dir_z];

    another_est_dir_x = (1/k)*another_los_candidates{n}(2);
    another_est_dir_y = (1/k)*another_los_candidates{n}(3);
    another_est_dir_z = sqrt(1 - another_est_dir_x^2 + another_est_dir_y^2);
    another_est_dir = [another_est_dir_x; another_est_dir_y; another_est_dir_z];

    dir_error(n) = (180/pi)*acos((los_dir.')*(est_dir)/(norm(los_dir)*norm(est_dir)));
    another_dir_error(n) = (180/pi)*acos((los_dir.')*(another_est_dir)/(norm(los_dir)*norm(another_est_dir)));

    % Since the normalized parameters already gives us the direction, it is
    % pretty straightforward to convert it to a position

    mu_2 = los_candidates{n}(2);
    mu_3 = los_candidates{n}(3);
    rx_height = rx_pos(3);
    tx_height = tx_pos(3,1);
    delta_z = rx_height - tx_height;
    t = delta_z/sqrt(1 - (mu_2^2 + mu_3^2)/k^2);

    pos_est = rx_pos + (t/k)*[mu_2;mu_3;0];
    pos_est(3) = tx_height;

    pos_estimates{n} = real(pos_est);


    mu_2 = another_los_candidates{n}(2);
    mu_3 = another_los_candidates{n}(3);
    rx_height = rx_pos(3);
    tx_height = tx_pos(3,1);
    delta_z = rx_height - tx_height;
    t = delta_z/sqrt(1 - (mu_2^2 + mu_3^2)/k^2);

    pos_est = rx_pos + (t/k)*[mu_2;mu_3;0];
    pos_est(3) = tx_height;

    another_pos_estimates{n} = real(pos_est);
end

%% Visualize the estimates

viewer = siteviewer("SceneModel", load_map(map_file));
show(tx);
show(rx);

% plot(strong_rays{1}{1}{1});
plot_positions(pos_estimates, "red");
plot_positions(another_pos_estimates, "blue");

%% Generate simulation report
simulation_info = sprintf("Tx Position: x: %.2f y: %.2f z:%.2f\n", tx_pos(1), tx_pos(2), tx_pos(3));
simulation_info = strcat(simulation_info, sprintf("Rx Position: x: %.2f y: %.2f z: %.2f\n", rx_pos(1,1), rx_pos(2,1), rx_pos(3,1)));

% Mean squared error
simulation_info = strcat(simulation_info, sprintf("Average squared dir error (deg): %.2f\n", mean(dir_error().^2)));
simulation_info = strcat(simulation_info, sprintf("Average score: %.2f\n", mean(scores)));

% Calculate position errors
pos_error = arrayfun(@(n) norm(pos_estimates{n} - tx_pos(:,n)), 1:num_observations);
another_pos_error = arrayfun(@(n) norm(another_pos_estimates{n} - tx_pos(:,n)), 1:num_observations);

% Graphs
figure(1)
plot(1:num_observations, dir_error, '-o', 'LineWidth', 2);
title("Direction Error");
xlabel("Observation index");
ylabel("Dir error squared (deg)");
graph_1 = gca;

figure(2)
plot(1:num_observations, scores, '-o', 'LineWidth', 2);
title("Scores");
xlabel("Observation index");
ylabel("Score (%)");
graph_2 = gca;


figure(3)
plot(1:num_observations, pos_error, '-o', 'LineWidth', 2);
title("Position error (distance)");
xlabel("Observation index");
ylabel("Error (m)");
graph_3 = gca;

figure(4)
plot(1:num_observations, another_pos_error, '-o', 'LineWidth', 2);
title("Position error simple method (distance)");
xlabel("Observation index");
ylabel("Error (m)");
graph_4 = gca;

figure(5)
plot(1:num_observations, another_scores, '-o', 'LineWidth', 2);
title("Score simple method");
xlabel("Observation index");
ylabel("Score (%)");
graph_5 = gca;

figure(6)
plot(1:num_observations, another_dir_error, '-o', 'LineWidth', 2);
title("Dir error simple method (deg)");
xlabel("Observation index");
ylabel("Error (deg)");
graph_6 = gca;

% Write to files
sim_name = "los_simulation";
export_simulation_report(simulation_info, [graph_1,graph_2, graph_3]);