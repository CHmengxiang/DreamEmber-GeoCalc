# DreamEmber GeoCalc (梦烬几何)

> A native OpenHarmony geometry & algebra graphing calculator — construction, function graphs, sliders, spreadsheet, CAS. Fully offline.

**DreamEmber GeoCalc** is built with pure-native ArkTS plus an NAPI C++ kernel (no Web components in core features). It works entirely offline, requests no permissions and collects no data.

## Features

- **Geometric construction**: points, segments, lines, rays, circles, polygons, angles, tangents, midpoints, intersections, perpendiculars, perpendicular/angle bisectors, vectors; transformations (rotation, dilation, translation); measure length, distance, area, perimeter, slope and angle;
- **Algebra view**: all objects listed with inline rename / revalue / redefine, live-linked to the canvas;
- **Functions & commands**: built-in math symbol keyboard, incremental command completion; derivative, integral, root and extremum computed exactly by the local CAS engine;
- **Sliders**: parametrize values, drag to update graphics in real time, four fields edited inline;
- **Spreadsheet**: cell-by-cell editing, submit to create objects, linked with the data view;
- **Dual layout**: single-hand portrait on phones; three-pane docked layout on tablets / 2-in-1 with freely resizable windows;
- **Dark / light mode** follows the system;
- **Fully offline**: no ads, no account, no network, no data collection — files stay on device.

## Requirements

- OpenHarmony SDK **API 23**
- Device types: `default` (phone/tablet) and `2in1` (free-window wide layout)

## Build from source

1. Clone the repo, then open the project root in **DevEco Studio**.
2. Configure signing via `File > Project Structure > Signing Configs` — **no signing material is included** in the repository (`signingConfigs` is empty).
3. Build via `Build > Build Hap(s)/APP(s)`, or CLI:

   ```bash
   node "C:\Program Files\Huawei\DevEco Studio\tools\hvigor\bin\hvigorw.js" \
     --mode module -p module=dreamEmberGeoCalc@default -p product=default \
     -p buildMode=release assembleHap --no-daemon
   ```

The C++ kernel and the statically linked **giac** CAS source are compiled automatically by hvigor via CMake (first build takes a while).

## License

Released under **GPL-3.0-or-later** (see [LICENSE](LICENSE)). The kernel statically links the open-source CAS library **giac** (© Bernard Parisse / Institut Fourier, GPL-3.0-or-later; source shipped in `entry/src/main/cpp/giac/`), so the whole project is distributed under GNU GPL-3.0-or-later.

## Disclaimer

An independent open-source project, **not affiliated with GeoGebra GmbH**; no GeoGebra trademarks, graphic assets or source code are used (its published source and interaction design were referenced during development).

## Author

**CH梦想** (CH_mengxiang) · [Gitee](https://gitee.com/CH_mengxiang)
