function [Rf, dmc_realization] = dmc_model(varargin)
    % INPUT: [dmc_parameters", "num_freq_samples", "num_independent_samples"].
    % OUTPUT: ["dmc_covariance_matrix, dmc_realization"]
    % Reference: Section 2.5 of Richter
    if isempty(varargin)
        run_unitary_test();
        return;
    end

    [dmc_parameters, Mf, N] = parse_input_parameters(varargin, ...
        ["dmc_parameters", "num_freq_samples", "num_independent_samples"]);

    k = kappa(dmc_parameters, Mf);

    Rf = toeplitz(k, k');
    
    Rf = (Rf + Rf')/2;

    L = chol(Rf);

    Z = wgn(Mf, N, 1, 'linear', 'complex');

    dmc_realization = L*Z;

end

function k = kappa(dmc_parameters, num_samples)
    
    alpha_1 = dmc_parameters(1);
    beta_d = dmc_parameters(2);
    tau_d = dmc_parameters(3);
    
    Mf = num_samples;

    k = (alpha_1/Mf)*(exp(-1i*2*pi*(0:Mf-1)*tau_d)./(beta_d + 1i*2*pi*(0:Mf-1)/Mf)).';
end

function run_unitary_test()
    Bd = 6e6; % DMC coherence bandwith
    f0 = 2e6; % frequency domain sampling distance
    Mf = 40; % number of frequency domain samples
    Bm = Mf*f0; % Measurement bandwith
    beta_d = Bd/Bm;
    Td = 0; % Base TDoA of DMC
    tm = 10e-6; % length of the observed impulse response
    tau_d = Td/tm;
    alpha_1 = 1e-6;
    dmc_parameters = [alpha_1, beta_d, tau_d];

    N = 1024; % num_independent_samples

    [Rf, X] = dmc_model(dmc_parameters, Mf, N);
    % surf(10*log10(abs(Rf)));

    F = (1/sqrt(Mf))*dftmtx(Mf);

    Xt = F'*X;
    ht = sum(Xt.*conj(Xt),2)/N;

    plot(ht);
end