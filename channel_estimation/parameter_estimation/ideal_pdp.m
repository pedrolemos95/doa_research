function [pdp_estimate, pdp_calculated] = ideal_pdp(varargin)
    if (isempty(varargin))
        run_unitary_test();
        return;
    end

    [channel_observations, dimensions ] = parse_input_parameters(varargin, ["channel_observation", "aperture_dimensions"]);

    M1 = dimensions(1);
    M2 = dimensions(2);
    M3 = dimensions(3);

    X = reshape(channel_observations, [M1 M2*M3]);

    F = (1/sqrt(M1))*dftmtx(M1);
    
    Xt = F'*X;
    
    N = numel(Xt(1,:));
    pdp_estimate = sum(Xt.*conj(Xt),2)/N;
    
end

function pdp_calculated = calculate_pdp(smc_parameters, dmc_parameters, time_stamps)

    weights = smc_parameters(:,1);
    delays = (1/(2*pi))*smc_parameters(:,2);

    alpha = dmc_parameters(1);
    beta = dmc_parameters(2);
    pdp_calculated = alpha*exp(-beta*time_stamps);
    
    N = numel(time_stamps);
    smc = zeros(1,N);
    smc(ceil(N*delays)) = weights.^2;

    pdp_calculated = pdp_calculated + smc;
end

function run_unitary_test()

    % Generate DMC realization
    M1 = 100;
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

    % estimate PDP
    pdp_estimate = ideal_pdp(dmc(:), dimensions);

    % calculate PDP

    weights = sqrt([0.5e-4; 1e-5; 1e-6]);
    delays = 2*pi*f0*[10e-9; 50e-9; 100e-9];
    smc_parameters = [weights, delays];
    time_stamps = linspace(0,1,100)*M1;
    pdp_calculated = calculate_pdp(smc_parameters, dmc_parameters, time_stamps);

    plot(pdp_estimate);
    hold on;
    plot(pdp_calculated);
end