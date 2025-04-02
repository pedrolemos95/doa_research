function output_parameters = parameter_mapping(varargin)
    % INPUT: ["parameters", "input_type"] . OUTPUT: normalized_parameters
    % Reference: Table 3-1 from Richter
    if isempty(varargin)
        run_unitary_test()
        return;
    end

    [parameters, input_type] = parse_input_parameters(varargin, ["parameters", "input_type"]);

    if (input_type == "physical")
        output_parameters = physical_to_normalized(parameters);
    else
        output_parameters = normalized_to_physical(parameters);
    end
    
end

function normalized_parameters = physical_to_normalized(physical_parameters)
    
    P = numel(physical_parameters)/3;
    rp = load_receiver_parameters;

    tau = physical_parameters(1:P);
    mu_1 = 2*pi*rp.f0*tau;

    els = physical_parameters(P+1:2*P);
    azs = physical_parameters(2*P+1:3*P);
    mu_2 = 2*pi*(rp.d/rp.lam)*cos(els).*cos(azs);
    mu_3 = 2*pi*(rp.d/rp.lam)*cos(els).*sin(azs);

    normalized_parameters = [mu_1; mu_2; mu_3];
end

function physical_parameters = normalized_to_physical(normalized_parameters)
    P = numel(normalized_parameters)/3;
    
    rp = load_receiver_parameters;

    mu_1 = normalized_parameters(1:P);
    delays = mu_1/(2*pi*rp.f0);

    mu_2 = normalized_parameters(P+1:2*P);
    mu_3 = normalized_parameters(2*P+1:3*P);

    azs = atan2(mu_3,mu_2);
    els = asin(sqrt(mu_2.^2 + mu_3.^2)./(2*pi*(rp.d/rp.lam)));

    physical_parameters = [delays; els; azs];
end

function run_unitary_test()
    delays = [10e-9; 100e-9]; % delays in [s]
    doas = (pi/180)*[45;60;10;100]; % delays in radians

    physical_parameters = [delays; doas];

    normalized_parameters = parameter_mapping(physical_parameters, "physical");
    physical = parameter_mapping(normalized_parameters, "normalized");
end