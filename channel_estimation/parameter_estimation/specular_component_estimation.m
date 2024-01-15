function [parameters_estimate, weights_estimate] = specular_component_estimation(varargin)
    % INPUT: ["channel_observation", "initial_parameters", "initial_weights", "noise_covariance", "dimensions"].
    % OUTPUT: ["parameters_estimate", "weights_estimate"]
    if isempty(varargin)
        run_unitary_test();
        return;
    end

    [channel_observation, initial_parameters, initial_weights, noise_covariance, dimensions] = parse_input_parameters( ...
        varargin, ["channel_observation", "initial_parameters", "initial_weights", "noise_covariance", "dimensions"]);

    x0 = [initial_parameters; real(initial_weights); imag(initial_weights)];
    options = optimset('TolFun', 1e-2, 'TolX', 1e-2);

    fun = @(x) cost_function(channel_observation, x, noise_covariance, dimensions);

    estimate = fminsearch(fun, x0, options);
    
    P = numel(initial_weights);
    parameters_estimate = estimate(1:3*P);
    weights_estimate = estimate(3*P+1:4*P) + 1i*estimate(4*P+1:5*P);
end

function value = cost_function(channel_observation, parameters, noise_covariance, dimensions)
    P = numel(parameters)/5;

    nonlinear_parameters = parameters(1:3*P);
    weights = parameters(3*P+1:4*P) + 1i*parameters(4*P+1:5*P);

    x = channel_observation;
    s = specular_model(nonlinear_parameters, dimensions)*weights;
    Rnn = noise_covariance;

    value = (x-s)'*inv(Rnn)*(x-s);
end

function run_unitary_test()
    delays = [10e-9; 100e-9];
    doas = (pi/180)*[45;60;10;100];
    weights = [1; 1i];
    noise_power = 1e-9; % power in W

    physical_parameters = [delays; doas];
    normalized_parameters = parameter_mapping(physical_parameters, "physical");

    M1 = 4; M2 = 4; M3 = 4;
    dimensions = [M1;M2;M3];

    % Generate synthetic data
    s_sp = specular_model(normalized_parameters, dimensions)*weights;
    noise = wgn(M1*M2*M3, 1, noise_power, 'linear');
    channel_observation = s_sp + noise;

    % give an initial value for the estimate
    init_delays = [15e-9; 90e-9];
    init_doas = (pi/180)*[40;60;12;95];
    init_weights = [0.9; 0.8i];
    init_parameters = parameter_mapping([init_delays; init_doas], "physical");
    noise_covariance = noise_power*eye(M1*M2*M3);

    [parameters_estimate, weights_estimate] = specular_component_estimation(channel_observation, init_parameters, init_weights, noise_covariance, dimensions);
end