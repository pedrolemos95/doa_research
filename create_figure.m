function create_figure(f)

    figure;
    hold off;
    hold on;
    graphs = f("graphs");
    handlers = cell([numel(graphs) 1]);
    for graph_idx = 1:numel(graphs)
        graph = graphs{graph_idx};
        y_data = graph.y_data;
        if (isfield(graph, 'x_data'))
            x_data = graph.x_data;
            handlers{graph_idx} = plot(x_data, y_data, "LineWidth", 2);
        else
            handlers{graph_idx} = plot(y_data, "LineWidth", 2);
        end
    end

    if f.isKey("markers")
        markers = f("markers");
        for marker_idx = 1:numel(f("markers"))
            h = handlers{marker_idx}; 
            h.Marker = markers{marker_idx};
        end
    end

    if f.isKey("markersfacecolors")
        markersfacecolors = f("markersfacecolors");
        for mfc_idx = 1:numel(f("markersfacecolors"))
            h = handlers{mfc_idx}; 
            h.MarkerFaceColor = markersfacecolors{mfc_idx};
        end
    end

    if f.isKey("linestyles")
        linestyles = f("linestyles");
        for linestyle_idx = 1:numel(f("linestyles"))
            h = handlers{linestyle_idx}; 
            h.LineStyle = linestyles{linestyle_idx};
        end
    end

    if f.isKey("linewidths")
        linewidths = f("linewidths");
        for linewidth_idx = 1:numel(f("linewidths"))
            h = handlers{linewidth_idx}; 
            h.LineWidth = linewidths{linewidth_idx};
        end
    end

    if f.isKey("xlabel")
        xlabel(f("xlabel"), 'FontSize', 12, 'Interpreter', 'latex');
    end
    
    if f.isKey("ylabel")
        ylabel(f("ylabel"), 'FontSize', 12, 'Interpreter', 'latex');
    end
    
    if f.isKey("legends")
        lgd = legend(f("legends"), 'Interpreter', 'latex', 'FontSize', 10, 'Location', 'northeast');
        set(lgd, 'Box', 'on');
        set(lgd, 'EdgeColor', [0.6 0.6 0.6]);
    end

    if f.isKey("tabletext")
        text(2.2, 4, f("tabletext"), 'FontSize', 12, 'BackgroundColor', 'w', 'EdgeColor', 'k', 'Margin', 5);
    end

    % Grid and axis styles
    ax = gca;
    grid on;
    ax.GridColor = [0.7 0.85 1];       % Light blue color (normalized RGB)
    ax.GridAlpha = 1;                  % Fully opaque grid
    ax.GridLineStyle = '-';           % Solid lines
    ax.Layer = 'bottom';              % Grid behind the data
    
    set(gca, 'GridLineStyle', '-'); set(gca, 'GridAlpha', 0.3); set(gca, 'LineWidth', 1);
    ax = gca; ax.FontSize = 10; ax.LineWidth = 1.2; ax.Box = 'on';
    set(gcf, 'Color', 'w'); axis tight;
   
    if f.isKey("ylim")
        ylim(f("ylim"));
    end

    print("figs/" + f("figure_name") + ".eps",'-depsc','-painters','-r300');
    savefig("figs/" + f("figure_name") + ".fig");
end