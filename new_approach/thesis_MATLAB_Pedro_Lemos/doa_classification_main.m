addpath estimator/

%% Clean variables and close windows
clear; clc; close all;

%% Simulate channel with different rice factors (K) and same SNR

% physical parameters
delays = 10*1e-9;
elevations = 30*(pi/180);
azimuths = 20*(pi/180);

% Aperture dimensions
M_1 = 10; % frequency related
M_2 = 4; % spatial related
M_3 = 4; % spatial related
dimensions = [M_1; M_2; M_3];

% number of observations per channel condition. Ex.: If 2 values of K, then
% 200 observations
num_observations = 100; 

K_high = 4;
weight = sqrt(K_high/(K_high+1)); % [W^(0.5)]
sig_rayleigh = 1/(K_high+1); % [W]

K_low = 1;
weight_low = sqrt(K_low/(K_low+1)); % [W^(0.5)]
sig_rayleigh_low = 1/(K_low+1); % [W]

measurement_noise_power = 1e-7; % [W]
K_rice_high = weight^2/sig_rayleigh;
K_rice_low = weight_low^2/sig_rayleigh_low;

%% Make sure that we keep the same SNR in both conditions
desired_rssi = 1e-6; % [W]
alpha_high_k = sqrt(desired_rssi/(weight^2 + sig_rayleigh));
alpha_low_k = sqrt(desired_rssi/(weight_low^2 + sig_rayleigh_low));

%% The signal model
parameters = parameter_mapping([delays;elevations;azimuths], "physical");

smc = specular_model(parameters, dimensions)*weight + wgn(M_1*M_2*M_3,1, measurement_noise_power, 'linear', 'complex');
smc_low = specular_model(parameters, dimensions)*weight_low + wgn(M_1*M_2*M_3,1, measurement_noise_power, 'linear', 'complex');

rssis_dbm_high_k = zeros(num_observations, 1);
rssis_dbm_low_k = zeros(num_observations, 1);

X_high_k = cell([num_observations 1]);
X_low_k = cell([num_observations 1]);
for n=1:num_observations

    % Generate measurement with high K
    X_high_k{n} = alpha_high_k*(smc + wgn(M_1*M_2*M_3,1, sig_rayleigh, 'linear', 'complex'));
    rayleigh_high_k = alpha_high_k*wgn(M_1*M_2*M_3,1, sig_rayleigh, 'linear', 'complex');
    rssis_dbm_high_k(n) = 10*log10(mean(abs(X_high_k{n}(1:4)).^2)) + 30;
    
    % Generate measurement with low K
    X_low_k{n} = alpha_low_k*(smc_low + wgn(M_1*M_2*M_3,1, sig_rayleigh_low, 'linear', 'complex'));
    rssis_dbm_low_k(n) = 10*log10(mean(abs(X_low_k{n}(1:4)).^2)) + 30;
end

mean_high_k_rssi = 10*log10(mean(10.^((rssis_dbm_high_k - 30)/10))) + 30;
mean_low_k_rssi = 10*log10(mean(10.^((rssis_dbm_low_k - 30)/10))) + 30;
%% Check that the observed fading comes from Ricean distribution (not possible without the DMC)
% rssis = 10.^((rssis_dbm_high_k - 30)/10);
% mean_rssi = mean(rssis);
% h = sqrt(rssis/mean_rssi);
% histogram(h)
% k = histfit(h,20,'rician');

%% Estimate channel and K factor (using simpler estimator)

los_high_k = cell([num_observations 1]);
los_weight_high_k = cell([num_observations 1]);

los_low_k = cell([num_observations 1]);
los_weight_low_k = cell([num_observations 1]);
est_high_k = zeros([num_observations 1]);
est_low_k = zeros([num_observations 1]);
for n=1:num_observations
    [los_high_k{n}, los_weight_high_k{n}, ~, ~, est_high_k(n)] = scored_estimator(X_high_k{n}, dimensions);
    [los_low_k{n}, los_weight_low_k{n},  ~, ~, est_low_k(n)] = scored_estimator(X_low_k{n}, dimensions);
end

% Check direction error
rp = load_receiver_parameters;
k = 2*pi*(rp.d/rp.lam);
dir_fun = @(mu) (1/k)*[mu(2); mu(3); sqrt(k^2 - mu(2)^2 + mu(3)^2)];

real_dir = dir_fun(parameters);
dirs_high_k = cellfun(@(param) dir_fun(param), los_high_k, 'UniformOutput', false);
dirs_low_k = cellfun(@(param) dir_fun(param), los_low_k, 'UniformOutput', false);

dir_error_high_k = arrayfun(@(n) (180/pi)*real(acos(dirs_high_k{n}.'*real_dir/(norm(dirs_high_k{n})*norm(real_dir)))), 1:num_observations).';
dir_error_low_k = arrayfun(@(n) (180/pi)*real(acos(dirs_low_k{n}.'*real_dir/(norm(dirs_low_k{n})*norm(real_dir)))), 1:num_observations).';

%% Check the correlation with LoS direction estimate error
scores_high_k = cellfun(@(los_estimate) los_estimate(end), los_high_k);
scores_low_k = cellfun(@(los_estimate) los_estimate(end), los_low_k);

corr_rssi_high_k = xcorr(10.^(rssis_dbm_high_k), dir_error_high_k, 0, 'normalized');
corr_score_high_k = xcorr(scores_high_k, dir_error_high_k, 0, 'normalized');

corr_rssi_low_k = xcorr(10.^(rssis_dbm_low_k), dir_error_low_k, 0, 'normalized');
corr_score_low_k = xcorr(scores_low_k, dir_error_low_k, 0, 'normalized');

%% Classification step. Eliminate measurements based on K estimate and some arbitrary threshold. How much do we miss?
K_threshold = K_high*1.1;
est_k = [est_high_k; est_low_k];
dir_errors = [dir_error_high_k; dir_error_low_k];
dir_errors_cleaned = dir_errors(est_k > K_threshold);

%% Write results to "output" folder as csv files
unclassified_dir_errors_file = "output/dir_errors_before_CL"+ ".csv";
classified_dir_errors_file = "output/dir_errors_after_CL"+ ".csv";
estimated_ks_file = "output/estimated_ks.csv";

writematrix(dir_errors, unclassified_dir_errors_file);
writematrix(dir_errors_cleaned, classified_dir_errors_file);
writematrix(est_k, estimated_ks_file);

%% Generate figures

% K estimate figure
hold off; hold on;

plot([K_high*ones([num_observations,1]); (K_low)*ones([num_observations,1])], 'LineWidth', 2);
plot(readmatrix(estimated_ks_file), 'LineWidth', 2);

xlabel('Sample', 'FontSize', 12);
ylabel('K estimate', 'FontSize', 12);

% Legend
legend({'Real', 'Estimated'}, ...
       'Interpreter', 'latex', 'FontSize', 10, 'Location', 'northeast');

% Grid and axis styles
grid on; ax = gca; ax.FontSize = 10; ax.LineWidth = 1.2; ax.Box = 'on';

set(gcf, 'Color', 'w'); axis tight;
print('figs/estimated_ks','-depsc','-painters','-r300');

% Dir error CDF
figure
hold off; hold on;
h = cdfplot(readmatrix(unclassified_dir_errors_file)); set(h, 'LineWidth', 2); title('');
h = cdfplot(readmatrix(classified_dir_errors_file)); set(h, 'LineWidth', 2); title('');

% Grid and axis styles
grid on; ax = gca; ax.FontSize = 10; ax.LineWidth = 1.2; ax.Box = 'on';

ylabel('P(X < x)', 'FontSize', 12);
xlabel('DoA error (º)', 'FontSize', 12);

% Legend
legend({'Without Classification', 'With Classification'}, ...
       'Interpreter', 'latex', 'FontSize', 10, 'Location', 'southeast');

set(gcf, 'Color', 'w'); axis tight;
print('figs/dir_error_cdf','-depsc','-painters','-r300');