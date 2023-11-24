# Salient Object Detection With Convex Hull Overlap

This is the official implementation for the paper `Y. Liang, "Salient Object Detection with Convex Hull Overlap," 2018 IEEE International Conference on Big Data (Big Data), Seattle, WA, USA, 2018, pp. 4605-4612, doi: 10.1109/BigData.2018.8622033.` The [link](https://ieeexplore.ieee.org/document/8622033) to view the full text.

In this paper, we establish a novel bottom-up cue named Convex Hull Overlap (CHO), and then propose an effective approach to detect salient object using the combination of the CHO cue and global contrast cue. Our scheme significantly differs from other earlier work in: 1) The hierarchical segmentation model based on Normalized Graph-Cut fits the splitting and merging processes in human visual perception; 2) Previous work only focuses on color and texture cues, while our CHO cue makes up the obvious gap between the spatial region covering and the region saliency. CHO is a kind of improved and enhanced Gestalt cue, while other popular figure-ground cues such as convexity and surroundedness can be regarded as the special cases of CHO. Our experiments on a large number of public data have obtained very positive results.


![](BigData18.png)

## Usage

Download the salient object detection dataset `ECSSD` from the website `http://www.cse.cuhk.edu.hk/leojia/projects/hsaliency/dataset.html`.
Then change the path in `main.cpp` to your own path of the dataset.
```
	string dataset_name = "ECSSD/";
	string dir_path = "/mnt/e/Dataset/" + dataset_name;
	string gt_dir_path = dir_path + "ground_truth_mask/";
	string img_dir_path = dir_path + "images/";
```

To compile and run the code,
```
make
./SOD_CHO 128
```
`128` could be any threshold value between 0 and 255 to determine the binary mask of the salient object.


## Citation

```
@INPROCEEDINGS{8622033,
  author={Liang, Yongqing},
  booktitle={2018 IEEE International Conference on Big Data (Big Data)}, 
  title={Salient Object Detection with Convex Hull Overlap}, 
  year={2018},
  volume={},
  number={},
  pages={4605-4612},
  doi={10.1109/BigData.2018.8622033}}
```