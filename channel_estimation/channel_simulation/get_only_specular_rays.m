function specular_rays = get_only_specular_rays(varargin)
    % INPUT: ["rays", "noise_power"]. OUTPUT: specular_rays
    if isempty(varargin)
        run_unitary_test();
        return;
    end

    [rays, noise_power] = parse_input_parameters(varargin, ["rays", "noise_power"]);

    num_obs = numel(rays);

    specular_rays = cell([num_obs 1]);
    for obs_idx = 1:num_obs
        specular_rays_for_this_obs = [];
        for ray_idx = 1:numel(rays{obs_idx}{1}{1})
            ray = rays{obs_idx}{1}{1}(ray_idx);
            ray_snr = 10^(-ray.PathLoss/10)/noise_power;
            if (ray_snr > 10.6724) % SNR > 10.6724 from Richter
                specular_rays_for_this_obs = [specular_rays_for_this_obs, ray];
            end
        end
        specular_rays{obs_idx} = {{specular_rays_for_this_obs}};
    end

end

function run_unitary_test()
    M_1 = 16; % frequency related
    M_2 = 4; % spatial related
    M_3 = 4; % spatial related
    
    % set trajectory
    tx_pos = repmat([5;2;0.5], [1 10]);
    rx_pos = [5;7;4.0];
    
    % simulate propagation rays
    num_observations = numel(tx_pos)/3;
    map_file = "simulation_map.stl";
    rays = simulate_propagation(map_file, tx_pos, rx_pos, 1, num_observations);

    rp = load_receiver_parameters;

    specular_rays = get_only_specular_rays(rays, rp.noise_power);
end