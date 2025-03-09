import cv2
import subprocess
import time
import os
import numpy as np
from ultralytics import YOLO
import torch
import random
from collections import deque
from datetime import datetime
from itertools import combinations
from requests.exceptions import ChunkedEncodingError, ConnectionError
import firebase_admin
from firebase_admin import credentials
from firebase_admin import db
from itertools import combinations



def capture_snapshots(youtube_url):

    temp_video_file = 'temp_live_stream.ts'

    if os.path.exists(temp_video_file):
        os.remove(temp_video_file)

    command = ['streamlink', youtube_url, 'best', '-o', temp_video_file]

    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    time.sleep(4)

    cap = cv2.VideoCapture(temp_video_file)

    if not cap.isOpened():
        print("Failed to open the video stream.")
        process.terminate()  # Dừng process streamlink nếu không mở thành công
        stderr = process.stderr.read().decode()
        print(f"Streamlink error: {stderr}")
        return

    try:
        ret, frame = cap.read()

        if ret:
            return frame

        else:
            print('Failed to capture frame')

    except KeyboardInterrupt:
        print("Stopped capturing snapshots.")

    finally:
        # Giải phóng tài nguyên
        cap.release()
        process.terminate()  # Dừng process streamlink

        # Đợi cho đến khi quá trình đã hoàn thành trước khi xóa tệp
        process.wait()  # Chờ cho process kết thúc
        

cred = credentials.Certificate("firebase.json")
firebase_admin.initialize_app(cred, {"databaseURL": "https://doan1-c4eac-default-rtdb.firebaseio.com/"})

chicken_ref = db.reference("/doan1/coop/chicken")
egg_ref = db.reference("/doan1/coop/egg")
chicken_sleep_begin = 18
chicken_sleep_end = 6

device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
print(f"Using device: {device}")

model = YOLO('best-2024-11-14.pt').to(device)

id_stream = db.reference("/doan1/controller/camera_link_youtube_id").get()

youtube_url = "https://www.youtube.com/watch?v=" + id_stream

# Frame buffer for tracking (maximum of 20 previous frames)
frame_buffer = deque(maxlen=6)
max_frame = 6

# Track existing dots and their colors
dot_colors = []
tracked_dots = []  # Stores (object_center, color)
object_id_counter = 0


# Function to generate random colors
def generate_random_color():
    return (random.randint(0, 255), random.randint(0, 255), random.randint(0, 255))

# Function to calculate the distance between two points
def calculate_distance(point1, point2):
    return int(np.sqrt((point1[0] - point2[0]) ** 2 + (point1[1] - point2[1]) ** 2))

# Function to draw dots on the image
def draw_dots(image, dots):
    for dot, color, class_name in dots:
        cv2.circle(image, dot, 5, color, -1)  # Draw colored dot

# New variables for snapshot export
last_export_time = time.time()
export_interval = 20 # Export every 20 seconds
all_dots_in_interval = []


def export_snapshots(image_shape, all_dots, timestamp, original_image, nest_position):

    filtered_dots = []
    isNotMove = False

    for color in dot_colors:
        temp_dots = [dot for dot, dot_color, class_name in all_dots if (dot_color == color and class_name == "chicken")]
        if len(temp_dots) > 1:
        #(max_frame * 0.3):
            filtered_dots.extend([(dot, color) for dot in temp_dots])  # Filter the color more than the mean
            
            max_distance = 0

            for point1, point2 in combinations(temp_dots, 2):
                distance = calculate_distance(point1, point2)
                
                if distance >= max_distance:
                    max_distance = distance
                    if max_distance > 20:
                        break
                
            
            print(f"Number position for chicken {color}: {len(temp_dots)}")
            print(f"Maximum distance position for chicken {color}: {max_distance} pixels")

            if max_distance < 20:
                for n in nest:
                    isNotMove = True
                    x_first, y_first = temp_dots[0]
                    x_last, y_last = temp_dots[len(temp_dots)-1]
                    # Check if the egg is on the nest using bounding box coordinates
                    if ((n[0] < x_first < n[1] or n[1] < x_first < n[0]) and (n[2] < y_first < n[3] or n[3] < y_first < n[2])):
                        isNotMove = False
                        break
                    if ((n[0] < x_last < n[1] or n[1] < x_last < n[0]) and (n[2] < y_last < n[3] or n[3] < y_last < n[2])):
                        isNotMove = False
                        break
                    


    # Check if max distance is less than 20 pixels
    last_shot_time = datetime.now().strftime("[%d/%m/%Y %H:%M:%S]")
    
    if not filtered_dots:
        chicken_ref.update({"problem": "NO CHICKEN"})
        chicken_ref.update({"time": last_shot_time})
        print("No chicken")
    elif isNotMove:
        if datetime.now().hour > chicken_sleep_begin or  datetime.now().hour < chicken_sleep_end:
            print("Chicken sleep")
            chicken_ref.update({"problem": "NO"})
            chicken_ref.update({"time": last_shot_time})
        else:
            cv2.imwrite(f"Chicken_{timestamp}.jpg", original_image)
            print("Chicken not moving")
            chicken_ref.update({"problem": "YES"})
            chicken_ref.update({"time": last_shot_time})
    else:
        chicken_ref.update({"problem": "NO"})
        chicken_ref.update({"time": last_shot_time})
        print("Chicken moving")
 

try:
    nest = []
    nest_locked = False
    while True:
        # Get a frame from the ESP32 camera stream
        #frame = get_frame_from_stream(esp32_stream_url)
        frame = capture_snapshots(youtube_url)
        if frame is None:
            print("Cannot read the stream")
            time.sleep(1)  # Wait for a second before retrying
            break

        # Store the original frame without any modifications
        original_image = frame.copy()

        # Resize the frame to fit within a window size (e.g., 640x480)
        display_image = cv2.resize(frame, (640, 480))

        # Detect objects in each frame using GPU if available
        results = model(display_image)
        predictions = results[0]

        boxes = predictions.boxes
        labels = predictions.names

        new_dots = []
        new_object_tracking = {}

        egg = []
        
        # Process each detected object
        for i, box in enumerate(boxes):
            class_id = int(box.cls)
            confidence = box.conf.item()

            # Get the bounding box coordinates and object center
            x1, y1, x2, y2 = box.xyxy[0].int().numpy()

            # Only show objects with confidence above 0.70
            if confidence > 0.50:
                object_center = ((x1 + x2) // 2, (y1 + y2) // 2)

                current_class_name = labels[class_id] # Get the class name

                if not nest_locked:
                    if current_class_name == "nest":
                        nest.append([x1,x2,y1,y2])

                if current_class_name == "egg":
                    egg.append(object_center)

                # Check if the object matches any dot in the frame buffer
                assigned_color = None
                matched_class_name = current_class_name  # Default to current detected class

                for frame in frame_buffer:
                    min_distance = float('inf')
                    for (prev_dot, color, class_name_in_buffer) in frame:
                        if (class_name_in_buffer == "chicken"):
                            distance = calculate_distance(object_center, prev_dot)
                            if distance < 40 and distance < min_distance:
                                min_distance = distance
                                assigned_color = color
                                matched_class_name = class_name_in_buffer  # Get class name from the buffer

                if assigned_color:
                    new_dots.append((object_center, assigned_color, matched_class_name))
                    new_object_tracking[object_id_counter] = (object_center, assigned_color, matched_class_name)
                else:
                    new_color = generate_random_color()
                    dot_colors.append(new_color)
                    new_dots.append((object_center, new_color, current_class_name))
                    new_object_tracking[object_id_counter] = (object_center, new_color, current_class_name)
                    object_id_counter += 1

                # Draw bounding box and label on display_image (not in exported snapshots)
                cv2.rectangle(display_image, (x1, y1), (x2, y2), assigned_color or new_color, 2)
                label = f"{current_class_name}: {confidence:.2f}"
                (text_width, text_height), _ = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 0.5, 1)
                cv2.rectangle(display_image, (x1, y1 - text_height - 5), (x1 + text_width, y1), assigned_color or new_color, -1)
                cv2.putText(display_image, label, (x1, y1 - 5), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 0), 1)


        nest_locked = True
        timestamp = datetime.now().strftime("%d-%m-%Y_%H-%M-%S")
        last_shot_time = datetime.now().strftime("[%d/%m/%Y %H:%M:%S]")

        if nest != []:
            egg_in_nest = 0
            for n in nest:
                print("nest position: ", n)
                cv2.rectangle(display_image, (n[0],n[2]), (n[1],n[3]), (255,255,255), 2)
                label = f"locked nest"
                (text_width, text_height), _ = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 0.5, 1)
                cv2.rectangle(display_image, (n[0], n[2] - text_height - 5), (n[0] + text_width, n[2]), (255,255,255), -1)
                cv2.putText(display_image, label, (n[0], n[2] - 5), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 0), 1)
                # Loop through egg list and check positions relative to the nest
                for egg_position in egg:  # Use enumerate to get both index and tuple
                    egg_x, egg_y = egg_position  # Unpack the tuple (x, y)
         
                    if ((n[0] < egg_x < n[1] or n[1] < egg_x < n[0]) and (n[2] < egg_y < n[3] or  n[3] < egg_y <  n[2])):
                        egg_in_nest = egg_in_nest + 1
                        
            
            print("Number of egg in the nest = ",egg_in_nest)
            if egg == []:
                egg_ref.update({"out": "NO EGG"})
                egg_ref.update({"time": last_shot_time})
                print("NO EGG")
            elif egg_in_nest < len(egg):
                egg_ref.update({"out": "YES"})
                egg_ref.update({"time": last_shot_time})
                cv2.imwrite(f"Egg_{timestamp}.jpg", original_image)
                print("Egg out of the nest")
            else:
                egg_ref.update({"out": "NO"})
                egg_ref.update({"time": last_shot_time})
                print("Egg in the nest")

        else:
            egg_ref.update({"out": "NO NEST"})
            egg_ref.update({"time": last_shot_time})
            nest_locked = False
            print("There is no nest")

        # Check if it's time to export snapshots
        current_time = time.time()
        if current_time - last_export_time >= export_interval:
            export_snapshots(display_image.shape, all_dots_in_interval, timestamp, original_image, nest)
            last_export_time = current_time
            all_dots_in_interval = []  # Reset the list after export

        # Update the tracked_dots list
        tracked_dots = [(obj_center, color, class_name) for obj_center, color, class_name in new_object_tracking.values()]

        # Add current frame dots to the frame buffer
        frame_buffer.append(new_dots)

        # Update the all_dots_in_interval list
        all_dots_in_interval.extend(new_dots)

        # Display the frame
        cv2.imshow('ESP32 Camera Stream', display_image)

        if cv2.waitKey(1) & 0xFF == ord('x'):
            nest.clear()
            print("nest changed position")
        # Break the loop if 'q' is pressed
        if cv2.waitKey(1) & 0xFF == ord(' '):
            break

except KeyboardInterrupt:
    print("Stream interrupted by user")

finally:
    # Close windows
    cv2.destroyAllWindows()