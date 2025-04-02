function varargout = parse_input_parameters(varargin_cell, parameters_list)
    p = inputParser;
    arrayfun(@(parameter) addRequired(p, parameter), parameters_list);
    parse(p, varargin_cell{:});

    for n = 1:nargout
        varargout{n} = getfield(p.Results, parameters_list(n)); 
    end
end