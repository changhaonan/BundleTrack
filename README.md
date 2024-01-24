# BundleTrack-Super-Point

Modified from Bowen's BundleTracking. Replace its original point-feature to superpoint. Further modification includes change CUDA's compatibility and add support for c++14.

## Prepare data

Data should be formated as follows:
```
color/
    - 0.png
    - 1.png
depth/
    - 0.png
    - 1.png
seg/
    - 0.png
    - 1.png
intrinsics.txt
```


## Run instruction

1. Start super point feature server:
```python
python external/PointFeatureHub2/run.py task=detect detect=super_point draw_keypoints=false
``` 

2. Run bundletrack
```bash
./build/bundle_track_colmap config_colmap.yml
```

3. After running, we can visualize the running result with Easy3DViewer.

```bash
python external/Easy3DViewer/example/python/check_sequence_poses.py --frame_skip=100 --traj_path=${traj_path} --data_path=${data_path}
```