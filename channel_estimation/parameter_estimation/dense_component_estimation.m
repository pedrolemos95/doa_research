function dmc_parameters = dense_component_estimation(varargin)
    % INPUT: ["channel_observation", "initial_dmc_estimate"]. OUTPUT. "dmc_parameters"
    % Reference: Chapter 2 and 6 of Richter
    if isempty(varargin)
        run_unitary_test();
        return;
    end

    [channel_observation, initial_estimate] = parse_input_parameters(varargin, ...
        ["channel_observation", "initial_dmc_estimate"]);
    
    x0 = initial_estimate;

    X = channel_observation;

    Mf = numel(X(:,1));
    N = numel(X(1,:));

    options = optimset('TolFun', 1e-15, 'TolX', 1e-15);
    fun = @(x) 1/cost_function(x, X, Mf, N);

    x0 = x0(1:2);

    dmc_parameters = fminsearch(fun, x0);
    
end

function value = cost_function(dmc_parameters, X, Mf, N)

    dmc_param = [dmc_parameters, 0];

    Rf = dmc_model(dmc_param, Mf, N);
    % value = (1/((det(pi*Rf)^N)))*exp(-abs(trace(X'*inv(Rf)*X)));

    value = real(-N*log(det(pi*Rf)) -trace(X'*inv(Rf)*X));

    % value = exp(-abs(trace(X'*inv(Rf)*X)));
    % value = abs(-N*log(det(Rf)) - abs(trace(X'*inv(Rf)*X)));
end

function initial_dmc_estimate = estimate_initial_value(X)
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

    initial_dmc_estimate = [alpha_0, alpha_1, beta, tau];
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

    N = 10; % num_independent_samples

    [Rf,X] = dmc_model(dmc_parameters, Mf, N);

    initial_dmc_estimate = estimate_initial_value(X);

    % initial_dmc_estimate = [1e-5; 0.025; tau_d];

    estimated_dmc_parameters = dense_component_estimation(X, initial_dmc_estimate(2:end));
end