function parameters = multidimensional_esprit(varargin)
    % INPUT: "channel_observation". OUTPUT: "parameters"
    if isempty(varargin)
        run_unitary_test();
        return;
    end

    channel_observation = parse_input_parameters(varargin, "channel_observations");

    % estimate covariance matrix
    X = channel_observation;
    Rxx = X*X';

    % perform eigenvalue decomposition
    [Q,D] = eig(Rxx);
    
    % get signal subspace
    Qs = Q(:,end);

    % apply selection matrix
end

function run_unitary_test()
    [channel_observation, parameters] = generate_synthetic_data_for_unitary_test;

    estimated_parameters = multidimensional_esprit(channel_observation);
end