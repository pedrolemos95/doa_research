function D = insightful_specular_model_jacobian(varargin)
    % INPUT: ["parameters", "weights", "dimensions"]. OUTPUT: D, the
    % Jacobian
    % Reference: Eq. 4.55 to 4.58 of Richter
    if isempty(varargin)
        run_unitary_test();
        return;
    end

    [parameters, weights, dimensions] = parse_input_parameters(varargin, ["parameters", "weights", "dimensions"]);

    pm = reshape(parameters, [numel(parameters)/3 3]);


    [B_1, D_1] = manifold_matrix(pm(:,1), dimensions(1));
    [B_2, D_2] = manifold_matrix(pm(:,2), dimensions(2));
    [B_3, D_3] = manifold_matrix(pm(:,3), dimensions(3));
    B = kr(B_3, kr(B_2, B_1));

    I = ones(numel(weights), 1);
    D0 = [weights.' weights.' weights.' I.' 1i*I.'];
    D1 = [D_1 B_1 B_1 B_1 B_1];
    D2 = [B_2 D_2 B_2 B_2 B_2];
    D3 = [B_3 B_3 D_3 B_3 B_3];

    D = kr(D3, kr(D2, kr(D1, D0)));
end

function run_unitary_test()
    delays = [10e-9; 100e-9];
    doas = (pi/180)*[45;60;10;100];
    weights = [1; 1i];

    physical_parameters = [delays; doas];
    mu = parameter_mapping(physical_parameters, "physical");

    dimensions = [4;4;4];

    D = insightful_specular_model_jacobian(mu, weights, dimensions);
end