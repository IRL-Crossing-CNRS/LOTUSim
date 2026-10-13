# Unity troubleshooting

## The Unity Editor crashes when opening the project (Linux, NVIDIA)

**Symptom.** The LOTUSim-Energy project crashes the Unity Editor almost every
time you open it, usually a few seconds after the project loads, and Unity
Hub reports a crash or the window simply disappears. `Editor.log`
(`~/.config/unity3d/Editor.log`) contains:

```text
Got a SIGSEGV while executing native code.
...
libnvidia-glcore.so.<driver version>
...
GfxDeviceVK::AdjustPrimarySwapChain
```

**Why.** The project uses the High Definition Render Pipeline (HDRP), which
needs Vulkan on Linux. Unity `2022.3.18f1` is an older release; with a recent
NVIDIA driver (observed with 595.99 on Ubuntu 24.04, an RTX 4090 laptop GPU
and an X11 session), the NVIDIA driver crashes when Unity creates the window's
Vulkan swapchain. The project, its assets and the amount of memory are not
involved, which is why the same project may open fine on another machine.

**Workaround: run the Editor on another GPU.** If your machine has an
integrated GPU next to the NVIDIA one, start Unity from a terminal and tell it
to render on that device:

```sh
~/Unity/Hub/Editor/2022.3.18f1/Editor/Unity \
  -projectPath <path to the Unity project> \
  -force-vulkan -force-device-index 0
```

Adjust the Unity path to where Unity Hub installed the editor. Unity Hub
cannot pass this argument, so launch the Editor from the terminal (or from a
launcher of your own).

`-force-device-index` is the position of the GPU in the list that Unity prints
at start-up. On the machine where this was found, `0` was the Intel
integrated GPU and `1` the NVIDIA one. To find yours, open the log and read
the `Physical Device` lines:

```sh
grep "Physical Device" ~/.config/unity3d/Editor.log
```

The line `Vulkan renderer=[...]` tells you which GPU the Editor actually uses.
Rendering on an integrated GPU is slower, so expect a lower frame rate in
heavy scenes.

**What does not work.**

- `-force-glcore` (OpenGL) stops the crash, but HDRP does not support OpenGL:
  the Editor opens and the scene stays empty.
- Resetting the Editor window layout, disabling the NVIDIA threaded
  optimisations (`__GL_THREADED_OPTIMIZATIONS=0`) or using PRIME offload
  variables did not change anything.

**Longer-term fixes.** Installing another NVIDIA driver branch (for example
the 570 or 580 production branch), or upgrading to a recent `2022.3.x` patch
release of Unity, are the two real fixes. Neither has been tested here, so it
is not known which one resolves the crash.

## Unity cannot find Blender

`Error: execv(blender): No such file or directory` appears when Unity imports
a `.blend` file (for example `Assets/ArgoFloat/Argo.blend`). Install Blender
and make sure `blender` is on your `PATH`, or ignore the message if you do not
need that model.
