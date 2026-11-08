# Dimension Z Android hub

Native Android launcher, package in.dimensionz.hub, version 0.1.0-prototype.
The original logo combines three spatial axes, a cyan Z and violet third-axis arrow.
It opens installed Delta Bots and Portal Z packages or browser review/download pages.
It has no runtime permissions, embedded credentials, account system or background service.
AR functionality and a shared Unreal launcher are not implemented.

Build with build.ps1 after the official Android SDK/build tools described by the
Portal Z build are available. Tool paths currently reuse the workspace's portal-z
tools. Signing keys remain local and excluded from source publication.

web/index.html is the public download page. The Android and SVG icons share the
same original geometry. Physical controller focus navigation and layout should
be checked on a phone; this app does not provide its own VR renderer.
