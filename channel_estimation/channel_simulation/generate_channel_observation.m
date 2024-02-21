function [channel_observation, parameters, weights] = generate_channel_observation(varargin)
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
    channel_observation = specular_model(parameters, dimensions)*weights;
    noise = wgn(M1*M2*M3, 1, rp.noise_power, 'linear', 'complex');
    channel_observation = channel_observation + noise;

    % another way of generating the channel observation
    % num_freq = numel(rays);
    % X = zeros(M2*M3, num_freq);
    % for freq_idx = 1:num_freq
    %     [parameters, weights] = get_rays_parameters(rays{1,freq_idx}{1});
    %     X(:, freq_idx) = specular_model(parameters, [1;M2;M3])*weights;
    % end
    % channel_obs_2 = X.';
    % channel_obs_2 = channel_obs_2(:);
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
    Mf_1 = 40;
    Ms_2 = 16;
    Ms_3 = 16;

    tx_pos = repmat([3;3;0.5], [1 2]);
    rx_pos = [3;3;4.0];
    map_file = "map_for_unitary_test.stl";
    
    num_obs = numel(tx_pos)/3;

    rays = simulate_propagation(map_file, tx_pos, rx_pos, Mf_1, num_obs);

    X = generate_channel_observation(rays{1}, [Mf_1, Ms_2, Ms_3]);

    Xrz = reshape(X, [Mf_1 Ms_2*Ms_3]);

    F = (1/sqrt(Mf_1))*dftmtx(Mf_1);

    Xt = F'*Xrz;

    N = numel(Xt(1,:));
    ht = sum(Xt.*conj(Xt),2)/N;

    plot(ht);

end