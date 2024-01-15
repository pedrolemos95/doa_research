function map = load_map(varargin)
    % INPUT: Map stl file. OUTPUT: stl object with reference on [0;0;0]
    if isempty(varargin)
        run_unitary_test();
        return;
    end

    map_file = parse_input_parameters(varargin, "map_file");
    
    map = stlread(map_file);

    map = triangulation(map.ConnectivityList, map.Points - min(map.Points));
end

function run_unitary_test()
    
    map_file = "map_for_unitary_test.stl";

    map = load_map(map_file);

    siteviewer("SceneModel", map);
end