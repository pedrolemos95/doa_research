function strong_rays = remove_weak_rays(varargin)
    % INPUT: ["rays", "noise_power"]. OUTPUT: specular_rays
    if isempty(varargin)
        run_unitary_test();
        return;
    end

    [rays, relative_power_threshold] = parse_input_parameters(varargin, ["rays", "relative_power_threshold"]);

    num_obs = numel(rays);

    strong_rays = cell([num_obs 1]);
    for obs_idx = 1:num_obs
        strong_rays_for_this_obs = [];
        max_power_for_this_obs = max(10.^(-[rays{obs_idx}{1}{1}.PathLoss]/10));
        for ray_idx = 1:numel(rays{obs_idx}{1}{1})
            ray = rays{obs_idx}{1}{1}(ray_idx);
            ray_power = 10^(-ray.PathLoss/10);

            ray_relative_power = 10*log10(ray_power/max_power_for_this_obs);
            % ray_snr = 10^(-ray.PathLoss/10)/noise_power;
            if (ray_relative_power > relative_power_threshold) % relative power < 30 dB
                strong_rays_for_this_obs = [strong_rays_for_this_obs , ray];
            end
        end
        strong_rays{obs_idx} = {{strong_rays_for_this_obs}};
    end

end

function run_unitary_test()
    % set trajectory
    tx_pos = [5;2;0.5];
    rx_pos = [5;7;4.0];
    
    % simulate propagation rays
    num_observations = numel(tx_pos)/3;
    map_file = "simulation_map.stl";
    rays = simulate_propagation(map_file, tx_pos, rx_pos, 1, num_observations);

    relative_power_threshold = -30;
    strong_rays = remove_weak_rays(rays, relative_power_threshold);
end