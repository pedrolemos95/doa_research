function plot_angles(varargin)
    % INPUT: ["angles", "weights", "rx_position", "color"]
    if isempty(varargin)
        run_unitary_test();
        return;
    end
    
    [angles, weights, rx_pos, color] = parse_input_parameters(varargin, ["angles", "weights", "rx_position", "color"]);

    norm_weights = abs(weights)/max(abs(weights)); % normalize weights

    for angle_idx = 1:numel(angles(:,1))
        plot_ray(angles(angle_idx,:), norm_weights(angle_idx), rx_pos, color);
    end

end

function plot_ray(doa, weight, rx_pos, color)
    ray = comm.Ray;
    ray.ReceiverLocation = rx_pos;
    ray.TransmitterLocation = rx_pos - weight*[cos(doa(1))*cos(doa(2));cos(doa(1))*sin(doa(2));sin(doa(1))];
    if (color == "blue")
        ray.PathLossSource = "Custom";
        ray.PathLoss = 90;
    end

    plot(ray, "ColorLimits", [35 90]);
end

function run_unitary_test()

    rx_pos = [5;5;4];
    doa = (pi/180)*[45 60; 30 80]; % [elevation, azimuth]
    weight = [10;5];

    siteviewer("SceneModel", load_map("map_for_unitary_test.stl"));

    plot_angles(doa, weight, rx_pos, "blue");
    
end