% testing RBM generation

alpha = 1.0;

generate_test_data2

% cov(X)
% X = 2*X;
% cov(X)

rbm = calculate_gbrbm(X, 25, 1, 10);

Y = reconstruct_gbrbm_data(X, rbm.W, rbm.a, rbm.b, rbm.z, rbm.CDk, alpha);

figure(1);
hold off;
plot(X(:,1), X(:,2), 'go');
hold on;
plot(Y(:,1), Y(:,2), 'bo');


