# xrdp_local

xrdp_local is a workaround for the longest standing issue in xrdp -
You can't connect to the same session you get locally.


## How xrdp (usually) works
[xrdp](https://github.com/neutrinolabs/xrdp) opens a headless Xorg server that,
instead of loading regular hardware drivers (for your GPU and input devices),
loads the [xorgxrdp](https://github.com/neutrinolabs/xorgxrdp) driver. xorgxrdp
is a replacement for the actual hardware drivers in Xorg (like i915 for Intel
GPUs) that lets xrdp serve as its virtual devices. xrdp connects to that driver
and exposes its framebuffer and input devices to the RDP client. Xorg
applications get an actual, real X11 server with all the features the regular
one has.


## How xrdp_local doesn't work
My original plan was to find a way to reload Xorg drivers dynamically, so it
can actually switch between real hardware and xrdp on the fly (like the Windows
RDP server). Xorg has some inconvenient design choices that make this impossible
(without massive changes to its code), which as far as I can tell were made
because not doing so will break key assumptions in the X11 protocol (like
screens not changing at runtime).

See [How to do it properly](#how-to-do-it-properly).


## How xrdp_local works
xrdp_local allows you to use the xrdp Xorg server locally, as if it was actually
your local Xorg server. It uses the xup library from xrdp (which is what xrdp
uses to communicate with xorgxrdp), and opens a full screen window on a new,
empty Xorg server to show its content. Without the overhead of the RDP protocol,
this is actually usable. Combined with GLAMOR support in recent versions of xrdp
(which lets the xorgxrdp Xorg server and X11 applications use hardware graphics
acceleration despite running on a virtual display) and
[DMA-BUF buffer sharing](#dma-buf-buffer-sharing), it's basically the same
experience as logging in natively, but xrdp can connect to the same session too.

Basically, you set up your system to use xrdp_local as the default X11 session
(instead of KDE or GNOME), and xrdp_local connects to the xrdp Xorg server
(which runs your actual X11 session) instead of xrdp. When you connect using
xrdp, xrdp_local exits (so your local display goes back to the login screen),
and xrdp gets the session. When you login locally again, xorgxrdp automatically
disconnects the RDP client and you get the same session back.


## How to use it
The easiest way is to use
[xrdp_local_session](https://github.com/shaulk/xrdp_local_session), which adds
the necessary glue at the system level to make it work seamlessly. It registers
itself as a X11 session (which you have to pick at your login screen or set as
default).  When you start it, it instructs xrdp-sesman (the component in xrdp
that manages sessions) to start a new session if none is running, and then
starts xrdp_local.  xrdp_local_session will also automatically unlock the inner
session when you login (if it's locked), so you don't have to type your password
twice.

To use xrdp_local without xrdp_local_session, you can also set up your system to
run xrdp-sesrun at boot (to make sure there's a session running), and configure
your local GUI to run xrdp_local directly.

### Installing
See the installation instructions at
[xrdp_local_session](https://github.com/shaulk/xrdp_local_session) for a quick
start.

You'll need both xrdp_local and xrdp_local_session. If you want DMA-BUF
acceleration (which you probably do unless your machine is a VM with no GPU),
you'll also need our patched [xrdp](https://github.com/shaulk/xrdp) and
[xorgxrdp](https://github.com/shaulk/xorgxrdp). You probably want those anyway,
as xrdp_local uses some xrdp structs that don't have a stable ABI, so you need
xrdp_local in the exact same version as xrdp. If there's no package for your
version of xrdp or distro,
[you can build it from source](#building-from-source).

Binary packages of xrdp_local, xrdp_local_session, and the modified xrdp and
xorgxrdp are currently available for:
- Arch Linux, x86_64
- Debian 12 (bookworm), x86_64, arm64
- Debian 13 (trixie), x86_64, arm64
- Fedora 42, x86_64, arm64
- Ubuntu 22.04 (jammy), x86_64
- Ubuntu 24.04 (noble), x86_64, arm64
- Ubuntu 25.04 (plucky), x86_64, arm64

Package sources and binary releases for the modified xrdp and xorgxrdp
(including a PKGBUILD for Arch Linux) are available in the
[xrdp_local_deps](https://github.com/shaulk/xrdp_local_deps) repository.

Note that modern GNOME has several incompatibilies with xrdp_local and so
doesn't work well in its common default configuration. Check out the notes at
[xrdp_local_session](https://github.com/shaulk/xrdp_local_session) for more
information.

### Building from source
xrdp_local is a standard cmake project, so you can build it from source using
cmake:

```
git clone https://github.com/neutrinolabs/xrdp_local.git
cd xrdp_local
cmake -B build
cmake --build build
sudo cmake --install build
```

There are also a build directory for deb packages, an RPM spec file, and a
PKGBUILD for Arch Linux.

### Configuring X Server Permissions
In some modern distributions, Xwrapper is used to start the X server. In some
of them, by default only console users can start the X server, which prevents
xrdp from working (this is actually not specific to xrdp_local).

To allow non-console users to start the X server, in /etc/X11/Xwrapper.config
change allowed_users to 'anybody'.

In Debian-based distributions, this is managed by debconf, so run:
```
dpkg-reconfigure xserver-xorg-legacy
```
and select 'Anybody'.

If you use the precompiled .deb packages, this should be done automatically on
installation.


## DMA-BUF buffer sharing
[DMA-BUF](https://docs.kernel.org/driver-api/dma-buf.html) is a Linux kernel
subsystem that allows different devices and user-land processes to share
hardware buffers, including GPU memory. It's supported by most modern GPU
drivers. Wayland compositors use it to get buffers from applications without
copying them, and specifically, without having to copy them out of GPU memory.

The regular xorgxrdp protocol only implements regular shared memory between
itself and its client (to which it copies screen data from GPU memory), which
is fine for xrdp, which has to copy screen data from the GPU to system RAM
anyway (to send it to clients). In our case, we don't need screen data to leave
the GPU at all. Our patches to
[libxup](https://github.com/shaulk/xrdp/tree/dmabuf/xup) and
[xorgxrdp](https://github.com/shaulk/xorgxrdp) allow xrdp_local to get
a reference to the offscreen framebuffer in GPU RAM, and then render it to the
real display directly.

When __not__ using DMA-BUF sharing, a single 1080p screen on a modern machine
without DMA-BUF acceleration gets ~60 fps (with relatively high CPU usage when 
there are many screen updates, like when watching a video). The screen copies
are O(n), so two 1080p displays bring this down to ~30 fps, which is usable but
suboptimal. A 4K display brings this down to ~15 fps, which is not usable.

With DMA-BUF sharing, basically anything your hardware can do natively can run
at full speed because behind the scenes xrdp_local works just like a regular
Wayland compositor, doing zero copies and letting the GPU do all the heavy
lifting.


## Sound support
xrdp_local doesn't support sound because it doesn't have to. [xrdp's PipeWire 
driver](https://github.com/neutrinolabs/pipewire-module-xrdp) doesn't disable
the real hardware, just overrides it when connected, so as long as it's
installed, it just works (and the real hardware works even without it).
When you connect using xrdp it takes precedence over the local hardware (which
keeps working, you can use it by selecting it manually). When you login locally,
the xrdp PipeWire module disables itself and your local sound hardware goes back
to being the default.

This obviously assumes you're using a modern system with PipeWire. I imagine it
works the same with the PulseAudio driver, but I haven't tested it. Other sound
systems will probably just output to local hardware even when xrdp is connected
as long as they're configured properly.


## State of xrdp_local

### What works
- Display including graphics acceleration.
  ([if supported by xorgxrdp](#DMA-BUF-acceleration-requirements))
- Keyboard and mouse input, with basic scrolling support.
- Seamless reconnection to the same session.
- [Sound](#sound-support).
- Input device hotplug - simply because it's handled automatically by the outer
  Xorg server.
- Auto unlock of the inner session (using
  [xrdp_local_session](https://github.com/shaulk/xrdp_local_session)).

### What kinda works
- Keyboard support is a hack and needs work. It probably has bugs in non-English
  latin layouts (though non-latin layouts that are overlaid on an English layout
  work fine as far as I've tested).
- Other input devices (like tablets), forwarded as regular mouse events.

### What doesn't work but is coming
- Display hotplug - currently you have to restart xrdp_local, which doesn't
  restart your session.
- Keyboard lock synchronization - right now your capslock state may get out of
  sync with the inner session.

### What doesn't work and won't come without changes to xorgxrdp
- Actual touch input support.
- High resolution scrolling.
- Trackpad and touch screen gestures.
- Changing display arrangement from within the session.
- High-DPI awareness and automatic scaling.
- HDR support.
- Exposing the different input devices within the inner session (so you can't
  explicitly configure different input devices when gaming).

Note that all of these missing features aren't supported in xorgxrdp because
it's designed for RDP, not for a local client. Adding them would mean that
xorgxrdp will officially support local clients using xrdp_local.

### What will probably never work
- Wayland support (see [How to do it properly](#how-to-do-it-properly)).

Note that this is a very new project and so there are probably many more bugs I
haven't seen myself.

PRs are welcome.


## Requirements
- xrdp >= 0.10
- xorgxrdp >= 0.10
- Qt 6
- [libargparse](https://github.com/p-ranav/argparse)
- Some X11 display manager (SDDM, LightDM, etc)

The general xrdp_local (without DMA-BUF acceleration) should work on any
configuration that supports xrdp (including VMs with no GPUs). If you want
DMA-BUF acceleration, see the
[DMA-BUF hardware support section](#dma-buf-acceleration-requirements).

xrdp_local uses some xrdp structs that don't have a stable ABI, so if you
use binary packages, you need to use the same version of xrdp_local as xrdp.

### DMA-BUF acceleration requirements
  - Linux (other platforms that support the DMA-BUF EGL extensions like FreeBSD
    and NetBSD might work but are untested, PRs are welcome!).
  - Our patched [xorgxrdp](https://github.com/shaulk/xorgxrdp) and
    [xrdp](https://github.com/shaulk/xrdp).
  - A GPU with a
    [driver that supports DMA-BUF](#hardware-support-for-dma-buf-acceleration),
	  GLAMOR, DRI3, and these EGL extensions (use eglinfo to check):
      - EGL_EXT_image_dma_buf_import
      - EGL_KHR_gl_texture_2D_image  
	(if Wayland works with hardware acceleration and you don't use NVIDIA's
  proprietary driver, you're probably good)
  - GLAMOR enabled in xrdp's Xorg server config (/etc/X11/xrdp/xorg.conf) and
    the desired driver in DRMAllowList.
  - The user you use has to have permissions to open /dev/dri/renderDxxx (in
    most distros, add them to the 'render' group).

xrdp_local will automatically disable DMA-BUF acceleration if it's not supported by
your hardware or configuration.

In most distros, you can see logs in `~/.xsession-errors` and
`~/.xorgxrdp.XX.log` (where XX is the X11 display number).

#### Hardware support for DMA-BUF acceleration
Anything marked as not supported in this list will work, but [will be
slower](#DMA-BUF-buffer-sharing).

| Hardware Vendor   | Driver     | Supported              | Notes   |
|-------------------|------------|------------------------|---------|
| Intel             | i915       | :white_check_mark: Yes |         |
| Intel             | xe         | :white_check_mark: Yes | 1       |
| NVIDIA            | nouveau    | :white_check_mark: Yes | 1       |
| NVIDIA            | nvidia     | :x: No                 | 2, 3    |
| AMD               | amdgpu     | :white_check_mark: Yes |         |
| AMD               | radeon     | :question: Probably    | 4       |
| Apple             | asahi      | :question: Probably    | 1, 4, 7 |
| Raspberry Pi      | v3d        | :white_check_mark: Yes | 1       |
| Qualcomm          | msm        | :question: Probably    | 4       |
| VMware            | vmwgfx     | :white_check_mark: Yes | 1, 5    |
| QEMU (std)        | bochs-drm  | :x: No                 | 8       |
| QEMU (cirrus)     | cirrus     | :x: No                 | 8       |
| QEMU (QXL)        | qxl        | :x: No                 | 8       |
| QEMU (virtio-gpu) | virtio_gpu | :white_check_mark: Yes | 1, 6, 8 |

If you test it on other platforms not listed here, please open an issue and tell
us about your results.

1. xrdp GLAMOR support (which is required for DMA-BUF acceleration) is
disabled by default in xorgxrdp (by the driver not being in the default
DRMAllowList in /etc/X11/xrdp/xorg.conf), but if you enable it, it works.
2. Neither the old proprietary driver nor the new open-source NVIDIA kernel
driver are supported, at least as of R570. Both appear to work in xrdp if
enabled, but glamor_fd_from_pixmap fails because it's not properly supported.
[See the discussion in their GitHub
project](https://github.com/NVIDIA/open-gpu-kernel-modules/discussions/243) for
more details.
3. Note that if you have a dual-GPU system and your physical display is
connected to the non-NVIDIA GPU (which is what you'll find in most muxless
Optimus laptops), you can configure xrdp to use the other GPU, which should
work if it's supported. If your display is connected to the NVIDIA GPU, it won't
work because the NVIDIA driver doesn't support DMA-BUF sharing with other
drivers.
4. Untested but should probably work (if you test it, open an issue and tell
us if it works!).
5. DMA-BUF support in VMware requires that 3D support be enabled in the VM
video card settings (even if using software rendering on a host with no GPU).
6. QEMU's virtio-gpu supports DMA-BUF when VirGL is enabled.  See [QEMU
virtio-gpu in Arch wiki for more details](https://wiki.archlinux.org/title/QEMU#virtio).
7. Asahi's DRM driver is designed with Wayland in mind so it should work, but
[the Asahi team does not test Xorg](https://asahilinux.org/docs/project/faq/#i-am-having-performancetearingfeature-issues-on-xorg)
so there may be issues not found in other supported hardware. Obviously, if
you're using Asahi Linux with Wayland (or any Linux with Wayland, for that
matter), [this entire project is not for you](#how-to-do-it-properly).
8. Many open source virtualization applications (like
[virt-manager](https://virt-manager.org/)) use QEMU by default, so this applies
to them too.

If you have multiple GPUs that support DMA-BUF (e.g. an onboard Intel iGPU and a
discrete AMD GPU) and you get garbled output, you may be using a different GPU
for GLAMOR in xrdp's Xorg server than the one you display on. Try changing the
DRMDevice value in /etc/X11/xrdp/xorg.conf to the GPU you actually use.

If your cards keep changing their order in /dev/dri, you can use the symlinks in
/dev/dri/by-path which don't change across reboots (unless your hardware
changes, e.g. some systems don't hardcode the PEG slot to a specific PCI device
number, so installing a non-graphics card can change the PCI device order and
move your discrete GPU to a different PCI device number, and then you have to
update your config).

### Optimizing performance when not using DMA-BUF acceleration
If you can't use DMA-BUF sharing because you don't have supported hardware,
there are very few things you can do to improve performance.

The only practical one is to disable compositing in your window manager.
Modern compositing X11 window managers (KWin, Mutter, etc.) update the entire
screen on every change, so xorgxrdp can't do incremental updates (and since
xrdp_local gets its data from xorgxrdp, it can't do them either). If you disable
compositing, incremental updates work properly and then it's O(n) with respect
to the update size instead of O(n) with respect to the screen resolution, so you
don't update the entire screen just for a blinking cursor.

You can also lower your screen resolution, which might be useful in a VM, but
probably not on your workstation.


## How to do it properly
xrdp_local works, but it's a workaround. It doesn't actually solve the core
issue, which is that Xorg can't switch drivers on the fly. Also, Xorg is legacy
software at this point, and Wayland is the future.  
If I had the time, what I'd do is fork xorgxrdp, remove X11 specific code and
make it into a generic library. I'd then add support for that library in
[KWin](https://github.com/KDE/kwin),
[Mutter](https://gitlab.gnome.org/GNOME/mutter) and
[wlroots](https://gitlab.freedesktop.org/wlroots/wlroots) (temporarily disabling
physical hardware when a client connects), which should be enough for basically
every modern Wayland desktop. If
[Wayback](https://gitlab.freedesktop.org/wayback/wayback) takes off, it should
solve the issue for X11 desktops too as Wayback uses wlroots.

If someone does it, I'd happily endorse it as the future of xrdp_local.

### What about libfreerdp based projects like GNOME Remote Desktop and KRdp?
While [GNOME Remote Desktop](https://gitlab.gnome.org/GNOME/gnome-remote-desktop)
and [KRdp](https://github.com/KDE/krdp) work just fine within the limitations of
the current open source RDP ecosystem, those limitations make them impractical
for actually using them as remote desktop software. The biggest issue is that
while they let you connect to your existing session, since they have no proper
support from the compositor and just use the same APIs used by screen recording
software, your local machine stays open and someone at your keyboard can see and
control everything you do.  Also, since they live within your session (and not
at the system level), you have to log in locally before you can remote into your
session (or configure your machine to auto-login on boot, which is a whole other
can of worms). This makes them impractical for anything but the simplest home
usage scenarios.

GNOME Remote Desktop actually does have a system-level daemon, but since it's
not integrated with Mutter (and GNOME's session management in general), it force
closes your local session when you connect.

IMO the correct way to build a proper remote desktop system is to have a
system-level daemon that handles incoming connections (i.e. xrdp), and let it
route them to the right place. With proper support from Wayland compositors
(who would only have to import the hypothetical xorgxrdp library I mentioned
earlier, and disable physical hardware when a client connects), it should be
as good as the current Windows RDP experience.

This is actually exactly the same as using an ssh daemon combined with tmux for
session management, but for graphical sessions.
