# pitchtool

A JUCE based audio plugin (vst3, lv2) for pitching your voice up or down.

https://user-images.githubusercontent.com/42894124/175772918-08d41f37-0639-4bc7-8308-7cc24682bff4.mp4

## Build

Building has been tested successfully on Manjaro Linux using gcc11.2.
clang13 does not work because of missing parts of c++20 ranges and concepts.
Other compilers or OSes might or might not work.

```
git clone git@github.com:StephanWidor/pitchtool.git
cd pitchtool
git submodule update --init --recursive
mkdir build
cd build
cmake ..
make -j$(nproc)
```

After successful build, Standalone app, LV2, and VST3 plugin can be found
in folder build/juce/jucepitchtool_artefacts.
