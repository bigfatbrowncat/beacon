# About Beacon GUI framework
Beacon is a Graphics User Interface framework that provides a "web-like" HTML+CSS+JS development interface and allows to connect it to a native (C/C++) backend.

Beacon's motto: 

    We shall use JavaScript for the UI, 
    but native languages for the backend.
    
    And let both of them be as simple as possible! Amen.

## What's the killer feature? 
Unlike the popular competitors (such as Electron and NW.js), Beacon does not include the full browser code to itself. Instead, it adopts only the HTML renderer engine and JavaScript virtual machine directly. Thus it does not have any of the drawbacks, which are typical to "including the full browser" approach:

* Beacon is **small** (about 70 megabytes)
* Beacon **starts instantly**, as any other small native application like Microsoft Windows Calculator
* Beacon creates **no excessive processes** (no irritating additional process-per-window browser-like sandboxing thing, no IPC calls inside your application)
* Beacon **allows direct synchroneous calls** from the application _frontend_ to its _backend_ (and makes it simple to make them)
* Beacon updates, layouts and draws the contents **faster and smoother** than any other HTML/JS-based UI framework does (especially during the resizing of its windows, when Beacon can literally _compete with native UI applications_ like Notepad or Windows Explorer).

_We are doing our best to develop and keep Beacon as easy to adopt as possible._

# Building

## Stage 1: Chromium

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

Note: it might be easier to install essential x86 tooling only by running `sudo ./install-build-deps.sh --syms --no-arm --no-nacl --no-backwards-compatible --no-chromeos-fonts`

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

## Stage 2: The project code

Clone this project inside the 'src' folder.
```
git clone https://github.com/bigfatbrowncat/beacon
```

Apply the necessary patches to chromium sources:
```
(cd beacon && python apply_patches.py)
```

### Replacing sysroot on Linux

Note: this has been checked on Debian 9 (sid)

After patching the source, build new sysroot:
```
cd build/linux/sysroot_scripts/
./sysroot-creator-sid.sh BuildSysrootAmd64
cd ..
mv debian_sid_amd64-sysroot/ debian_sid_amd64-sysroot.old
mkdir debian_sid_amd64-sysroot/
cd debian_sid_amd64-sysroot/
tar xvf ../../../out/sysroot-build/sid/debian_sid_amd64_sysroot.tar.xz
cp ../debian_sid_amd64-sysroot.old/.stamp .
cd ../../
```

If any package fails to download by script `wget` it manually and re-run the creating script, it would continue on.

### Generating ninja configs

Then run the project generating script.
```
beacon/gen_Debug.sh
```
or
```
beacon/gen_Release.sh
```

After the script has finished running, it creates `out/Debug` or `out/Release`.

### Other Linux quirks

For some reason (yet unknown) a few generated files are not generated when running normal build
even though they are specified in the dependency chain. To make them appear run
```
ninja -C out/Debug -j 2 gpu/config
```
It is okay for now if this command eventually fails.

### Building THE THING

To build the project, run
```
ninja -C out/Debug -j 8 Beacon
```
or
```
ninja -C out/Release -j 8 Beacon
```

Then you should generate a project.
