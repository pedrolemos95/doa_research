function weights = weights_estimation(varargin)
    % INPUT: ["channel_observation", "parameters", "noise_covariance", "dimensions"].
    % OUTPUT: weights_estimate 
    % Reference: Best Linear Unbiased Estimator (BLUE). Eq. 5.5 of Richter
    if isempty(varargin)
        run_unitary_test();
        return;
    end

    [channel_observation, parameters, noise_covariance, dimensions]  = parse_input_parameters(varargin, ...
        ["channel_observation", "parameters", "noise_covariance", "dimensions"]);

    B = specular_model(parameters, dimensions);
    Rnn = noise_covariance;
    x = channel_observation;
    
    weights = inv(B'*inv(Rnn)*B)*B'*inv(Rnn)*x;

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

    noise_covariance = noise_power*eye(M1*M2*M3);
    estimate = weights_estimation(channel_observation, normalized_parameters, noise_covariance, dimensions);

end