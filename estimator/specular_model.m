function B = specular_model(varargin)
    % INPUT: ["parameters", "dimensions"]. OUTPUT: manifold matrix
    if isempty(varargin)
        run_unitary_test();
        return;
    end

    p = inputParser;
    addRequired(p, "parameters");
    addRequired(p, "dimensions");
    parse(p, varargin{:});

    parameters = p.Results.parameters;
    dimensions = p.Results.dimensions;

    % [parameters, dimensions] = parse_input_parameters(varargin, ["parameters", "dimensions"]);

    pm = reshape(parameters, [numel(parameters)/3 3]); % reshape parameters vector as a matrix
    
    B_1 = manifold_matrix(pm(:,1), dimensions(1)); % frequency aperture
    B_2 = manifold_matrix(pm(:,2), dimensions(2)); % spatial aperture
    B_3 = manifold_matrix(pm(:,3), dimensions(3)); % spatial aperture
       
    B = kr(B_3, kr(B_2, B_1));
end

function run_unitary_test()

    % physical parameters
    delays = [10e-9; 100e-9];
    doas = (pi/180)*[45;60;10;100];
    physical_parameters = [delays; doas];

    mu = parameter_mapping(physical_parameters, "physical");

    B = specular_model(mu, [1;4;4]);

end