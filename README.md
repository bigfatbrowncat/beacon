## Building

# Stage 1: Chromium

This version of the project uses Chromium codebase version 81.0.4002.1.

To prepare the build environment, follow the instructions from the Chromium source building documentation for the necessary version. Read and execute the following up to the `fetch chromium` command:

Windows:
```
https://chromium.googlesource.com/chromium/src/+/tags/81.0.4002.1/docs/windows_build_instructions.md
```
Linux:
```
https://chromium.googlesource.com/chromium/src/+/tags/81.0.4002.1/docs/linux_build_instructions.md
```
macOS:
```
https://chromium.googlesource.com/chromium/src/+/tags/81.0.4002.1/docs/mac_build_instructions.md
```
(note that this build instructions are taken from the proper tad. They are not the same as the published "most recent" ones)

We recommend use `fetch --no-history chromium` instead of just `fetch chromium` to economize drive space. This command will clone the main root 'src' repo.

Then go into `src`
```
cd src
git fetch https://chromium.googlesource.com/chromium/src.git +refs/tags/81.0.4002.1:chromium_81.0.4002.1 --depth 1
git checkout chromium_81.0.4002.1
```

Note the `--depth 1` option in the end. We are fetching a single commit marked with the tag! 

At last, run three commands that will fetch all the subprojects and prepare everything properly:
```
gclient sync
gclient sync --with_branch_heads
gclient runhooks
```

# Stage 2: The project code

Clone this project inside the 'src' folder.
```
git clone https://bitbucket.org/bigfatbrowncat/my_example.git
```

Apply the necessary patches to chromium sources:
```
(cd my_example && python apply_patches.py)
```

Then run the project generating script.
```
my_example/gen_Debug.sh
```
or
```
my_example/gen_Release.sh
```

After the script has finished running, it creates `out/Debug` or `out/Release`.

To build the project, run
```
ninja -C out/Debug -j 2 HelloWorld
```
or
```
ninja -C out/Release -j 2 HelloWorld
```

Then you should generate a project.