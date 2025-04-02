addpath estimator/

%% Clean variables and close windows
clear; clc; close all;

%% Choose the parameters
M_1 = 10; % frequencies
M_2 = 4; % antenna hor
M_3 = 4; % antenna ver
dim_input = [M_1; M_2; M_3];
num_observations = 200;
K_high = 4;
K_low = 1;

freqs = [4,16,64];

est_ks_total = cell(1,length(freqs));

for f = 1:length(freqs)

K_range = 0.5:0.1:10;
mean_k = zeros(length(K_range),1);

for k = 1:length(K_range)
    results = doa_classification_generic(K_range(k), K_low, [16; sqrt(freqs(f)); sqrt(freqs(f))], num_observations);
    est_ks = results("est_ks");
    est_ks_high = est_ks(1:end/2);
    mean_k(k) = mean(est_ks_high); 
end

est_ks_total{f} = mean_k;
end


%% Plot resuilts
hold on;
for f = 1:length(freqs)

    plot(K_range, est_ks_total{f}, 'LineWidth', 2);

end

xlabel('Sample', 'FontSize', 12);
ylabel('K estimate', 'FontSize', 12);

% Legend
legend({'$$N = 4$$', '$$N = 16$$', '$$N = 64$$'}, ...
       'Interpreter', 'latex', 'FontSize', 10, 'Location', 'northeast');

% Grid and axis styles
grid on; ax = gca; ax.FontSize = 10; ax.LineWidth = 1.2; ax.Box = 'on';
set(gcf, 'Color', 'w'); axis tight;
print('figs/estimated_ks_var_ant','-depsc','-painters','-r300');

%% TMP

% results_f_1_1 = doa_classification_generic(K_high, K_low, [16; 2; 2], num_observations);
% results_f_1_2 = doa_classification_generic(K_high, K_low, [16; 4; 4], num_observations);
% results_f_1_3 = doa_classification_generic(K_high, K_low, [16; 8; 8], num_observations);

% results_f_2_1 = doa_classification_generic(K_high, K_low, [16; 2; 2], num_observations);
% results_f_2_2 = doa_classification_generic(K_high, K_low, [16; 4; 4], num_observations);
% results_f_2_3 = doa_classification_generic(K_high, K_low, [16; 8; 8], num_observations);

%% Make the first figure, pt.1 
% est_k_1_1 = results_f_1_1("est_ks");
% est_k_1_2 = results_f_1_2("est_ks");
% est_k_1_3 = results_f_1_3("est_ks");
% 
% 
% % K estimate figure
% hold off; hold on;
% 
% plot([K_high*ones([num_observations,1]); (K_low)*ones([num_observations,1])], 'LineWidth', 2);
% plot(est_k_1_1, 'LineWidth',2);
% plot(est_k_1_2, 'LineWidth',2);
% plot(est_k_1_3, 'LineWidth',2);
% 
% xlabel('Sample', 'FontSize', 12);
% ylabel('K estimate', 'FontSize', 12);
% 
% % Legend
% legend({'Real', '$$N = 4$$', '$$N = 16$$', '$$N = 64$$'}, ...
%        'Interpreter', 'latex', 'FontSize', 10, 'Location', 'northeast');
% 
% % Grid and axis styles
% grid on; ax = gca; ax.FontSize = 10; ax.LineWidth = 1.2; ax.Box = 'on';
% set(gcf, 'Color', 'w'); axis tight;
% ylim([0,10])
% print('figs/estimated_ks_many_as','-depsc','-painters','-r300');
%% Make the first figure, pt.2 