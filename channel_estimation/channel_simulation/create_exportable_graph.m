function graph_handler = create_exportable_graph(graph_idx, x_axes, y_axes, titles)
    figure(graph_idx);
    plot(x_axes, y_axes, '-o', 'LineWidth', 2);
    title(titles(1));
    xlabel(titles(2));
    ylabel(titles(3));
    graph_handler = gca;
end