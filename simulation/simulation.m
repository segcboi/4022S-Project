% sim_comp.m - Complementary Kinetic Sculpture (Leader-Follower Canon)
% Dynamics: 40 kg*cm DS3240 Actuators, Laban Movement Analysis, Torso Angle Sensing
clear; clc; close all;

%% Mechanical & CAD Specifications (40 kg*cm DS3240 Actuators)
L1 = 0.20;          % Upper arm length (m)
L2 = 0.20;          % Forearm length (m)
d_base = 0.0;       % Coaxial central shaft
link_w = 0.05;     % Maximum link width (m)
TRAIL_LEN = 220;    % Deep luminous ribbon history
base_L = [-d_base/2, 0];
base_R = [ d_base/2, 0];

%% Actuator Physical Dynamics & Limits
% DS3240 Servo Specifications:
% Rated Torque: 40.0 kg*cm = 40.0 * 0.0980665 N*m = 3.923 N*m
TORQUE_MAX = 3.92;  % Maximum motor torque ceiling (N*m)
MAX_VEL    = 2.5;   % Maximum loaded angular velocity (rad/s, ~200 deg/s safe limit)
LIMIT_SERVO = [-deg2rad(120), deg2rad(120)]; % ±120° from upright vertical

% Structural Mass & Inertia Model for 2-Link Robotic Arms
% Link 1 (Upper Arm + Elbow Servo):
m1 = 0.20; r1 = 0.14; I1 = 0.0013; m_servo = 0.065;
% Link 2 (Forearm):
m2 = 0.14; r2 = 0.11; I2 = 0.0006;
g_acc = 9.81; % Gravitational acceleration (m/s^2)
b_fric = 0.045; % Viscous joint friction (N*m*s/rad) — higher for structural damping

%% Visual Canvas Architecture (Obsidian & Bioluminescence)
fig = figure('Name', 'Kinetic Sculpture - Laban Dynamics & 40kg*cm Echo', ...
             'Color', [0.03 0.03 0.04], 'NumberTitle', 'off', 'MenuBar', 'none');
ax = axes('Parent', fig, 'Color', [0.02 0.02 0.025]);
hold(ax, 'on'); axis(ax, 'equal'); axis(ax, 'off');
xlim(ax, [-0.72, 0.72]); ylim(ax, [-0.70, 0.70]);

% -------------------------------------------------------------
% 1. BACKGROUND LAYER: Translucent Ghost Human Skeleton
% -------------------------------------------------------------
c_ghost = [0.35, 0.42, 0.58, 0.32]; 
h_ghost_head = plot(ax, NaN, NaN, 'o', 'Color', c_ghost, 'MarkerSize', 15, 'LineWidth', 1.5);
h_ghost_body = plot(ax, NaN, NaN, '-', 'Color', c_ghost, 'LineWidth', 1.5);
h_ghost_L    = plot(ax, NaN, NaN, 'o-', 'Color', c_ghost, 'LineWidth', 1.5, 'MarkerSize', 4, 'MarkerFaceColor', c_ghost(1:3));
h_ghost_R    = plot(ax, NaN, NaN, 'o-', 'Color', c_ghost, 'LineWidth', 1.5, 'MarkerSize', 4, 'MarkerFaceColor', c_ghost(1:3));

% -------------------------------------------------------------
% 2. FOREGROUND LAYER: The Kinetic Sculpture
% -------------------------------------------------------------
% Monolithic Coaxial Pivot Hub
plot(ax, 0, 0, 'o', 'MarkerSize', 21, 'MarkerFaceColor', [0.18 0.18 0.22], 'MarkerEdgeColor', [0.10 0.10 0.14], 'LineWidth', 2);
plot(ax, 0, 0, 'o', 'MarkerSize', 7,  'MarkerFaceColor', [0.45 0.50 0.65], 'MarkerEdgeColor', 'none');

% Aesthetic Palettes:
% Arm 1 (Leader): Neon Crimson / Rose
c_L_wrist = [0.98, 0.25, 0.45]; c_L_elbow = [0.65, 0.15, 0.30];
% Arm 2 (Follower): Bioluminescent Azure / Cyan
c_R_wrist = [0.10, 0.85, 0.95]; c_R_elbow = [0.05, 0.40, 0.60];

% Luminous Ribbon Trails
h_wr_trail_L = plot(ax, NaN, NaN, '-', 'Color', [c_L_wrist, 0.85], 'LineWidth', 2.8);
h_wr_trail_R = plot(ax, NaN, NaN, '-', 'Color', [c_R_wrist, 0.85], 'LineWidth', 2.8);
h_el_trail_L = plot(ax, NaN, NaN, ':', 'Color', [c_L_elbow, 0.50], 'LineWidth', 1.5);
h_el_trail_R = plot(ax, NaN, NaN, ':', 'Color', [c_R_elbow, 0.50], 'LineWidth', 1.5);

% Tapered Sculpture Links
h_link_L1 = patch(ax, 'XData', [], 'YData', [], 'FaceColor', [0.85 0.20 0.35], 'FaceAlpha', 0.82, 'EdgeColor', 'none');
h_link_L2 = patch(ax, 'XData', [], 'YData', [], 'FaceColor', [0.95 0.35 0.50], 'FaceAlpha', 0.90, 'EdgeColor', 'none');
h_link_R1 = patch(ax, 'XData', [], 'YData', [], 'FaceColor', [0.05 0.60 0.85], 'FaceAlpha', 0.82, 'EdgeColor', 'none');
h_link_R2 = patch(ax, 'XData', [], 'YData', [], 'FaceColor', [0.15 0.80 0.98], 'FaceAlpha', 0.90, 'EdgeColor', 'none');

% Luminous Pivot Bearings and Wrist Glow
h_pivots = plot(ax, NaN, NaN, 'o', 'MarkerSize', 7, 'MarkerFaceColor', [0.88 0.88 0.92], 'MarkerEdgeColor', [0.1 0.1 0.1]);
h_glow   = plot(ax, NaN, NaN, 'o', 'MarkerSize', 5.5, 'MarkerFaceColor', [1 1 1], 'MarkerEdgeColor', 'none');

% -------------------------------------------------------------
% 3. Typographic HUD & Real-Time Telemetry Badges
% -------------------------------------------------------------
text(ax, 0, 0.66, 'KINETIC SCULPTURE', ...
    'HorizontalAlignment', 'center', 'Color', [0.82 0.85 0.92], ...
    'FontSize', 11, 'FontWeight', 'bold', 'FontName', 'Helvetica');


h_txt_laban = text(ax, 0, -0.58, 'LMA EFFORT: POISED STILLNESS  |  BODY ORIENTATION: 0.0°', ...
    'HorizontalAlignment', 'center', 'Color', [0.45 0.85 0.95], ...
    'FontSize', 8.5, 'FontName', 'Helvetica');
h_txt_torque = text(ax, 0, -0.62, 'TORQUE LOAD: L1: 0.0 Nm (0%) | R1: 0.0 Nm (0%) [LIMIT: 40 kg*cm]', ...
    'HorizontalAlignment', 'center', 'Color', [0.90 0.75 0.40], ...
    'FontSize', 8.5, 'FontName', 'Courier');
h_txt_status = text(ax, 0, -0.66, 'STATUS: INITIALIZING TELEMETRY (UDP 5005)...', ...
    'HorizontalAlignment', 'center', 'Color', [0.40 0.80 0.65], ...
    'FontSize', 8.5, 'FontName', 'Courier');

%% Java UDP Socket Setup (Port 5005)
import java.net.DatagramSocket java.net.DatagramPacket java.net.InetSocketAddress;
socket = DatagramSocket([]);
socket.setReuseAddress(true);
socket.bind(InetSocketAddress(5005));
socket.setSoTimeout(4);
cleanupObj = onCleanup(@() socket.close());
packetBuffer = zeros(1, 1024, 'int8');
packet = DatagramPacket(packetBuffer, length(packetBuffer));

%% Kinematic & Dynamic State Buffers
% 0 rad = pointing straight UP (Zenith, +y).
% Arm 1: q_pos(1:2) [Shoulder; Elbow]
% Arm 2: q_pos(3:4) [Shoulder; Elbow]
q_pos = [deg2rad(30); -deg2rad(45); -deg2rad(30); deg2rad(45)];
q_vel = [0; 0; 0; 0];
tau_applied = [0; 0; 0; 0];

% Human Pose & Body Orientation State
q_human_raw      = [0; 0]; 
q_human_filt     = [0; 0];
q_human_prev     = [0; 0];
dq_prev          = [0; 0];
user_energy      = 0;
sh_dist_baseline = 0.28;   % Running baseline for maximum frontal shoulder width
body_yaw_filt    = 0.0;    % Smoothed body orientation angle (rad)
body_foreshorten = 1.0;    % Shoulder distance foreshortening ratio

% Laban Movement Analysis (LMA) Continuous Effort Factors
lma_weight    = 0.1;       % Weight: 0.0 (Light) to 1.0 (Strong)
lma_time      = 0.9;       % Time:   0.0 (Sudden) to 1.0 (Sustained)
lma_space     = 0.5;       % Space:  0.0 (Indirect) to 1.0 (Direct)
lma_flow      = 0.5;       % Flow:   0.0 (Bound) to 1.0 (Free)
lma_category  = 'POISED STILLNESS';
amp_gain      = 1.0;       % LMA-driven amplitude multiplier

% Fluid Impedance Modulators (Tuned for Lively but Structurally Safe Response)
omega_n = 5.0;
zeta    = 0.72;
w_human = 0.50;
w_wave  = 0.50;

% Kinetic Echo Ring Buffer for Arm 2 (Follower)
% Arm 2 samples Arm 1's position and velocity from this buffer
DELAY_STEPS = 22; % ~250-300ms temporal canon delay
arm1_buffer = repmat([q_pos(1); q_pos(2); 0; 0], 1, DELAY_STEPS);
buf_idx = 1;

% Multi-Layered Harmonic Breathing Generator (Irrational Frequency Ratios)
% 5 oscillators with incommensurate frequencies prevent exact repetition
phi = zeros(5, 1);
w_harm = [0.30; ...                             % Base pulse
          0.30 * sqrt(2); ...                    % sqrt(2) harmonic
          0.30 * ((1 + sqrt(5))/2); ...           % Golden ratio harmonic
          0.30 * pi/2; ...                        % Pi/2 harmonic
          0.30 * sqrt(3)];                        % sqrt(3) harmonic

wr_hist_L = NaN(2, TRAIL_LEN); wr_hist_R = NaN(2, TRAIL_LEN);
el_hist_L = NaN(2, TRAIL_LEN); el_hist_R = NaN(2, TRAIL_LEN);
last_time = tic;

disp('Fluid Laban Kinetic Sculpture Active (40 kg*cm Motor Torque Mode).');

%% Main Simulation Loop
while ishandle(fig)
    dt = toc(last_time);
    last_time = tic;
    dt = min(max(dt, 0.001), 0.04);
    
    % Flush buffer to read the freshest available UDP frame
    gotPacket = false;
    while true
        try
            socket.receive(packet);
            gotPacket = true;
        catch
            break;
        end
    end
    
    if gotPacket
        rawBytes = packet.getData();
        raw = char(rawBytes(1:packet.getLength()))';
        vals = sscanf(raw, '%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f');
        if numel(vals) == 12
            % Invert Y from screen space (downwards) to Cartesian space (upwards)
            l_sh = [vals(1), -vals(2)];  l_el = [vals(3), -vals(4)];  l_wr = [vals(5), -vals(6)];
            r_sh = [vals(7), -vals(8)];  r_el = [vals(9), -vals(10)]; r_wr = [vals(11), -vals(12)];
            v_L1 = l_el - l_sh; v_L2 = l_wr - l_el;
            v_R1 = r_el - r_sh; v_R2 = r_wr - r_el;
            
            % Update Ghost Skeleton (Background Visualizer)
            if norm(v_L1) > 1e-3 && norm(v_R1) > 1e-3
                scale_ghost = 0.16;
                u_L1 = (v_L1 / norm(v_L1)) * scale_ghost;
                u_L2 = (v_L2 / norm(v_L2)) * (scale_ghost * 0.85);
                u_R1 = (v_R1 / norm(v_R1)) * scale_ghost;
                u_R2 = (v_R2 / norm(v_R2)) * (scale_ghost * 0.85);
                
                g_sh_L = [-0.14, 0.28]; 
                g_sh_R = [ 0.14, 0.28];
                g_head = [ 0.00, 0.40];
                g_el_L = g_sh_L + u_L1; g_wr_L = g_el_L + u_L2;
                g_el_R = g_sh_R + u_R1; g_wr_R = g_el_R + u_R2;
                
                set(h_ghost_head, 'XData', g_head(1), 'YData', g_head(2));
                set(h_ghost_body, 'XData', [g_sh_L(1), g_sh_R(1)], 'YData', [g_sh_L(2), g_sh_R(2)]);
                set(h_ghost_L, 'XData', [g_sh_L(1), g_el_L(1), g_wr_L(1)], 'YData', [g_sh_L(2), g_el_L(2), g_wr_L(2)]);
                set(h_ghost_R, 'XData', [g_sh_R(1), g_el_R(1), g_wr_R(1)], 'YData', [g_sh_R(2), g_el_R(2), g_wr_R(2)]);
            end
            
            % -------------------------------------------------------------
            % BODY ANGLE & SHOULDER FORESHORTENING DETECTION
            % -------------------------------------------------------------
            d_sh = norm(r_sh - l_sh);
            % Adaptively track user's frontal shoulder width baseline
            if d_sh > sh_dist_baseline
                sh_dist_baseline = 0.98 * sh_dist_baseline + 0.02 * d_sh;
            else
                sh_dist_baseline = 0.999 * sh_dist_baseline + 0.001 * d_sh;
            end
            sh_dist_baseline = max(sh_dist_baseline, 0.14);
            
            % Foreshortening ratio: rho = 1.0 (frontal) down to 0.3 (profile)
            rho = min(max(d_sh / sh_dist_baseline, 0.25), 1.0);
            body_foreshorten = rho;
            yaw_mag = acos(rho); % Torso angle from camera normal (rad)
            
            % Determine direction of torso angle (left vs right rotation)
            sh_center_x = (l_sh(1) + r_sh(1)) / 2;
            % Shift in shoulder relative positions indicates turn sign
            turn_bias = (r_sh(1) + l_sh(1)) - 1.0; % Offset from screen center
            turn_sign = sign(turn_bias);
            if abs(turn_bias) < 0.03, turn_sign = sign(r_sh(2) - l_sh(2)); end
            if turn_sign == 0, turn_sign = 1; end
            raw_yaw = yaw_mag * turn_sign;
            
            % Smooth body angle with a fluid low-pass filter
            body_yaw_filt = body_yaw_filt + min(1.0, 3.5 * dt) * (raw_yaw - body_yaw_filt);
            
            % -------------------------------------------------------------
            % ACTIVE GESTURING ARM SELECTION
            % -------------------------------------------------------------
            act_L = norm(v_L1) * (1.0 + max(0, l_wr(2) - l_sh(2) + 0.3));
            act_R = norm(v_R1) * (1.0 + max(0, r_wr(2) - r_sh(2) + 0.3));
            
            if act_R >= act_L
                v_act1 = v_R1; v_act2 = v_R2;
            else
                v_act1 = v_L1; v_act2 = v_L2;
            end
            
            if norm(v_act1) > 1e-3 && norm(v_act2) > 1e-3
                world_ang = atan2(v_act1(2), v_act1(1));
                % Map Human arm angle to Zenith:
                % Hanging down (-pi/2) -> 0 (Zenith straight UP)
                % Pointing right (0)   -> -pi/2 (Lean right)
                % Pointing left (pi)   -> +pi/2 (Lean left)
                q_human_raw(1) = atan2(sin(-world_ang - pi/2), cos(-world_ang - pi/2));
                
                % Elbow relative flexion
                cross_prod = v_act1(1)*v_act2(2) - v_act1(2)*v_act2(1);
                dot_prod   = dot(v_act1, v_act2);
                q_human_raw(2) = atan2(cross_prod, dot_prod);
            end
        end
    end
    
    % --- 1. Adaptive Pose Filtering (LMA-Responsive Tracking Speed) ---
    % Sudden movement -> track faster; Sustained -> smoother glide
    alpha_base = 5.0 + 7.0 * (1.0 - lma_time);   % 5 Hz (sustained) to 12 Hz (sudden)
    alpha_lp = min(1.0, alpha_base * dt);
    q_human_filt = q_human_filt + alpha_lp * (q_human_raw - q_human_filt);
    
    % Track Gesture Velocity and Acceleration
    dq_human = (q_human_filt - q_human_prev) / dt;
    q_human_prev = q_human_filt;
    user_energy = (1 - 2.0 * dt) * user_energy + (2.0 * dt) * norm(dq_human);
    user_energy = min(max(user_energy, 0), 2.5);
    
    % --- 2. LABAN MOVEMENT ANALYSIS — EFFORT QUALITY ENGINE ---
    % Effort Factor 1: WEIGHT (Light <-> Strong) — force / intensity
    lma_weight = min(max(user_energy / 1.2, 0.0), 1.0);
    
    % Effort Factor 2: TIME (Sudden <-> Sustained) — acceleration / jerk
    jerk_human = norm(dq_human - dq_prev) / max(dt, 0.003);
    dq_prev = dq_human;
    suddenness = min(max(jerk_human / 4.5, 0.0), 1.0);
    lma_time = (1 - 3.5 * dt) * lma_time + (3.5 * dt) * (1.0 - suddenness);
    
    % Effort Factor 3: SPACE (Indirect <-> Direct) — path directness
    dir_measure = abs(cos(q_human_filt(1)));
    lma_space = (1 - 2.5 * dt) * lma_space + (2.5 * dt) * dir_measure;
    
    % Effort Factor 4: FLOW (Bound <-> Free) — velocity consistency
    speed_now = norm(dq_human);
    if speed_now > 0.15
        flow_raw = 1.0 - min(jerk_human / (speed_now * 8.0 + 1.0), 1.0);
    else
        flow_raw = 0.5;
    end
    lma_flow = (1 - 2.0 * dt) * lma_flow + (2.0 * dt) * flow_raw;
    
    % Categorize into Laban's 8 Basic Effort Actions
    if user_energy < 0.12
        lma_category = 'POISED STILLNESS';
    elseif lma_weight < 0.40 && lma_time > 0.55 && lma_space < 0.50
        lma_category = 'FLOAT (Light Sustained Indirect)';
    elseif lma_weight < 0.40 && lma_time > 0.55 && lma_space >= 0.50
        lma_category = 'GLIDE (Light Sustained Direct)';
    elseif lma_weight < 0.40 && lma_time <= 0.55 && lma_space < 0.50
        lma_category = 'FLICK (Light Sudden Indirect)';
    elseif lma_weight < 0.40 && lma_time <= 0.55 && lma_space >= 0.50
        lma_category = 'DAB (Light Sudden Direct)';
    elseif lma_weight >= 0.40 && lma_time > 0.55 && lma_space < 0.50
        lma_category = 'WRING (Strong Sustained Indirect)';
    elseif lma_weight >= 0.40 && lma_time > 0.55 && lma_space >= 0.50
        lma_category = 'PRESS (Strong Sustained Direct)';
    elseif lma_weight >= 0.40 && lma_time <= 0.55 && lma_space < 0.50
        lma_category = 'SLASH (Strong Sudden Indirect)';
    else
        lma_category = 'PUNCH (Strong Sudden Direct)';
    end
    
    % --- EFFORT-TO-DYNAMICS: Leader MATCHES person's movement quality ---
    % Time: Sudden -> responsive | Sustained -> smooth, flowing
    omega_target = 4.0 + 3.5 * (1.0 - lma_time);     % 4.0 (sustained) to 7.5 (sudden)
    zeta_target  = 0.65 + 0.20 * lma_time;             % 0.65 (lively) to 0.85 (smooth)
    
    % Weight: Strong -> dominant human tracking | Light -> gentler, more wave
    w_human_target = 0.45 + 0.35 * lma_weight;         % 0.45 to 0.80
    w_wave_target  = 1.0 - w_human_target;
    
    % Amplitude: Strong effort -> bigger leader gestures (capped for safety)
    amp_target = 0.80 + 0.35 * lma_weight;             % 0.80 to 1.15
    
    % Wave speed matches person's energy level
    flow_speed_target = 0.5 + 1.0 * user_energy / 2.5;
    
    % Smooth interpolation — buttery transitions between effort states
    alpha_lma = min(1.0, 3.5 * dt);
    omega_n  = omega_n  + alpha_lma * (omega_target - omega_n);
    zeta     = zeta     + alpha_lma * (zeta_target - zeta);
    w_human  = w_human  + alpha_lma * (w_human_target - w_human);
    w_wave   = w_wave   + alpha_lma * (w_wave_target - w_wave);
    amp_gain = amp_gain + alpha_lma * (amp_target - amp_gain);
    
    % --- 3. Organic Breathing Wavelets (Multi-Layered Flow-Modulated) ---
    phi = phi + (w_harm * flow_speed_target) * dt;
    % Free flow -> wider, expressive waves; Bound -> tighter, controlled
    wave_range = 0.5 + 0.5 * lma_flow;
    
    % Shoulder wave: layered harmonics for rich, never-repeating organic flow
    wave_1 = wave_range * (0.55 * sin(phi(1)) ...
                         + 0.25 * sin(phi(2) + pi/3) ...
                         + 0.15 * cos(phi(3)) ...
                         + 0.10 * sin(phi(4) * 0.7));
    
    % Elbow wave: phase-offset cascade for flowing wrist-trails
    wave_2 = wave_range * (0.50 * cos(phi(1) * 1.3 + pi/4) ...
                         + 0.25 * sin(phi(3) * 0.9 + pi/6) ...
                         + 0.15 * cos(phi(5) * 1.1) ...
                         + 0.10 * sin(phi(2) * 0.6 + pi/3));
    
    % --- 4. ARM 1 (LEADER): Complementary Pose, MATCHING Movement Quality ---
    % Incorporate Body Angle from Camera (Torso Yaw):
    % A. Conversational Horizon Bias: Shifts equilibrium toward person's facing angle
    body_bias = 0.40 * body_yaw_filt;
    
    % B. Kinesphere Depth Modulation:
    % When person turns in profile (rho < 0.75), deepen crescent elbow curls
    depth_curl_scale = 1.0 + 0.60 * (1.0 - body_foreshorten);
    
    target_comp_1 = (q_human_filt(1) * 1.15) + body_bias;
    target_comp_2 = (-q_human_filt(2) * 1.85 - 0.40 * sin(q_human_filt(1))) * depth_curl_scale;
    
    % Apply LMA amplitude gain — Strong effort -> bigger leader gestures
    target_comp_1 = target_comp_1 * amp_gain;
    target_comp_2 = target_comp_2 * amp_gain;
    
    % Blend gesture complement with breathing waves
    target_L1 = w_human * target_comp_1 + w_wave * wave_1;
    target_L2 = w_human * target_comp_2 + w_wave * wave_2;
    target_L1 = min(max(target_L1, LIMIT_SERVO(1)), LIMIT_SERVO(2));
    target_L2 = min(max(target_L2, LIMIT_SERVO(1)), LIMIT_SERVO(2));
    
    % --- 5. ARM 2 (FOLLOWER): Clean Inverted Echo of Arm 1 ---
    % Arm 2 receives ZERO human input. It mirrors Arm 1's actual state
    % from the ring buffer with pure chiral inversion and a time delay.
    arm1_delayed = arm1_buffer(:, mod(buf_idx, DELAY_STEPS) + 1);
    q1_hist_base  = arm1_delayed(1);
    q1_hist_elbow = arm1_delayed(2);
    
    % Pure Chiral Inversion: mirror the leader's shoulder across zenith
    target_R1 = -q1_hist_base;
    
    % Inverted elbow with subtle softening for organic feel
    target_R2 = -q1_hist_elbow * 0.92;
    
    target_R1 = min(max(target_R1, LIMIT_SERVO(1)), LIMIT_SERVO(2));
    target_R2 = min(max(target_R2, LIMIT_SERVO(1)), LIMIT_SERVO(2));
    
    q_target = [target_L1; target_L2; target_R1; target_R2];
    
    % --- 6. 40 kg*cm DS3240 MOTOR DYNAMICS & TORQUE CONSTRAINTS ---
    % Model the true physical mass, inertia, gravity, and 40 kg*cm torque limits for both arms
    
    % A. ARM 1 DYNAMICS (Leader)
    c2_1 = cos(q_pos(2));
    M11_1 = I1 + m1*r1^2 + (m_servo + m2)*L1^2 + I2 + m2*r2^2 + 2*m2*L1*r2*c2_1;
    M12_1 = I2 + m2*r2^2 + m2*L1*r2*c2_1;
    M22_1 = I2 + m2*r2^2;
    M_1 = [M11_1, M12_1; M12_1, M22_1];
    
    % Gravity torques (Zenith reference: link 1 world angle = pi/2 + q1)
    G1_1 = (m1*r1 + (m_servo + m2)*L1)*g_acc*sin(q_pos(1)) + m2*r2*g_acc*sin(q_pos(1) + q_pos(2));
    G2_1 = m2*r2*g_acc*sin(q_pos(1) + q_pos(2));
    G_1  = [G1_1; G2_1];
    
    % Impedance torque demand:
    pos_err_1 = q_target(1:2) - q_pos(1:2);
    pos_err_1(1) = atan2(sin(pos_err_1(1)), cos(pos_err_1(1)));
    tau_demand_1 = M_1 * ((omega_n^2) * pos_err_1 - (2.0 * zeta * omega_n) * q_vel(1:2)) + G_1;
    
    % Actuator Torque Saturation: STRICTLY CLAMPED TO 40 kg*cm (3.92 N*m)
    tau_applied(1:2) = min(max(tau_demand_1, -TORQUE_MAX), TORQUE_MAX);
    
    % Compute physical angular acceleration under torque limits:
    MAX_ACC = 15.0; % Maximum angular acceleration (rad/s^2) — structural safety limit
    q_acc_1 = M_1 \ (tau_applied(1:2) - G_1 - b_fric * q_vel(1:2));
    q_acc_1 = min(max(q_acc_1, -MAX_ACC), MAX_ACC);
    q_vel(1:2) = q_vel(1:2) + q_acc_1 * dt;
    q_vel(1:2) = min(max(q_vel(1:2), -MAX_VEL), MAX_VEL);
    q_pos(1:2) = q_pos(1:2) + q_vel(1:2) * dt;
    
    % B. ARM 2 DYNAMICS (Follower)
    c2_2 = cos(q_pos(4));
    M11_2 = I1 + m1*r1^2 + (m_servo + m2)*L1^2 + I2 + m2*r2^2 + 2*m2*L1*r2*c2_2;
    M12_2 = I2 + m2*r2^2 + m2*L1*r2*c2_2;
    M22_2 = I2 + m2*r2^2;
    M_2 = [M11_2, M12_2; M12_2, M22_2];
    
    G1_2 = (m1*r1 + (m_servo + m2)*L1)*g_acc*sin(q_pos(3)) + m2*r2*g_acc*sin(q_pos(3) + q_pos(4));
    G2_2 = m2*r2*g_acc*sin(q_pos(3) + q_pos(4));
    G_2  = [G1_2; G2_2];
    
    pos_err_2 = q_target(3:4) - q_pos(3:4);
    pos_err_2(1) = atan2(sin(pos_err_2(1)), cos(pos_err_2(1)));
    tau_demand_2 = M_2 * ((omega_n^2) * pos_err_2 - (2.0 * zeta * omega_n) * q_vel(3:4)) + G_2;
    
    tau_applied(3:4) = min(max(tau_demand_2, -TORQUE_MAX), TORQUE_MAX);
    
    q_acc_2 = M_2 \ (tau_applied(3:4) - G_2 - b_fric * q_vel(3:4));
    q_acc_2 = min(max(q_acc_2, -MAX_ACC), MAX_ACC);
    q_vel(3:4) = q_vel(3:4) + q_acc_2 * dt;
    q_vel(3:4) = min(max(q_vel(3:4), -MAX_VEL), MAX_VEL);
    q_pos(3:4) = q_pos(3:4) + q_vel(3:4) * dt;
    
    % Enforce Hard Mechanical Servo Stops
    for idx = 1:4
        if q_pos(idx) <= LIMIT_SERVO(1)
            q_pos(idx) = LIMIT_SERVO(1); q_vel(idx) = 0;
        elseif q_pos(idx) >= LIMIT_SERVO(2)
            q_pos(idx) = LIMIT_SERVO(2); q_vel(idx) = 0;
        end
    end
    
    % Store Arm 1's actual state into the ring buffer for Arm 2 to follow
    arm1_buffer(:, buf_idx) = [q_pos(1); q_pos(2); q_vel(1); q_vel(2)];
    buf_idx = mod(buf_idx, DELAY_STEPS) + 1;
    
    % --- 7. Forward Kinematics (Zenith Reference) ---
    th_L1 = pi/2 + q_pos(1);
    th_L2 = th_L1 + q_pos(2);
    el_L  = base_L + [L1 * cos(th_L1), L1 * sin(th_L1)]; 
    wr_L  = el_L   + [L2 * cos(th_L2), L2 * sin(th_L2)];
    
    th_R1 = pi/2 + q_pos(3);      
    th_R2 = th_R1 + q_pos(4);
    el_R  = base_R + [L1 * cos(th_R1), L1 * sin(th_R1)]; 
    wr_R  = el_R   + [L2 * cos(th_R2), L2 * sin(th_R2)];
    
    % --- 8. Render Links & Ribbons ---
    renderTaperedLink(h_link_L1, base_L, el_L, link_w, link_w * 0.72);
    renderTaperedLink(h_link_L2, el_L,   wr_L, link_w * 0.72, link_w * 0.40);
    renderTaperedLink(h_link_R1, base_R, el_R, link_w, link_w * 0.72);
    renderTaperedLink(h_link_R2, el_R,   wr_R, link_w * 0.72, link_w * 0.40);
    
    set(h_pivots, 'XData', [base_L(1), el_L(1), base_R(1), el_R(1)], ...
                  'YData', [base_L(2), el_L(2), base_R(2), el_R(2)]);
    set(h_glow, 'XData', [wr_L(1), wr_R(1)], 'YData', [wr_L(2), wr_R(2)]);
              
    wr_hist_L = [wr_L', wr_hist_L(:, 1:end-1)]; wr_hist_R = [wr_R', wr_hist_R(:, 1:end-1)];
    el_hist_L = [el_L', el_hist_L(:, 1:end-1)]; el_hist_R = [el_R', el_hist_R(:, 1:end-1)];
    
    set(h_wr_trail_L, 'XData', wr_hist_L(1, :), 'YData', wr_hist_L(2, :));
    set(h_wr_trail_R, 'XData', wr_hist_R(1, :), 'YData', wr_hist_R(2, :));
    set(h_el_trail_L, 'XData', el_hist_L(1, :), 'YData', el_hist_L(2, :));
    set(h_el_trail_R, 'XData', el_hist_R(1, :), 'YData', el_hist_R(2, :));
    
    % --- 9. Real-Time HUD & Telemetry Updates ---
    yaw_deg = rad2deg(body_yaw_filt);
    if lma_flow > 0.60, flow_str = 'FREE'; elseif lma_flow < 0.40, flow_str = 'BOUND'; else, flow_str = 'FLUID'; end
    set(h_txt_laban, 'String', sprintf('LMA: %s | FLOW: %s | BODY: %+0.1f° (%2.0f%%)', ...
                                       lma_category, flow_str, yaw_deg, body_foreshorten * 100));
    
    torque_pct_L = (abs(tau_applied(1)) / TORQUE_MAX) * 100;
    torque_pct_R = (abs(tau_applied(3)) / TORQUE_MAX) * 100;
    set(h_txt_torque, 'String', sprintf('TORQUE LOAD: L1: %0.2f Nm (%2.0f%%) | R1: %0.2f Nm (%2.0f%%) [40 kg*cm RATED]', ...
                                        abs(tau_applied(1)), torque_pct_L, abs(tau_applied(3)), torque_pct_R));
    
    if gotPacket
        set(h_txt_status, 'String', sprintf('COUPLED DIALOGUE | HUMAN ENERGY: %3.0f%% | ARM 2 ECHO LAG: %0.0f ms', ...
                                            user_energy * 50, DELAY_STEPS * dt * 1000), ...
                          'Color', [0.35 0.90 0.65]);
    else
        set(h_txt_status, 'String', 'POISED BREATHING | WAITING FOR TELEMETRY (UDP 5005)', ...
                          'Color', [0.70 0.70 0.45]);
    end
    
    drawnow limitrate; 
    pause(0.003);
end

%% Architectural Link Geometry (Tapered Form Factor)
function renderTaperedLink(patchHandle, p1, p2, w1, w2)
    vec = p2 - p1; len = norm(vec);
    if len < 1e-5, return; end
    u = vec / len; n = [-u(2), u(1)];
    corners = [p1 + n * (w1 / 2); p2 + n * (w2 / 2); p2 - n * (w2 / 2); p1 - n * (w1 / 2)];
    set(patchHandle, 'XData', corners(:, 1), 'YData', corners(:, 2));
end
