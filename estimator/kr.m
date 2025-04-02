function C = kr(varargin)
    % INPUT: ["A", "B"]. OUTUPT: C, the kathri-rao product of A and B
    % Description: The kathri-rao product
    if isempty(varargin)
        run_unitary_test();
        return;
    end

    p = inputParser;
    addRequired(p, "A");
    addRequired(p, "B");
    parse(p, varargin{:});

    A = p.Results.A;
    B = p.Results.B;

    % [A,B] = parse_input_parameters(varargin, ["A", "B"]);

    C = arrayfun(@(col) kron(A(:,col), B(:,col)), 1:numel(A(1,:)), 'UniformOutput', false);
    C = [C{:}];

end

function run_unitary_test()
    A = [1 3; 2 4];
    B = [4 7; 5 8; 6 9];
    C = kr(A,B);
end