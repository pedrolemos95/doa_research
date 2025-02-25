clear
clc

%% Generate K estimate figure
% fig_2_data_1 = readmatrix('fig_2_data_1.csv');
% hold off; hold on;
% K = 4; plot([K*ones([100,1]); (K/4)*ones([100,1])], 'LineWidth', 2);
% plot(fig_2_data_1, 'LineWidth', 2);
% 
% xlabel('Sample', 'FontSize', 12);
% ylabel('K estimate', 'FontSize', 12);
% 
% % Grid and axis styles
% grid on;
% ax = gca;
% ax.FontSize = 10;
% ax.LineWidth = 1.2;
% ax.Box = 'on';
% 
% % Legend
% legend({'Real', 'Estimated'}, ...
%        'Interpreter', 'latex', 'FontSize', 10, 'Location', 'northeast');
% 
% set(gcf, 'Color', 'w');
% axis tight;
% ylim([0, 7]);
% print('figure_2','-depsc','-painters','-r300');

%% Generate dir error CDF
% fig_3_data_1 = readmatrix('fig_3_data_1.csv');
% fig_3_data_2 = readmatrix('fig_3_data_2.csv');
% hold off; hold on;
% h = cdfplot(fig_3_data_1); set(h, 'LineWidth', 2); title('');
% h = cdfplot(fig_3_data_2); set(h, 'LineWidth', 2); title('');
% 
% % Grid and axis styles
% grid on;
% ax = gca;
% ax.FontSize = 10;
% ax.LineWidth = 1.2;
% ax.Box = 'on';
% 
% ylabel('P(X < x)', 'FontSize', 12);
% xlabel('DoA error (º)', 'FontSize', 12);
% 
% % Legend
% legend({'With Classification', 'Without Classification'}, ...
%        'Interpreter', 'latex', 'FontSize', 10, 'Location', 'southeast');
% 
% set(gcf, 'Color', 'w');
% axis tight;
% print('figure_3','-depsc','-painters','-r300');

%% Generate fig 4 ## OK ##
% fig_4_data_1 = readmatrix('fig_4_data_1.csv');
% fig_4_data_2 = readmatrix('fig_4_data_2.csv');
% hold off; hold on;
% 
% % Retrieve the data
% K = 4; plot([K*ones([100,1]); (K/4)*ones([100,1])], 'LineWidth', 2);
% plot(fig_4_data_1, 'LineWidth', 2);
% plot(fig_4_data_2, 'LineWidth', 2);
% 
% % Labels
% xlabel('Sample', 'FontSize', 12);
% ylabel('K estimate', 'FontSize', 12);
% 
% % Grid and axis styles
% grid on;
% ax = gca;
% ax.FontSize = 10;
% ax.LineWidth = 1.2;
% ax.Box = 'on';
% 
% % Legend
% legend({'Real', '$M_f = 4$', '$M_f = 40$'}, ...
%        'Interpreter', 'latex', 'FontSize', 10, 'Location', 'northeast');
% 
% set(gcf, 'Color', 'w');
% axis tight;
% ylim([0, 10]);
% print('figure_4','-depsc','-painters','-r300');

%% Generate fig 5 ## OK ##
% fig_5_data_1 = readmatrix('fig_5_data_1.csv');
% fig_5_data_2 = readmatrix('fig_5_data_2.csv');
% hold off; hold on;
% h = cdfplot(fig_5_data_1); set(h, 'LineWidth', 2); title('');
% h = cdfplot(fig_5_data_2); set(h, 'LineWidth', 2); title('');
% 
% % Grid and axis styles
% grid on;
% ax = gca;
% ax.FontSize = 10;
% ax.LineWidth = 1.2;
% ax.Box = 'on';
% 
% ylabel('P(X < x)', 'FontSize', 12);
% xlabel('DoA error (º)', 'FontSize', 12);
% 
% % Legend
% legend({'$M_f = 4$', '$M_f = 40$'}, ...
%        'Interpreter', 'latex', 'FontSize', 10, 'Location', 'northeast');
% 
% set(gcf, 'Color', 'w');
% axis tight;
% print('figure_5','-depsc','-painters','-r300');

%% Generate fig 6 ## OK ##
% fig_6_data_1 = readmatrix('tmp.csv');
% fig_6_data_2 = readmatrix('fig_6_data_2.csv');
% fig_6_data_3 = readmatrix('fig_6_data_3.csv');
% hold off; hold on;
% K = 4; plot([K*ones([100,1]); (K/4)*ones([100,1])], 'LineWidth', 2);
% plot(fig_6_data_1, 'LineWidth', 2);
% plot(fig_6_data_2, 'LineWidth', 2);
% plot(fig_6_data_3, 'LineWidth', 2);
% 
% % Labels
% xlabel('Sample', 'FontSize', 12);
% ylabel('K estimate', 'FontSize', 12);
% 
% % Grid and axis styles
% grid on;
% ax = gca;
% ax.FontSize = 10;
% ax.LineWidth = 1.2;
% ax.Box = 'on';
% 
% % Legend
% legend({'Real', '$M_{ant} = 4$', '$M_{ant} = 16$', '$M_{ant} = 64$'}, ...
%        'Interpreter', 'latex', 'FontSize', 10, 'Location', 'northeast');
% 
% set(gcf, 'Color', 'w');
% axis tight;
% ylim([0, 21]);
% print('figure_6','-depsc','-painters','-r300');

%% Generate fig 7 ## OK ##
% fig_7_data_1 = readmatrix('fig_7_data_1.csv');
% fig_7_data_2 = readmatrix('fig_7_data_2.csv');
% fig_7_data_3 = readmatrix('fig_7_data_3.csv');
% hold off;
% hold on;
% h = cdfplot(fig_7_data_1); set(h, 'LineWidth', 2); title('');
% h = cdfplot(fig_7_data_2); set(h, 'LineWidth', 2); title('');
% h = cdfplot(fig_7_data_3); set(h, 'LineWidth', 2); title('');
% 
% % Grid and axis styles
% grid on;
% ax = gca;
% ax.FontSize = 10;
% ax.LineWidth = 1.2;
% ax.Box = 'on';
% 
% ylabel('P(X < x)', 'FontSize', 12);
% xlabel('DoA error (º)', 'FontSize', 12);
% 
% % Legend
% legend({'$M_{ant} = 4$', '$M_{ant} = 16$', '$M_{ant} = 64$'}, ...
%        'Interpreter', 'latex', 'FontSize', 10, 'Location', 'northeast');
% 
% set(gcf, 'Color', 'w');
% axis tight;
% print('figure_7','-depsc','-painters','-r300');

%% Generate fig 8
% fig_8_data_1 = readmatrix('fig_8_data_1.csv');
% fig_8_data_2 = readmatrix('fig_8_data_2.csv');
% fig_8_data_3 = readmatrix('fig_8_data_3.csv');
% fig_8_data_4 = readmatrix('fig_8_data_4.csv');
% fig_8_data_5 = readmatrix('fig_8_data_5.csv');
% fig_8_data_6 = readmatrix('fig_8_data_6.csv');
% fig_8_data_7 = readmatrix('fig_8_data_7.csv');
% hold off; hold on;
% k_values=1:0.5:10.5;
% plot(k_values, fig_8_data_1, 'LineWidth', 2);
% plot(k_values, fig_8_data_2, 'LineWidth', 2);
% plot(k_values, fig_8_data_3, 'LineWidth', 2);
% plot(k_values, fig_8_data_4, 'LineWidth', 2);
% plot(k_values, fig_8_data_5, 'LineWidth', 2);
% plot(k_values, fig_8_data_6, 'LineWidth', 2);
% plot(k_values, fig_8_data_7, 'LineWidth', 2);
% 
% % Labels
% xlabel('Real K', 'FontSize', 12);
% ylabel('Estimated K', 'FontSize', 12);
% 
% % Grid and axis styles
% grid on;
% ax = gca;
% ax.FontSize = 10;
% ax.LineWidth = 1.2;
% ax.Box = 'on';
% 
% % Legend
% legend({'$M_f = 4$', '$M_f = 10$', '$M_f = 16$', '$M_f = 22$', '$M_f = 28$', '$M_f = 34$', '$M_f = 40$'}, ...
%        'Interpreter', 'latex', 'FontSize', 10, 'Location', 'northwest');
% 
% set(gcf, 'Color', 'w');
% axis tight;
% ylim([0, 21]);
% print('figure_8','-depsc','-painters','-r300');

%% Generate fig 9
% fig_9_data_1 = readmatrix('fig_9_data_1.csv');
% fig_9_data_2 = readmatrix('fig_9_data_2.csv');
% hold off; hold on;
% scatter(fig_9_data_1(:,1),fig_9_data_1(:,2),'filled');
% scatter(fig_9_data_2(:,1),fig_9_data_2(:,2),'filled');
% 
% % Labels
% xlabel('X estimate [m]', 'FontSize', 12);
% ylabel('Y estimate [m]', 'FontSize', 12);
% 
% % Grid and axis styles
% grid on;
% ax = gca;
% ax.FontSize = 10;
% ax.LineWidth = 1.2;
% ax.Box = 'on';
% 
% % Legend
% legend({'$M_f = 2$', '$M_f = 40$'}, ...
%        'Interpreter', 'latex', 'FontSize', 10, 'Location', 'northwest');
% 
% set(gcf, 'Color', 'w');
% axis tight;
% xlim([-4,4]);
% ylim([-4,4]);
% print('figure_9','-depsc','-painters','-r300');