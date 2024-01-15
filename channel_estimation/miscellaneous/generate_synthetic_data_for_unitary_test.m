function [channel_observation, parameters, weights, noise_covariance, dimensions] = generate_synthetic_data_for_unitary_test()
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
    parameters = normalized_parameters;
end