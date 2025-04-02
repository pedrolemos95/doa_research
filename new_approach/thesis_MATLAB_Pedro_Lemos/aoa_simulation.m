function results = aoa_simulation(K_high, K_threshold, dimensions, num_observations, angle_of_arrival)
    %% Simulate channel with different rice factors (K) and same SNR
    % physical parameters
    delays = 10*1e-9;
    elevations = angle_of_arrival(1);
    azimuths = angle_of_arrival(2);
    % elevations = 60*(pi/180);
    % azimuths = 20*(pi/180);

    % Aperture dimensions
    M_1 = dimensions(1); % frequency related
    M_2 = dimensions(2); % spatial related
    M_3 = dimensions(3); % spatial related

    weight = sqrt(K_high/(K_high+1)); % [W^(0.5)]
    sig_rayleigh = 1/(K_high+1); % [W]
        
    measurement_noise_power = 1e-7; % [W]
    %% Make sure that we keep the same SNR in both conditions
    desired_rssi = 1e-6; % [W]
    alpha = sqrt(desired_rssi/(weight^2 + sig_rayleigh));
    
    %% Generate observations based on the signal and channel model (Sec. II of paper)
    parameters = parameter_mapping([delays;elevations;azimuths], "physical");
    smc = specular_model(parameters, dimensions)*weight + wgn(M_1*M_2*M_3,1, measurement_noise_power, 'linear', 'complex');
    
    rssis_dbm = zeros(num_observations, 1);
    X = cell([num_observations 1]);
    for n=1:num_observations
        % Generate observation
        X{n} = alpha*(smc + wgn(M_1*M_2*M_3,1, sig_rayleigh, 'linear', 'complex'));
        rssis_dbm(n) = 10*log10(mean(abs(X{n}(1:4)).^2)) + 30;
    end
    
    mean_rssi = 10*log10(mean(10.^((rssis_dbm - 30)/10))) + 30; % This is just to check that the mean RSSI matches the desired RSSI
    %% Estimate AoA and K factor
    los_estimate = cell([num_observations 1]);
    los_weight = cell([num_observations 1]);
    
    estimated_ks = zeros([num_observations 1]);
    for n=1:num_observations
        [los_estimate{n}, los_weight{n}, ~, ~, estimated_ks(n)] = scored_estimator(X{n}, dimensions);
    end

    %% Calculate direction error
    rp = load_receiver_parameters;
    k = 2*pi*(rp.d/rp.lam);
    dir_fun = @(mu) (1/k)*[mu(2); mu(3); sqrt(k^2 - mu(2)^2 + mu(3)^2)];
    real_dir = dir_fun(parameters);
    directions = cellfun(@(param) dir_fun(param), los_estimate, 'UniformOutput', false);
    dir_errors = arrayfun(@(n) (180/pi)*real(acos(directions{n}.'*real_dir/(norm(directions{n})*norm(real_dir)))), 1:num_observations).'; % AoA error definition


    %% Classification step. Eliminate measurements based on K estimate and some arbitrary threshold. How much do we miss?
    dir_errors_classified = dir_errors(estimated_ks > K_threshold);
    los_est_class = los_estimate(estimated_ks > K_threshold);

    %% Export results
    results = containers.Map();
    results("unclass_doa_error") = dir_errors;
    results("class_doa_error") = dir_errors_classified;
    results("los_ests") = los_estimate;
    results("est_ks") = estimated_ks;
    results("los_est_class") = los_est_class;
    results("sample_discard_ratio") = numel(los_est_class)/numel(los_estimate);
end