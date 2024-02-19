function [channel_observation, parameters, weights] = generate_channel_observation_with_dmc(varargin)
    % INPUT: ["rays", "dimensions"]. OUTPUT: channel observation
    if isempty(varargin)
        run_unitary_test();
        return;
    end

    [rays, dimensions] = parse_input_parameters(varargin, ["rays", "dimensions"]);

    % specular model
    M1 = dimensions(1);
    M2 = dimensions(2);
    M3 = dimensions(3);

    rp = load_receiver_parameters;
    [parameters, weights] = get_rays_parameters(rays{1}{1});
    smc = specular_model(parameters, dimensions)*weights;
    noise = wgn(M1*M2*M3, 1, rp.noise_power, 'linear', 'complex');

    dmc_power = 1e-1*sqrt(max(10.^(-([rays{1}{1}.PathLoss])/10))); % 10 db less than the maximum power
    channel_coherence_bandwith = 10e6; % in Hz
    measurement_bandwidth = 100e6; % in Hz
    Bd = channel_coherence_bandwith/measurement_bandwidth;
    tau_d = 0;
    
    dmc_parameters = [dmc_power, Bd, tau_d];
    [ ~ , dmc] = dmc_model(dmc_parameters, M1, M2*M3);
    dmc = dmc(:);
    channel_observation = smc + dmc + noise;

end

function [parameters, weights] = get_rays_parameters(rays)

    num_rays = numel(rays);
    parameters = zeros(num_rays, 3);
    weights = zeros(num_rays,1);
    for ray_idx = 1:num_rays
        [parameters(ray_idx, :), weights(ray_idx)] = get_ray_parameters(rays(ray_idx));
    end
    parameters = parameters(:);
end

function [parameters, weight] = get_ray_parameters(ray)
    delay = ray.PropagationDelay;
    aoa = (pi/180)*ray.AngleOfArrival;

    parameters = parameter_mapping([delay;flip(aoa)], "physical").';
    weight = sqrt(10^(-(ray.PathLoss)/10))*exp(-1i*ray.PhaseShift);
end

function run_unitary_test()
    Mf_1 = 100;
    Ms_2 = 16;
    Ms_3 = 16;

    tx_pos = repmat([3;3;0.5], [1 2]);
    rx_pos = [3;3;4.0];
    map_file = "map_for_unitary_test.stl";
    
    num_obs = numel(tx_pos)/3;

    rays = simulate_propagation(map_file, tx_pos, rx_pos, Mf_1, num_obs);

    X = generate_channel_observation_with_dmc(rays{1}, [Mf_1, Ms_2, Ms_3]);

    Xrz = reshape(X, [Mf_1 Ms_2*Ms_3]);

    F = (1/sqrt(Mf_1))*dftmtx(Mf_1);

    Xt = F'*Xrz;

    N = numel(Xt(1,:));
    ht = sum(Xt.*conj(Xt),2)/N;

    plot(ht);

end