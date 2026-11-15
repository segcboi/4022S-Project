import cv2
import mediapipe as mp
import socket
import time

# --- UDP NETWORK SETUP ---
UDP_IP = "172.20.10.3"  #ESP32
#UDP_IP = "127.0.0.1"
UDP_PORT = 5005       

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# 1. Strictly non-blocking: Never block the vision loop on network stack
sock.setblocking(False)

# 2. Lean send buffer: Prevent queuing stale telemetry frames if receiver lags
sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 8192)

# 3. Windows-specific fix: Disable ICMP Port Unreachable auto-close (WinError 10054)
# Prevents Python from crashing if the MATLAB receiver is closed or started later
if hasattr(socket, "SIO_UDP_CONNRESET"):
    try:
        sock.ioctl(socket.SIO_UDP_CONNRESET, False)
    except Exception:
        pass

print(f"Broadcasting to {UDP_IP}:{UDP_PORT} (Non-blocking UDP)...")

# --- MEDIAPIPE AI SETUP ---
# model_complexity: 0 = Lite (fastest, ideal for real-time arm tracking), 1 = Full, 2 = Heavy
MODEL_COMPLEXITY = 0
mp_pose = mp.solutions.pose
pose = mp_pose.Pose(
    static_image_mode=False,
    model_complexity=MODEL_COMPLEXITY,
    smooth_landmarks=True,
    min_detection_confidence=0.5,
    min_tracking_confidence=0.5
)
mp_draw = mp.solutions.drawing_utils

# --- CAMERA SETUP ---
# Prefer DirectShow on Windows for lower initialization and capture latency
cap = cv2.VideoCapture(0, cv2.CAP_DSHOW)
if not cap.isOpened():
    cap = cv2.VideoCapture(0)

# Minimize internal OpenCV capture buffer to avoid visual/telemetry lag
cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
cap.set(cv2.CAP_PROP_FPS, 30)

prev_time = time.time()
fps = 0.0

try:
    while cap.isOpened():
        success, image = cap.read()
        if not success:
            continue

        # FLIP THE IMAGE: Make it act like a mirror
        image = cv2.flip(image, 1)

        # MediaPipe Optimization: Mark image non-writeable before inference
        # Enables zero-copy memory access in the underlying C++ graph
        image_rgb = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
        image_rgb.flags.writeable = False
        results = pose.process(image_rgb)
        image_rgb.flags.writeable = True

        if results.pose_landmarks:
            mp_draw.draw_landmarks(image, results.pose_landmarks, mp_pose.POSE_CONNECTIONS)
            landmarks = results.pose_landmarks.landmark
            h, w, _ = image.shape
            
            # 1. Physical LEFT Arm (MediaPipe Right indices, mirrored by cv2.flip)
            l_sh = landmarks[12] 
            l_el = landmarks[14]    
            l_wr = landmarks[16]    

            # 2. Physical RIGHT Arm (MediaPipe Left indices, mirrored by cv2.flip)
            r_sh = landmarks[11]
            r_el = landmarks[13]
            r_wr = landmarks[15]

            # Visual Feedback: Highlight Leader (Right Arm) in bright green
            r_sh_px = (int(r_sh.x * w), int(r_sh.y * h))
            r_el_px = (int(r_el.x * w), int(r_el.y * h))
            r_wr_px = (int(r_wr.x * w), int(r_wr.y * h))
            cv2.line(image, r_sh_px, r_el_px, (0, 255, 0), 4)
            cv2.line(image, r_el_px, r_wr_px, (0, 255, 0), 4)
            cv2.circle(image, r_sh_px, 8, (0, 255, 0), -1)
            cv2.circle(image, r_el_px, 8, (0, 255, 0), -1)
            cv2.circle(image, r_wr_px, 8, (0, 255, 0), -1)
            cv2.putText(image, "LEADER (Active CV)", (r_sh_px[0] - 50, r_sh_px[1] - 20),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)

            # Highlight Follower (Left Arm) in Cyan
            l_sh_px = (int(l_sh.x * w), int(l_sh.y * h))
            cv2.putText(image, "FOLLOWER (Inverted)", (l_sh_px[0] - 50, l_sh_px[1] - 20),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 200, 0), 1)
            
            # 3. PACKAGE THE UDP MESSAGE (12 variables)
            # Format: L_sh_x,L_sh_y, L_el_x,L_el_y, L_wr_x,L_wr_y, R_sh_x,R_sh_y, R_el_x,R_el_y, R_wr_x,R_wr_y
            message = (
                f"{l_sh.x:.4f},{l_sh.y:.4f},"
                f"{l_el.x:.4f},{l_el.y:.4f},"
                f"{l_wr.x:.4f},{l_wr.y:.4f},"
                f"{r_sh.x:.4f},{r_sh.y:.4f},"
                f"{r_el.x:.4f},{r_el.y:.4f},"
                f"{r_wr.x:.4f},{r_wr.y:.4f}"
            )
            
            # 4. Strictly Non-blocking (Fire-and-forget) transmission
            try:
                sock.sendto(message.encode('utf-8'), (UDP_IP, UDP_PORT))
            except (BlockingIOError, OSError):
                # If OS network buffer is full or unreachable, drop packet and proceed immediately
                pass

        # Compute smoothed live FPS and render on frame
        curr_time = time.time()
        fps = 0.9 * fps + 0.1 * (1.0 / max(curr_time - prev_time, 1e-6))
        prev_time = curr_time
        cv2.putText(image, f"FPS: {fps:.1f}", (20, 35), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)
        cv2.putText(image, "RIGHT: Active CV | LEFT: Inverted Follower", (20, 65),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 2)
        cv2.putText(image, "Cycle Dance Modes (1/2/3): Press ESP32 Button", (20, 95),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (180, 220, 255), 1)

        cv2.imshow('Dual Arm Tracking', image)
        
        # Reduced waitKey from 5ms to 1ms to eliminate artificial sleep latency
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

finally:
    cap.release()
    cv2.destroyAllWindows()
    sock.close()
    print("Cleaned up camera, windows, and UDP socket.")
