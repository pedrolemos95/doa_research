add_paths()

%% Clean variables and close windows

clear
clc
close all

%% MEASUREMENT SETUP
% aperture dimensions
M_1 = 100; % frequency related
M_2 = 32; % spatial related
M_3 = 32; % spatial related

% set trajectory
speed = 4; % [m/s]
sampling_time = 500e-3; % The interval between channel observations [s]
x_inc = speed*sampling_time;
x_pos = 0:x_inc:40;
tx_pos = [5;4;0.5] + [x_pos; zeros(1,numel(x_pos)); zeros(1,numel(x_pos))];

% tx_pos = repmat([5;2;0.5], [1 10]);

rx_pos = [20;7;4.0];

rp = load_receiver_parameters;

% rp.f0 = 1e6;

%% Raytracing simulation

% simulate propagation rays
num_observations = numel(tx_pos)/3;
map_file = "simulation_map_with_ground.stl";
[rays,tx,rx] = simulate_propagation(map_file, tx_pos, rx_pos, 1, num_observations);
strong_rays = remove_weak_rays(rays, -30);
% rays = get_only_specular_rays(rays, rp.noise_power);

%% Channel observation

dimensions = [M_1; M_2; M_3];
% X = arrayfun(@(n) generate_channel_observation(rays{n}, dimensions), 1:num_observations, 'UniformOutput', false);
[X,X_dmc,smc_parameters,weights,dmc_parameters] = arrayfun(@(n) generate_channel_observation_with_dmc(rays{n}, dimensions), 1:num_observations, 'UniformOutput', false);

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

close all;
plot(10*log10(abs(ht)));
hold on;
plot(10*log10(abs(ht_dmc)));
% plot(sqrt(ht));

%% View rays

% viewer = siteviewer("SceneModel", load_map(map_file));

% plot(strong_rays{1}{1}{1});

%% Visualize channel parameters?
rtchan = comm.RayTracingChannel(rays{1}{1}{1},tx,rx);
showProfile(rtchan);

%% Estimate channel
rimax_iteration