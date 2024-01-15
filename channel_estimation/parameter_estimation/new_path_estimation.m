function [new_path_parameters, new_path_weight] = new_path_estimation(varargin)
    % INPUT: ["channel_observation", "parameters", "weights",
    % "noise_covariance", "dimensions"]. OUTPUT: new_path_estimate
    % Reference: Table 5-2 of Richter
    % WARNING: Right now it doesn't take into account the possibility of
    % non-diagonal noise covariance matrices. It should be changed in the
    % future
    if isempty(varargin)
        run_unitary_test();
        return;
    end
    
    [channel_observation, parameters, weights, noise_covariance, dimensions] = parse_input_parameters(varargin, ...
        ["channel_observation", "parameters", "weights", "noise_covariance", "dimensions"]);
    
    x = channel_observation;
    s = specular_model(parameters, dimensions)*weights;
    
    % compute residual channel obsevation
    x_r = x - s;

    M1 = dimensions(1);
    M2 = dimensions(2);
    M3 = dimensions(3);

    % estimate dimension 1 value
    X_1 = reshape(x_r, [M1 M2*M3]);

    mu_1 = 0:.05:2*pi;
    C = arrayfun(@(mu) norm(manifold_matrix(mu, M1)'*X_1), mu_1);
    [~, idx] = max(C);
    mu_1_hat = mu_1(idx);

    % estimate dimension 2 value
    Q1 = manifold_matrix(mu_1_hat, M1);
    x_r2 = Q1'*X_1;
    x_r2 = x_r2(:);

    X_2 = reshape(x_r2, [M2 M3]);
    mu_2 = -pi:.05:pi;
    C = arrayfun(@(mu) norm(manifold_matrix(mu, M2)'*X_2), mu_2);
    [~, idx] = max(C);
    mu_2_hat = mu_2(idx);

    % estimate dimension 3 value
    Q2 = manifold_matrix(mu_2_hat, M2);
    x_r3 = Q2'*X_2;
    x_r3 = x_r3(:);

    X_3 = reshape(x_r3, [M3 1]);
    mu_3 = -pi:.05:pi;
    C = arrayfun(@(mu) norm(manifold_matrix(mu, M3)'*X_3), mu_3);
    [~, idx] = max(C);
    mu_3_hat = mu_3(idx);

    new_path_parameters = [mu_1_hat; mu_2_hat; mu_3_hat];
    new_path_weight = weights_estimation(x_r, new_path_parameters, noise_covariance, dimensions);
end

function run_unitary_test()
    [channel_observation, parameters, weights, noise_covariance, dimensions]  = generate_synthetic_data_for_unitary_test();

    % add one more path
    new_path_parameters = physical_to_normalized_mapping([50e-9; 25; 50]);
    new_path_weight = 0.15;
    x_new = specular_model(new_path_parameters, dimensions)*new_path_weight;

    [estimated_parameters, estimated_weights] = new_path_estimation(channel_observation + x_new, ...
        parameters, weights, noise_covariance, dimensions);
end