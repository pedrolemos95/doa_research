function [parameters, weight, score] = scored_estimator(varargin)
    % INPUT: channel_observation. OUTPUT: parameters, path_weight, score
    % Reference: Sec 5.3 from Richter
    if isempty(varargin)
        run_unitary_test();
        return;
    end
    
    [X, dim] = parse_input_parameters(varargin, ["channel_observation", "dimensions"]);

    % first, apply subarray smoothing to decorrelate the signals. It should
    % be done only in the frequency domain. 
    % TODO: Should the smoothing be applied to stay with 2M1/3 + 1 size or size 1?

    % All we have to do is to rearrange the observation.
    M1 = dim(1);
    M2 = dim(2);
    M3 = dim(3);
    M = M1*M2*M3;

    % TODO: Test if its not great than M
    M1_ss = 1;
    X_ss = reshape(X, [M1 M2*M3]).';

    dim_ss = [M1_ss;M2;M3];
    [parameters, weight] = esprit(X_ss, dim_ss);

    % Remove the path estimate from the observation, in order to get an
    % residue
    X_res = X - specular_model(parameters, dim)*weight;

    % The score is a measurement of the relation between the power of the
    % estimated path and the other components. If it is too low the estimate is bad.
    % However, if it is high, it is a indicative that the estimate fits the
    % measurement very well and, therefore, must be a good esimate.

    % If the residue norm increases the power, we discard the
    % estimate. It's probably a very bad estimate
    if (norm(X_res) < norm(X))
        score = 1 - (norm(X_res)^2)/(norm(X)^2);
    else
        score = 0;
    end
end

function run_unitary_test()
    delays = [1e-9;30e-9];
    doas = [53; 15;20;80]*(pi/180);
    mu = parameter_mapping([delays;doas], "physical");
    path_weights = [1;0.1];

    % aperture dimensions
    M1 = 20; % number of frequencies
    M2 = 4; % number of rows in antenna array
    M3 = 4; % number of cols in antenna array
    dim = [M1;M2;M3];

    X = specular_model(mu, dim)*path_weights + wgn(M1*M2*M3,1,-20,'dBW');

    [est_mu, est_weight, score] = scored_estimator(X, dim);
end