clear all;
close all;

% s = screen coordinates of calibration points
% e = pupil center coordinates in eye cam coordinates

% simulate a misaligned (slightly rotated) eye camera relative to toe scene
% camera
a = pi*0/180;
R = [cos(a), -sin(a);
     sin(a),  cos(a)];ed

% 3 point calibration 
%s = [0, 100, 100;
%     0,   0, 100]; 
%e = [10, 50, 50;
%     10, 10, 50];


%{
% 4 point calibration 
s = [0, 100, 100    0;
     0,   0, 100, 100]; 
e = [10, 50, 50, 10;
     10, 10, 50, 50];
%}

%%{
% 4 point real data
s = [257 556 551 265;
     154 151 345 334];

e = [545 485 489 551;
     235 233 263 265];
%}
 
% 5 point calibration 
%s = [0, 100, 100    0, 50;
%     0,   0, 100, 100, 50]; 
%e = [10, 50, 50, 10, 30;
%     10, 10, 50, 50, 30];


% 9 point calibration 
%s = [0, 100, 100    0, 50,  0, 50,  50, 100;
%     0,   0, 100, 100, 50, 50,  0, 100,  50]; 
%e = [10, 50, 50, 10, 30, 10, 30, 30, 50;
%     10, 10, 50, 50, 30, 30, 10, 50, 30];

%e = [5.1, 15.2, 15.1,  4.9;
%     2.9,  3.1, 15.2, 14.8];

%simulate pupil tracking noise 
%e = e + 0.5*randn(size(e));
%e

% rotate data to simulate misalignment
n_samples = size(e,2);
for i = 1:n_samples
    e(:,i) = R*e(:,i);
end

% s = W*e
% what is W ??
% W = s*pinv(e);

%tmp = pinv(e)
tmp = inv(e'*e) * e'; % alternative to pinv but not precise enough
W = s*tmp

W * (R*[50 50]')

% now add nonlinearities

tmp = poly_features(0,0);
ee = zeros(size(tmp,1), n_samples);
for i = 1 : n_samples
    ee(:,i) = poly_features(e(1,i),e(2,i));
end


ee_inv = pinv(ee);
%ee_inv = inv(ee'*ee) * ee'; % alternative to pinv
ee_inv
W = s*ee_inv

error = zeros(1,4);
for i = 1:4
    error(i) = norm(W*ee(:,i) - s(:,i));
end
error

figure(22);
tmp = W*ee;
plot(tmp(1,:),tmp(2,:), 'kx');
hold on;
plot(s(1,:), s(2,:), 'ro');

%%

figure(1);
n_subdiv = 25; 
X = zeros(n_subdiv,n_subdiv);
Y = zeros(n_subdiv,n_subdiv);
%xlin = linspace(10,50, n_subdiv);
%ylin = linspace(10,50, n_subdiv);

xlin = linspace(min(e(1,:)), max(e(1,:)), n_subdiv);
ylin = linspace(min(e(2,:)), max(e(2,:)), n_subdiv);


for i = 1:n_subdiv
    for k = 1:n_subdiv
        tmp = R*[xlin(i) ylin(k)]';
        tmp = W * poly_features(tmp(1), tmp(2));
        %X(i,k) = tmp(1);
        %Y(i,k) = tmp(2);
        plot(tmp(1), tmp(2), 'kx');
        hold on;
    end
end


function v = poly_features(x,y)
     v = [1, x, y, x*y]';%, x*x, y*y]';%, x*x*y*y]';%, x*x*x, y*y*y]';
end