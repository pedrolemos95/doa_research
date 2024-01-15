function [B_i, D_i] = manifold_matrix(varargin)
    % INPUT: ["parameters", "dimension_samples"]  OUTPUT = [B_i,D_i], the manifold
    % matrix of the i-th dimension and the derivative of B_i with respect
    % to parameter mu_i
    if isempty(varargin)
        run_unitary_test();
        return;
    end

    p = inputParser;
    addRequired(p, "parameters");
    addRequired(p, "dimension_samples");
    parse(p, varargin{:});

    parameters = p.Results.parameters;
    dimension_samples = p.Results.dimension_samples;


    % [parameters, dimension_samples] = parse_input_parameters(varargin, ["parameters", "dimension_samples"]);

    a_mu = @(mu, M) exp(-1i*(-(M-1)/2:(M-1)/2)*mu).';
    B_i = arrayfun(@(parameter) a_mu(parameter, dimension_samples), parameters, 'UniformOutput', false);
    
    B_i = [B_i{:}];

    M = dimension_samples;
    D_i = -1i*diag((-(M-1)/2:(M-1)/2))*B_i;
end

function run_unitary_test()
    mu_1 = [0.5; 1];
    dimension_samples = 4;

    [B_1, D_1] = manifold_matrix(mu_1, dimension_samples);
end