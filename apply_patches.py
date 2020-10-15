#!/usr/bin/python

import os



def apply(path, target):
    for p in os.listdir(path):
        fp = os.path.join(path, p)
        if p.endswith(".patch"):
            print("* Applying " + p)
            cur_dir = os.getcwd()
            os.chdir(target)
            os.system("git apply --ignore-whitespace --whitespace=nowarn " + fp)
            os.chdir(cur_dir)
        else:
            continue

cur_dir = os.getcwd()
patch_path = os.path.join(cur_dir, "chromium_patches")
target_path = os.path.join(cur_dir, "..")

apply(patch_path, target_path)
#apply(os.path.join(patch_path, "third_party/skia"), os.path.join(target_path, "third_party/skia"))

#os.system('ls -l')