addpath estimator/

%% Clean variables and close windows
clear; clc; close all;

%% Choose the parameters
M_1 = 10; % frequencies
M_2 = 4; % antenna hor
M_3 = 4; % antenna ver
dim_input = [M_1; M_2; M_3];
num_observations = 100;
K_high = 4;
K_low = 1;

%% Estimate DoA error from multiple K values
% ks_values = [1;4];
ks = 0.25*ones(1,20);
% ks_values = [ks, 0.25:0.25:4];
ks_values = 0.01:0.02:4;

doa_errors = cell(1,length(ks_values));
est_ks = cell(1,length(ks_values));

for k = 1:length(ks_values)
    results = doa_classification_generic(ks_values(k), ks_values(k), [16; 4; 4], num_observations);
    doa_errors{k} = results("unclass_dir_error");
    est_ks{k} = results("est_ks");
end

%% Estimate the results
% K_thres = 0.25:0.25:4;
K_thres = ks_values;
total_doa_errors = [doa_errors{:}]; total_doa_errors = total_doa_errors(:);
total_est_ks = [est_ks{:}]; total_est_ks = total_est_ks(:);

mean_errors = zeros(1, length(K_thres));
percentage_dropped = zeros(1, length(K_thres));
cdfs = cell(1,length(K_thres));
sdata = cell(1,length(K_thres));

for k = 1:length(K_thres)

    class_doa_errors = total_doa_errors(total_est_ks > K_thres(k));
    mean_errors(k) = mean(class_doa_errors);
    percentage_dropped(k) = 1 - length(class_doa_errors)/length(total_doa_errors);
    [cdfs{k}, sdata{k}] = ecdf(class_doa_errors);
end

%% Plot results
% Plot the CDFs
% hold on;
% for k = 1:length(K_thres)
%     plot(sdata{k}, cdfs{k}, 'LineWidth', 2);
% end
% 
% xlabel('DoA error (º)', 'FontSize', 12);
% ylabel('CDF', 'FontSize', 12);
% 
% % Legend
% legend_cell = cell(1,length(K_thres));
% for k = 1:length(K_thres)
%     legend_cell{k} = "$$K = " + K_thres(k) + "$$";
% end
% legend(legend_cell, ...
%        'Interpreter', 'latex', 'FontSize', 10, 'Location', 'northeast');
% 
% % Grid and axis styles
% grid on; ax = gca; ax.FontSize = 10; ax.LineWidth = 1.2; ax.Box = 'on';
% set(gcf, 'Color', 'w'); axis tight;
% print('figs/cdfs_vs_threshold','-depsc','-painters','-r300');

% Plot the mean errors for different threshold
hold off;
% figure;
plot(K_thres, mean_errors, 'LineWidth', 2);
xlabel('K threshold', 'FontSize', 12);
ylabel('Mean AoA error (º)', 'FontSize', 12);

title("Mean AoA error vs K threshold");
xPos = 2.2;  % X position
yPos = 1.3; % Y position
text(xPos, yPos, sprintf('Azimuth = %d (º)\nElevation = %d (º)\n', ...
     60, 20), 'FontSize', 12, 'BackgroundColor', 'w', ...
     'EdgeColor', 'k', 'Margin', 5);

grid on; ax = gca; ax.FontSize = 10; ax.LineWidth = 1.2; ax.Box = 'on';
set(gcf, 'Color', 'w'); axis tight;
print('figs/mean_error_vs_threshold','-depsc','-painters','-r300');