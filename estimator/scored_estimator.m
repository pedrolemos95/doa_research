function [los_candidate, los_weight, parameters, weight, rice_k] = scored_estimator(varargin)
    % INPUT: channel_observation. OUTPUT: los_candidate, parameters, path_weight, score
    % Reference: Sec 5.3 from Richter
    if isempty(varargin)
        run_unitary_test();
        return;
    end
    
    [X, dim] = parse_input_parameters(varargin, ["channel_observation", "dimensions"]);

    % first, apply subarray smoothing to decorrelate the signals. It should
    % be done only in the frequency domain.
    % TODO: Should the smoothing be applied to stay with 2M1/3 + 1 size or size 1?
    % TODO: Use forward-backward averaging?
    % All we have to do is to rearrange the observation.
    M1 = dim(1);
    M2 = dim(2);
    M3 = dim(3);
    M = M1*M2*M3;

    X_ss = reshape(X, [M1 M2*M3]).';
    P = 1;

    dim = [1;M2;M3];
    [parameters, weight, nlos_power] = esprit(X_ss, dim, P);

    % The score is a measurement of the relation between the power of the
    % estimated path and the other components. If it is too low the estimate is bad.
    % However, if it is high, it is a indicative that the estimate fits the
    % measurement very well and, therefore, must be a good esimate.
    if (weight^2 < (norm(X)^2/M))
        score = (weight^2/(norm(X)^2/M));
    else
        score = 0;
    end

    los_candidate = [parameters; score];
    los_weight = weight;
    rice_k = los_weight^2/(mean(abs(X).^2) - los_weight^2);
end

function run_unitary_test()
    delays = [1e-9;30e-9];
    doas = [53; 15;20;70]*(pi/180);
    mu = parameter_mapping([delays;doas], "physical");
    path_weights = [1.8 - 1i*0.75;1.5*1i];

    % aperture dimensions
    M1 = 40; % number of frequencies
    M2 = 4; % number of rows in antenna array
    M3 = 4; % number of cols in antenna array
    dim = [M1;M2;M3];

    % Generate an observation for K estimate testing
    K = 5;
    log_weight = sqrt(K/(K+1));
    sig_rayleigh = 1/(K+1);
    delay_los = 1e-9;
    doas_los = [30;20];
    mu_los = parameter_mapping([delay_los; doas_los], "physical");

    smc = specular_model(mu_los, dim)*log_weight;
    rayleigh = wgn(M1*M2*M3, 1, sig_rayleigh, 'linear', 'complex');
    X = smc + rayleigh;

    % X = specular_model(mu, dim)*path_weights + wgn(M1*M2*M3,1,-50,'dBW');

    [los_candidate, los_weight, est_mu, est_weight, est_K] = scored_estimator(X, dim);
end