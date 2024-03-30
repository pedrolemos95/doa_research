function [parameters, weight] = esprit(varargin)
    % INPUT: channel_observation, dimensions, num_of_paths. OUTPUT: parameters, path_weight
    % Reference: Sec 5.3 from Richter
    if isempty(varargin)
        run_unitary_test();
        return;
    end
    
    [X, dim, P] = parse_input_parameters(varargin, ["channel_observation", "dimensions", "num_of_paths"]);

    L = numel(X(1,:));
    Rxx = (X*X')/L;

    [Q,D] = eig(Rxx);
    [D,I]=sort(diag(D),1,'descend');
    Q = Q(:,I); % Sort the eigenvectors to put signal eigenvector first
    Qs = Q(:,1:P); % Extract the signal subspace

    M1 = dim(1);
    M2 = dim(2);
    M3 = dim(3);
    M = M1*M2*M3;

    % selection matrices...
    Js1_1 = kron(kron(eye(M3), eye(M2)), [eye(M1-1) zeros(M1-1,1)]);
    Js1_2 = kron(kron(eye(M3), [eye(M2-1) zeros(M2-1,1)]), eye(M1));
    Js1_3 = kron(kron([eye(M3-1) zeros(M3-1,1)], eye(M2)), eye(M1));

    Js2_1 = kron(kron(eye(M3), eye(M2)), [zeros(M1-1,1) eye(M1-1)]);
    Js2_2 = kron(kron(eye(M3), [zeros(M2-1,1) eye(M2-1)]), eye(M1));
    Js2_3 = kron(kron([zeros(M3-1,1) eye(M3-1)], eye(M2)), eye(M1));

    % Estimate mu_1, mu_2 and mu_3
    if (M1 > 1); Psi_1 = inv((Js1_1*Qs)'*Js1_1*Qs)*(Js1_1*Qs)'*Js2_1*Qs; else; Psi_1 = zeros(P); end
    if (M2 > 1); Psi_2 = inv((Js1_2*Qs)'*Js1_2*Qs)*(Js1_2*Qs)'*Js2_2*Qs; else; Psi_2 = zeros(P); end
    if (M3 > 1); Psi_3 = inv((Js1_3*Qs)'*Js1_3*Qs)*(Js1_3*Qs)'*Js2_3*Qs; else; Psi_3 = zeros(P); end

    [~,mu_1] = eig(Psi_1);
    mu_1 = diag(-angle(mu_1));
    [~,mu_2] = eig(Psi_2);
    mu_2 = diag(-angle(mu_2));
    [~,mu_3] = eig(Psi_3);
    mu_3 = diag(-angle(mu_3));

    parameters = [mu_1;mu_2;mu_3];
    % Best Linear Unbiased Estimator with white noise assumption. Richter Sec. 5.1
    B = specular_model(parameters, dim);
    weight = mean(inv(B'*B)*B'*X,2); % TODO: Check if this mean is ok
end

function run_unitary_test()
    delay = 1e-9;
    doa = [45; 30]*(pi/180);
    mu = parameter_mapping([delay;doa], "physical");
    path_weight = 1+0.5*1i;

    P = 1; % Paths to be estimated

    % aperture dimensions
    M1 = 20; % number of frequencies
    M2 = 4; % number of rows in antenna array
    M3 = 4; % number of cols in antenna array
    dim = [M1;M2;M3];

    X = specular_model(mu, dim)*path_weight + wgn(M1*M2*M3,1,-20,'dBW');

    [est_mu, est_weight] = esprit(X, dim, P);
end