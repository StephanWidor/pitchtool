# pitchtool

An JUCE based audio plugin (vst3, lv2) for pitching your voice up or down.


https://user-images.githubusercontent.com/42894124/162634774-3fd168ed-93ec-4d54-8161-442e580fbbf2.mp4


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
