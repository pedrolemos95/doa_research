function dmc_estimate = raw_dmc_estimate(varargin)
    if (isempty(varargin))
        run_unitary_test();
        return;
    end

    X = parse_input_parameters(varargin, "channel_observation");

    % Reference: Section 6.1.8 from Richter
    Mf = numel(X(:,1));
    N = numel(X(1,:));
    R = X*X'/N;

    F = (1/sqrt(Mf))*dftmtx(Mf);

    y = diag(F'*R*F);

    alpha_0 = abs(min(y));

    alpha_1 = abs(max(y)-alpha_0);

    r1 = abs(mean(diag(R)));

    beta = alpha_1/(Mf*(r1-alpha_0));

    tau = 0.025; % hard coded for the moment

    % dmc_estimate = [alpha_0, alpha_1, beta, tau];
    dmc_estimate = [alpha_1, beta, 0];

end

function run_unitary_test()
    Bd = 6e6; % DMC coherence bandwith
    f0 = 1e6; % frequency domain sampling distance
    Mf = 60; % number of frequency domain samples
    Bm = Mf*f0; % Measurement bandwith
    beta_d = Bd/Bm;
    % Td = 250e-9; % Base TDoA of DMC
    Td = 0; % Base TDoA of DMC
    tm = 10e-6; % length of the observed impulse response
    tau_d = Td/tm;
    alpha_1 = 1e-4;
    dmc_parameters = [alpha_1, beta_d, tau_d];

    N = 100; % num_independent_samples

    [Rf,X] = dmc_model(dmc_parameters, Mf, N);

    dmc_estimate = raw_dmc_estimate(X);
end