function pdp_estimate = raw_pdp_estimate(varargin)
    if (isempty(varargin))
        run_unitary_test();
        return;
    end

    [channel_observations, dimensions ]= parse_input_parameters(varargin, ["channel_observation", "aperture_dimensions"]);

    M1 = dimensions(1);
    M2 = dimensions(2);
    M3 = dimensions(3);

    X = reshape(channel_observations, [M1 M2*M3]);

    F = (1/sqrt(M1))*dftmtx(M1);
    
    Xt = F'*X;
    
    N = numel(Xt(1,:));
    pdp_estimate = sum(Xt.*conj(Xt),2)/N;
    
end

function run_unitary_test()

    % Generate DMC realization
    M1 = 60;
    M2 = 16;
    M3 = 16;
    dimensions = [M1;M2;M3];
    f0 = 1e6;

    dmc_power = 1e-5;

    channel_coherence_bandwidth = 10e6;
    measurement_bandwidth = M1*f0;
    dmc_beta = channel_coherence_bandwidth/measurement_bandwidth;
    dmc_tau = 0;

    dmc_parameters = [dmc_power, dmc_beta, dmc_tau];
    
    [~, dmc] = dmc_model(dmc_parameters, M1, M2*M3);

    pdp_estimate = raw_pdp_estimate(dmc(:), dimensions);

    plot(pdp_estimate);
end