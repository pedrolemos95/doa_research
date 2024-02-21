function [rays, tx_out, rx_out ] = simulate_propagation(varargin)
    % INPUT: ["map_file", "tx_pos", "rx_pos", "num_freq", "num_obs"]. OUTPUT: rays
    if isempty(varargin)
        run_unitary_test();
    end

    [map_file, tx_pos, rx_pos, num_freq, num_obs] = parse_input_parameters(varargin, ...
        ["map_file", "tx_pos", "rx_pos", "num_freq", "num_obs"]);

    map = load_map(map_file);

    tx = txsite("cartesian", "AntennaPosition", tx_pos(:,1), "TransmitterFrequency",2.402e9); tx_out = tx;
    rx = rxsite("cartesian", "AntennaPosition", rx_pos); rx_out = rx;
    pm = propagationModel("raytracing", ...
        "CoordinateSystem","cartesian", ....
        "MaxNumReflections",7, ...
        "MaxNumDiffractions",0, ...
        "SurfaceMaterial","metal"); 

    rp = load_receiver_parameters;

    rays = cell([num_obs 1]);

    for k=1:num_obs
        rays_for_same_obs = cell([num_freq 1]);
        tx.AntennaPosition = tx_pos(:,k);
        for n=1:num_freq
            tx.TransmitterFrequency = tx.TransmitterFrequency + (n-1)*rp.f0;
            rays_for_same_obs{n} = raytrace(tx, rx, pm,"Map", map);
        end
        rays{k} = rays_for_same_obs;
    end

end

function run_unitary_test()
    tx_pos = repmat([3;3;0.5], [1 2]);
    rx_pos = [3;3;5.0];
    map_file = "map_for_unitary_test.stl";
    num_frequencies = 4;
    num_obs = numel(tx_pos)/3;

    rays = simulate_propagation(map_file, tx_pos, rx_pos, num_frequencies, num_obs);
    
    siteviewer("SceneModel", load_map(map_file));

    plot(rays{1}{1});

end