function [H, Hn] = complete_model_hessian(varargin)
    % INPUT: ["parameters", "weights", "noise_covariance", "dimensions"].
    % OUTPUT: [H, Hn], the hessian of the specular model with respect to the input
    % parameters and the normalized Hessian
    % Reference: Eq. 4.16 from Richter
    if isempty(varargin)
        run_unitary_test();
        return;
    end

    [parameters, weights, noise_covariance, dimensions] = parse_input_parameters(varargin, ...
        ["parameters", "weights", "noise_covariance", "dimensions"]);

    D = specular_model_jacobian(parameters, weights, dimensions);

    Rnn = noise_covariance;

    H = 2*real(D'*inv(Rnn)*D);
    Hn = diag(diag(H))^(-1/2)*H*diag(diag(H))^(-1/2);
    Hn (abs(Hn) < 1e-5) = 0;
end

function run_unitary_test()
    delays = [10e-9; 30e-9];
    doas = (pi/180)*[45;60;70;100];
    physical_parameters = [delays; doas];
    mu = parameter_mapping(physical_parameters, "physical");
    
    weights = [1; 1i];

    M1 = 16;
    M2 = 4;
    M3 = 4;
    dimensions = [M1; M2; M3];

    noise_power = 1e-9;
    noise_covariance = noise_power*eye(M1*M2*M3);
    
    [H, Hn] = complete_model_hessian(mu, weights, noise_covariance, dimensions);
end