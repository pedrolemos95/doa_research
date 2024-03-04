function R = full_covariance_matrix(varargin)
    if (isempty(varargin))
        run_unitary_test();
        return;
    end

    
    [dmc_parameters, noise_power, dimensions] = parse_input_parameters(varargin, ["dmc_parameters", "noise_power", "dimensions"]);

    M1 = dimensions(1);
    M2 = dimensions(2);
    M3 = dimensions(3);

    R1 = dmc_model(dmc_parameters, M1, 1);
    R2 = noise_power*eye(M2);
    R3 = noise_power*eye(M3);

    R = kron(R3, kron(R2, R1));
end


function run_unitary_test()

    noise_power = 1;

    M1 = 10;
    M2 = 4;
    M3 = 4;

    dmc_power = 1e-5;
    beta = 0.1;
    tau = 0;
    dmc_parameters = [dmc_power; beta; tau];

    dimensions = [M1;M2;M3];

    R = full_covariance_matrix(dmc_parameters, noise_power, dimensions);

end