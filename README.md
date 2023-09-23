# BundleTrack-2023

A fork of BundleTrack. Replacing the original 2D tracker with pytracking and superpoints keypoint detector.

## Installation

The current mask generation relies on pytracking. To install pytracking. You need to change your python version to `3.8` and change your torch version to `1.10`.

## Usage

Currently we support to run BundleTrack on offline data. To launch it, you need to first configure parameters in `config_colmap.yml`. More specifically, you need to change `data_dir`, `mask_dir`.

The colmap formats looks like follows:
```
data_folder/
- color/
    - i.png
- depth/
    - i.png
- seg/
    - i.png
```

## Record new data

To run a new trajectory of data, you need to start with recording raw images.

```
python scripts/record_colmap_seq.py --data_folder=${data_folder} --data_name=${data_name}
```

Replace \${data_folder} with your `data_folder` path, and replace \${data_name} with your selected `data_name`.

Then you need to run pytracking to retrieve mask.

## Run 2D tracker

```
python run_video.py rts rts50 ${DATA_PATH}/%d.png --save_results
```

## Debug

We are using Easy3DViewer to visualize the resulting trajectory of objects. To run Easy3DViewer, run the following commands:

Generate visualization sequence:
```
python external/Easy3DViewer/example/python/check_sequence_poses.py --traj_path=log/poses --data_path=data/${dataset_name}
```

Visualize result:
```
cd external/Easy3DViewer
node app.js
```
