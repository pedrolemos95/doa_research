function [path_parameters, path_weights, remainder, dmc_estimate] = rimax_iteration_full(varargin)
    % INPUT: ["channel_observation", "parameters", "weights",
    % "noise_covariance", "dimensions"]. OUTPUT: path_parameters,
    % path_weights
    if isempty(varargin)
        run_unitary_test();
        return;
    end

    [channel_observation, parameters, weights, noise_covariance, dimensions, dmc_parameters] = parse_input_parameters(varargin, ...
        ["channel_observation", "parameters", "weights", "noise_covariance", "dimensions", "dmc_parameters"]);

    M1 = dimensions(1);
    M2 = dimensions(2);
    M3 = dimensions(3);

    % estimate new weights
    % weights = weights_estimation(channel_observation, parameters, noise_covariance, dimensions);
    weights = weights_estimation(channel_observation, parameters, full_covariance_matrix(dmc_parameters, 1, dimensions), dimensions);

    % get new path_parameters
    % [new_path_parameter, new_path_weight] = new_path_estimation(channel_observation, parameters, ...
    %     weights, noise_covariance, dimensions);
    [new_path_parameter, new_path_weight] = new_path_estimation(channel_observation, parameters, ...
        weights, full_covariance_matrix(dmc_parameters, 1, dimensions), dimensions);

    % decide if this parameter should be appended or not
    parameters_tmp = [reshape(parameters, [numel(parameters)/3 3]); new_path_parameter.'] ;
    parameters_tmp = parameters_tmp(:);
    weights_tmp = [weights; new_path_weight];
    % [~, FIMn] = complete_model_hessian(parameters_tmp, weights_tmp, noise_covariance, dimensions);
    [~, FIMn] = complete_model_hessian(parameters_tmp, weights_tmp, full_covariance_matrix(dmc_parameters, 1, dimensions), dimensions);
    FIMnv = FIMn - eye(size(FIMn));
    add_measurement = max(abs(FIMnv(:))) < 0.6;
    if add_measurement
        parameters = parameters_tmp;
        weights = weights_tmp;
    end

    % append new estimate to specular parameters
    % parameters = [reshape(parameters, [numel(parameters)/3 3]); new_path_parameter.'] ;
    % parameters = parameters(:);
    % weights = [weights; new_path_weight];

    paths_discarded = true;
    while paths_discarded

        % improve the specular estimates
        % [parameters, weights] = specular_component_estimation(channel_observation, ...
        %     parameters, weights, noise_covariance, dimensions);
    
        [parameters, weights] = specular_component_estimation(channel_observation, ...
            parameters, weights, full_covariance_matrix(dmc_parameters, 1, dimensions), dimensions);

        dmc_only_observation =  channel_observation - specular_model(parameters, dimensions)*weights;

        dmc_parameters = dense_component_estimation(reshape(dmc_only_observation, [M1 M2*M3]), dmc_parameters);

        % check the reliability of the propagation paths
        % FIM = complete_model_hessian(parameters, weights, noise_covariance, dimensions);
        FIM = complete_model_hessian(parameters, weights, full_covariance_matrix(dmc_parameters, 1, dimensions), dimensions);
        CRB = diag(inv(FIM));
        CRB_g = CRB(3*numel(weights)+1:4*numel(weights)) + CRB(4*numel(weights)+1:5*numel(weights));
        SNRS = (abs(weights).^2)./CRB_g;
   
        % discard low SNR paths
        pm = reshape(parameters, [numel(parameters)/3 3]);

        reamining_paths = numel(pm(:,1));
        num_paths_to_be_eliminated = numel(pm(SNRS < 10.6724, 1));

        % we cannot eliminate all paths! At least one has to stay. 
        % Leave the path with highest SNR.
        if (num_paths_to_be_eliminated == reamining_paths)
            [~, max_snr_idx] = max(SNRS);
            parameters = pm(max_snr_idx, :);
            parameters = parameters(:);
            weights = weights(max_snr_idx);
    
            paths_discarded = true;
        else
            parameters = pm(SNRS > 10.6724, :);
            parameters = parameters(:);
            weights = weights(SNRS > 10.6724);
    
            paths_discarded = any(SNRS < 10.6724);
        end
    end

    path_parameters = parameters;
    path_weights = weights;
    dmc_estimate = dmc_parameters;
    remainder = channel_observation - specular_model(path_parameters, dimensions)*path_weights; 
end

function run_unitary_test()
    % test_data = readmatrix('multipath_parameters.csv');
    test_data = readmatrix('multipath_parameters_dynamic.csv');
    P = numel(test_data(:,1))/4;
    
    test_data(1:P,:) = test_data(1:P,:)*1e-9;
    test_data(P+1:2*P,:) = (pi/180)*test_data(P+1:2*P,:);
    test_data(2*P+1:3*P,:) = (pi/180)*test_data(2*P+1:3*P,:);

    num_observations = numel(test_data(1,:));

    multipath_parameters = arrayfun(@(n) parameter_mapping(test_data(1:3*P,n), "physical"), ...
        1:num_observations, 'UniformOutput', false);
    multipath_parameters = [multipath_parameters{:}];
    multipath_weights = test_data(3*P+1:4*P,:);

    M1 = 40; M2 = 4; M3 = 4;
    dimensions = [M1;M2;M3];
    noise_power = 1e-12;
    noise_covariance = noise_power*eye(M1*M2*M3);

    dmc_power = 1;
    channel_coherence_bandwidth = 6e6;
    f0 = 1e6;
    beta = channel_coherence_bandwidth/(M1*f0);
    tau_d = 0;
    dmc_parameters = [dmc_power, beta, tau_d];

    path_parameters = cell([1 num_observations]);
    path_weights = cell([1 num_observations]);
    dmc_estimates = cell([1 num_observations]);
    remainders = cell([1 num_observations]);

    % path_parameters{1} = multipath_parameters(:,1);
    % path_weights{1} = multipath_weights(:,1);

    % initialize algorithm
    path_parameters{1} = multipath_parameters([1;P+1;2*P+1],1);
    path_weights{1} = multipath_weights(1,1);

    [~, dmc] = dmc_model(dmc_parameters, M1, M2*M3);
    dmc_only_observation = specular_model(multipath_parameters(:,1), dimensions)*multipath_weights(:,1) + ...
                        wgn(M1*M2*M3,1, noise_power, 'linear') - ...
                        specular_model(path_parameters{1}, dimensions)*path_weights{1} + ...
                        dmc(:);

    dmc_estimates{1} = raw_dmc_estimate(reshape(dmc_only_observation, [M1 M2*M3]));
    for n=2:num_observations
        channel_observation = specular_model(multipath_parameters(:,n), dimensions)*multipath_weights(:,n) + ...
                                wgn(M1*M2*M3,1, noise_power, 'linear');

        smc = channel_observation;

        [~, dmc] = dmc_model(dmc_parameters, M1, M2*M3);

        channel_observation = channel_observation + dmc(:);

        coarse_pdp_estimate = raw_pdp_estimate(smc, [M1;M2;M3]);

        [path_parameters{n}, path_weights{n}, remainders{n}, dmc_estimates{n}] = rimax_iteration_full(channel_observation, path_parameters{n-1}, ...
            path_weights{n-1}, noise_covariance, dimensions, dmc_estimates{n-1});
        
    end
end