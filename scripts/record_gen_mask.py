"""Generate mask & 2D tracking using pytracking"""
import os
import numpy as np
from natsort import natsorted
import cv2
import argparse

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--pytrack_result_dir", type=str, default="/home/robot-learning/Projects/ws_Haonan/BundleTrack/external/pytracking/pytracking/tracking_results/rts")
    parser.add_argument("--colmap_result_dir", type=str, default="/home/robot-learning/Projects/VIL2/test_data/exp_0/")
    args = parser.parse_args()
    # First run pytracking to generate the mask
    # Then run this script to transfer pytracking mask to colmap format
    colmap_seg_dir = os.path.join(args.colmap_result_dir, "seg")
    os.makedirs(colmap_seg_dir, exist_ok=True)
    image_list = natsorted(os.listdir(args.pytrack_result_dir))
    counter = 0
    for id, image_file in enumerate(image_list):
        if (not image_file.endswith(".png")) and (not image_file.endswith(".jpg")):
            continue
        image_path = os.path.join(args.pytrack_result_dir, image_file)
        image = cv2.imread(image_path, cv2.IMREAD_UNCHANGED)
        image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
        image[image > 0] = 255
        cv2.imwrite(os.path.join(colmap_seg_dir, "{}.png".format(counter)), image)
        counter += 1