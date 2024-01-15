clear all
clc

add_paths()

%% MEASUREMENT SETUP
% aperture dimensions
M_1 = 16; % frequency related
M_2 = 4; % spatial related
M_3 = 4; % spatial related

% set trajectory
speed = 4; % [m/s]
sampling_time = 500e-3; % The interval between channel observations [s]
x_inc = speed*sampling_time;
x_pos = 0:x_inc:40;
tx_pos = [5;4;0.5] + [x_pos; zeros(1,numel(x_pos)); zeros(1,numel(x_pos))];

% tx_pos = repmat([5;2;0.5], [1 10]);

rx_pos = [20;7;4.0];

rp = load_receiver_parameters;

% simulate propagation rays
num_observations = numel(tx_pos)/3;
map_file = "simulation_map_with_ground.stl";
rays = simulate_propagation(map_file, tx_pos, rx_pos, 1, num_observations);
% rays = get_only_specular_rays(rays, rp.noise_power);

dimensions = [M_1; M_2; M_3];
X = arrayfun(@(n) generate_channel_observation(rays{n}, dimensions), 1:num_observations, 'UniformOutput', false);

%% process samples
% start with a good guess (the loss parameters)
delay = rays{1}{1}{1}(1).PropagationDelay;
doa = (pi/180)*rays{1}{1}{1}(1).AngleOfArrival;

path_parameters = cell([num_observations 1]);
path_weights = cell([num_observations 1]);
remainder = cell([num_observations 1]);

path_parameters{1} = parameter_mapping([delay;flip(doa)], "physical");
path_weights{1} = 1;

noise_covariance = eye(M_1*M_2*M_3)*rp.noise_power;
for n=2:num_observations
    [path_parameters{n}, path_weights{n}, remainder{n}] = rimax_iteration(X{n}, path_parameters{n-1}, path_weights{n-1}, noise_covariance, dimensions);
end

%% PLOT ANGLES

viewer = siteviewer("SceneModel", load_map(map_file));

%% clear and plot

clearMap(viewer);

n = 16;

norm(remainder{n})^2

initial_phys_param = parameter_mapping(path_parameters{1}, "normalized");
physical_parameters = parameter_mapping(path_parameters{n}, "normalized");
physical_parameters = reshape(physical_parameters, [numel(physical_parameters)/3 3]);
doas = physical_parameters(:,2:3);
weights = path_weights{n};

plot_angles(doas, weights, rx_pos, "blue");

[~, real_parameters,real_weights] = generate_channel_observation(rays{n}, dimensions);
real_physical_parameters = parameter_mapping(real_parameters, "normalized");
real_physical_parameters = reshape(real_physical_parameters, [numel(real_physical_parameters)/3 3]);
real_doas = real_physical_parameters(:,2:3);

plot_angles(real_doas, real_weights, rx_pos, "red");