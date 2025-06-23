function results = aoa_simulation_new(K_high, K_threshold, dimensions, num_observations, angle_of_arrival)
    %% Simulate channel with different rice factors (K) and same SNR
    % physical parameters
    delays = 10*1e-9;
    elevations = angle_of_arrival(1);
    azimuths = angle_of_arrival(2);

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
    los_parameters = parameter_mapping([delays;elevations;azimuths], "physical");
    smc = specular_model(los_parameters, dimensions)*weight + wgn(M_1*M_2*M_3,1, measurement_noise_power, 'linear', 'complex');
   

    rssis_dbm = zeros(num_observations, 1);
    X = cell([num_observations 1]);
    for n=1:num_observations
        % Generate observation
        % X{n} = alpha*(smc + wgn(M_1*M_2*M_3,1, sig_rayleigh, 'linear', 'complex'));
        
        s_los = smc/norm(smc);

        rho = 0;             % base spatial‐correlation coeff

        M1 = M_1; M2 = M_2; M3 = M_3;
        Na = M2*M3;
        [Rows, Cols] = ndgrid(0:M2-1, 0:M3-1);
        Rows = Rows(:);  % Na×1
        Cols = Cols(:);  % Na×1

        % fill R_spat with 2D exponential decay: rho^(Δrow + Δcol)
        R_spat = zeros(Na);
        for p = 1:Na
            for q = 1:Na
                dr = abs(Rows(p) - Rows(q));
                dc = abs(Cols(p) - Cols(q));
                R_spat(p,q) = rho^(dr + dc);
            end
        end

        % normalize so each frequency‐slice has unit avg power
        R_spat = R_spat / trace(R_spat);

        %--- 3) generate NLOS samples --------------------
        L = chol(R_spat, 'lower');   % Na×Na
        h_nlos = zeros(Na, M1);
        for f = 1:M1
            z = (randn(Na,1) + 1j*randn(Na,1))/sqrt(2);
            h_nlos(:,f) = L * z;    % E[||h_nlos(:,f)||^2] = 1
        end
        h_nlos = reshape(h_nlos, Na*M1, 1);

        h_nlos = h_nlos / sqrt(M_1);  

        alpha_los = sqrt(K_high/(K_high+1));
        beta  = sqrt(1/(K_high+1));

        X{n} = alpha*(alpha_los*s_los + beta*h_nlos);
        % N = 20;
        % nlos_parameters = generate_indoor_nlos_channel(N, sqrt(sig_rayleigh));
        % smc_nlos = calculate_smc_nlos(nlos_parameters, dimensions);
        % X{n} = alpha*(smc + smc_nlos);

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
    real_dir = dir_fun(los_parameters);
    directions = cellfun(@(param) dir_fun(param), los_estimate, 'UniformOutput', false);
    dir_errors = arrayfun(@(n) (180/pi)*real(acos(directions{n}.'*real_dir/(norm(directions{n})*norm(real_dir)))), 1:num_observations).'; % AoA error definition

    %% Classification step. Eliminate measurements based on K estimate and some arbitrary threshold. How much do we miss?
    dir_errors_classified = dir_errors(estimated_ks > K_threshold);
    los_est_class = los_estimate(estimated_ks > K_threshold);

    %% Estimate position using a single locator (assuming height = 1m)
    rx_pos = [0;0;rp.height];
    rx_height = rp.height;
    tx_height = 1.0;
    dz = rx_height - tx_height;
    
    pos_estimate_fun = @(mu) real(rx_pos + [0;0;tx_height - rx_height] + (dz/(k*sqrt(1 - (mu(2)^2 + mu(3)^2)/k^2)))*[mu(2);mu(3);0]);
    
    tx_pos = pos_estimate_fun(los_parameters);
    % unclassified estimates
    pos_estimates = cellfun(@(parameter) pos_estimate_fun(parameter), los_estimate, 'UniformOutput', false);
    unclass_pos_estimates = [pos_estimates{:}];
    
    % classified estimates
    pos_estimates = cellfun(@(parameter) pos_estimate_fun(parameter), los_est_class, 'UniformOutput', false);
    class_pos_estimates = [pos_estimates{:}];

    %% Export results
    results = containers.Map();
    results("unclass_doa_error") = dir_errors;
    results("class_doa_error") = dir_errors_classified;
    results("los_ests") = los_estimate;
    results("est_ks") = estimated_ks;
    results("los_est_class") = los_est_class;
    results("sample_discard_ratio") = numel(los_est_class)/numel(los_estimate);
    results("unclass_estimated_pos") = unclass_pos_estimates;
    results("class_estimated_pos") = class_pos_estimates;
    results("tx_pos") = tx_pos;
    results("rx_pos") = rx_pos;
end

function smc_nlos = calculate_smc_nlos(nlos_parameters, dimensions)
    alpha_n = nlos_parameters.alpha;
    tau_n = nlos_parameters.delay;
    az_n = nlos_parameters.azimuth;
    el_n = nlos_parameters.elevation;

    M_f = dimensions(1);
    M_1 = dimensions(2);
    M_2 = dimensions(3);
    smc_nlos = zeros(M_f*M_1*M_2, 1);

    rp = load_receiver_parameters;

    for n=1:numel(alpha_n)
        nlos_param = parameter_mapping([tau_n(n); el_n(n); az_n(n)], "physical");
        smc_nlos = smc_nlos + specular_model(nlos_param, dimensions)*alpha_n(n);
    end
end